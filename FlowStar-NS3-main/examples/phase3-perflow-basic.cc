/**
 * Phase 3 Experiment: Basic per-flow queue validation.
 *
 * Topology: 4 senders → SW1 → bottleneck → SW2 → 1 receiver
 * 4 physical queues on the bottleneck device.
 *
 * Verifies:
 *  - Each flow is assigned a dedicated queue (when queues > flows-1)
 *  - CbfcQueueDisc correctly demultiplexes by MID
 *  - Credit mechanism allows full transmission
 *  - Outputs flow_queue_mapping.csv and flow_results.csv
 */
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/traffic-control-module.h"
#include "../model/topology-builder.h"
#include "../model/cbfc-queue-disc.h"
#include "../model/cbfc-credit-manager.h"
#include "../model/flowstar-mid.h"
#include "../application/cbfc-traffic-sender.h"
#include "../application/cbfc-traffic-receiver.h"
#include "../utils/metrics-collector.h"
#include "../utils/csv-writer.h"
#include <vector>
#include <fstream>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("Phase3PerflowBasic");

int main (int argc, char *argv[])
{
  uint32_t numSenders       = 4;
  uint32_t numQueues        = 4;
  std::string senderRate    = "10Gbps";
  std::string bottleneckRate= "1Gbps";
  std::string delay         = "1us";
  uint32_t packetSize       = 1000;
  uint64_t messageSize      = 500000; // 500 KB per flow
  uint32_t initialCredit    = 65536;  // 64 KB per flow
  uint32_t fccl             = 65536;
  uint16_t dataPort         = 9;
  uint16_t creditPortBase   = 6000;

  CommandLine cmd;
  cmd.AddValue ("numQueues",     "Number of physical queues",        numQueues);
  cmd.AddValue ("messageSize",   "Message size in bytes",            messageSize);
  cmd.AddValue ("initialCredit", "Initial FCTBS per flow (bytes)",   initialCredit);
  cmd.Parse (argc, argv);

  TopologyBuilder topo;
  NodeContainer hosts    = topo.CreateHosts (numSenders + 1); // +1 receiver
  NodeContainer switches = topo.CreateSwitches (2);

  topo.InstallInternetStack (hosts);
  topo.InstallInternetStack (switches);

  // Connect senders to SW1
  std::vector<NetDeviceContainer> senderLinks;
  for (uint32_t i = 0; i < numSenders; ++i)
    {
      auto lnk = topo.ConnectHostToSwitch (hosts.Get(i), switches.Get(0), senderRate, delay);
      senderLinks.push_back (lnk);
      std::ostringstream net; net << "10.1." << (i+1) << ".0";
      topo.AssignIpv4Addresses (lnk, net.str ().c_str (), "255.255.255.0");
    }

  // Receiver to SW2
  NetDeviceContainer recvLink = topo.ConnectHostToSwitch (hosts.Get(numSenders), switches.Get(1), senderRate, delay);
  topo.AssignIpv4Addresses (recvLink, "10.2.1.0", "255.255.255.0");

  // SW1 → SW2 (bottleneck)
  NetDeviceContainer sw1sw2 = topo.ConnectSwitchToSwitch (switches.Get(0), switches.Get(1), bottleneckRate, delay);
  topo.AssignIpv4Addresses (sw1sw2, "10.3.1.0", "255.255.255.0");

  topo.PopulateRoutingTables ();

  // Install CbfcQueueDisc on SW1's bottleneck-facing device
  // Remove any auto-installed default QueueDisc before installing CbfcQueueDisc
  TrafficControlHelper tchRemove;
  tchRemove.Uninstall (sw1sw2.Get (0));

  TrafficControlHelper tch;
  tch.SetRootQueueDisc ("ns3::CbfcQueueDisc",
                        "NumQueues", UintegerValue (numQueues),
                        "QueueSize", UintegerValue (1000));
  QueueDiscContainer qdiscs = tch.Install (sw1sw2.Get (0));
  Ptr<CbfcQueueDisc> cbfcDisc = DynamicCast<CbfcQueueDisc> (qdiscs.Get (0));

  // Receiver setup
  Ptr<CbfcTrafficReceiver> receiver = CreateObject<CbfcTrafficReceiver> ();
  receiver->Setup (dataPort);
  hosts.Get (numSenders)->AddApplication (receiver);
  receiver->SetStartTime (Seconds (0.0));
  receiver->SetStopTime  (Seconds (3.0));

  Ptr<Ipv4> recvIpv4 = hosts.Get (numSenders)->GetObject<Ipv4> ();
  Ipv4Address destAddr = recvIpv4->GetAddress (1, 0).GetLocal ();

  // Shared credit manager (per-flow state is keyed by MID internally)
  Ptr<CbfcCreditManager> creditManager = CreateObject<CbfcCreditManager> ();

  // Sender setup
  for (uint32_t i = 0; i < numSenders; ++i)
    {
      uint16_t creditPort = creditPortBase + i;
      FlowStarMid mid (i, numSenders, i+1, 1);

      creditManager->InitializeFlow (mid, initialCredit, fccl);

      Ptr<Ipv4> srcIpv4 = hosts.Get (i)->GetObject<Ipv4> ();
      Ipv4Address srcAddr = srcIpv4->GetAddress (1, 0).GetLocal ();

      // Register sender credit address with receiver
      receiver->RegisterSenderCredit (mid, InetSocketAddress (srcAddr, creditPort));

      Ptr<CbfcTrafficSender> sender = CreateObject<CbfcTrafficSender> ();
      sender->SetCreditManager (creditManager);
      sender->SetCreditListenPort (creditPort);
      sender->Setup (mid,
                     InetSocketAddress (destAddr, dataPort),
                     InetSocketAddress (destAddr, dataPort), // unused in Phase 3
                     packetSize, messageSize, DataRate (senderRate));
      hosts.Get (i)->AddApplication (sender);
      sender->SetStartTime (Seconds (0.1));
      sender->SetStopTime  (Seconds (3.0));
    }

  MetricsCollector::GetInstance().Reset();
  MetricsCollector::GetInstance().InstallTxRxDropHooks();
  MetricsCollector::GetInstance().InstallQueueHooks();

  Simulator::Stop (Seconds (3.0));
  Simulator::Run ();
  Simulator::Destroy ();

  // ---- Output ----
  std::cout << "=== Phase 3: Per-Flow Basic ===" << std::endl;
  std::cout << "Packets Dropped: " << MetricsCollector::GetInstance().GetTotalDroppedPackets() << std::endl;
  std::cout << "Max Queue Occupancy: " << MetricsCollector::GetInstance().GetMaxQueueOccupancy() << std::endl;
  std::cout << "\nQueue mapping:" << std::endl;

  // Write flow_queue_mapping.csv
  std::ofstream mapFile ("flow_queue_mapping.csv");
  mapFile << "srcFlowId,dstFlowId,queueId,isDedicated\n";

  for (uint32_t i = 0; i < numSenders; ++i)
    {
      FlowStarMid mid (i, numSenders, i+1, 1);
      uint32_t qIdx = cbfcDisc->GetQueueIndex (mid);
      bool dedicated = (qIdx != UINT32_MAX) && cbfcDisc->IsDedicatedQueue (qIdx);
      std::cout << "  Flow " << (i+1) << " → Queue " << qIdx
                << (dedicated ? " (dedicated)" : " (shared)") << std::endl;
      mapFile << (i+1) << ",1," << qIdx << "," << (dedicated ? 1 : 0) << "\n";
    }
  mapFile.close ();

  // Write flow_results.csv
  std::ofstream resultsFile ("flow_results.csv");
  resultsFile << "flowId,bytesReceived,packetsReceived\n";
  const auto& stats = receiver->GetFlowStats ();
  for (const auto& kv : stats)
    {
      resultsFile << kv.first.srcFlowId << ","
                  << kv.second.bytesReceived << ","
                  << kv.second.packetsReceived << "\n";
    }
  resultsFile.close ();

  std::cout << "CSV files: flow_queue_mapping.csv, flow_results.csv" << std::endl;
  return 0;
}
