/**
 * Phase 3 Experiment: Direct Phase 2 vs Phase 3 comparison.
 *
 * Same topology, flows, message sizes, link rates, and simulation duration.
 * Runs both Phase 2 shared-queue baseline and Phase 3 CBFC.
 * Reports side-by-side metrics.
 *
 * Run: --mode=phase2  or  --mode=phase3
 */
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/traffic-control-module.h"
#include "../model/topology-builder.h"
#include "../model/cbfc-queue-disc.h"
#include "../model/cbfc-credit-manager.h"
#include "../model/flowstar-mid.h"
#include "../application/basic-traffic-sender.h"
#include "../application/basic-traffic-receiver.h"
#include "../application/cbfc-traffic-sender.h"
#include "../application/cbfc-traffic-receiver.h"
#include "../utils/metrics-collector.h"
#include <fstream>
#include <string>

using namespace ns3;
NS_LOG_COMPONENT_DEFINE ("Phase3CbfcComparison");

static void BuildTopology (TopologyBuilder& topo,
                           NodeContainer& hosts,
                           NodeContainer& switches,
                           const std::string& senderRate,
                           const std::string& bottleneck,
                           const std::string& delay)
{
  hosts    = topo.CreateHosts (4);   // 0,1,2=senders 3=receiver
  switches = topo.CreateSwitches (2);
  topo.InstallInternetStack (hosts);
  topo.InstallInternetStack (switches);

  auto s1   = topo.ConnectHostToSwitch (hosts.Get(0), switches.Get(0), senderRate, delay);
  auto s2   = topo.ConnectHostToSwitch (hosts.Get(1), switches.Get(0), senderRate, delay);
  auto s3   = topo.ConnectHostToSwitch (hosts.Get(2), switches.Get(0), senderRate, delay);
  auto d1   = topo.ConnectHostToSwitch (hosts.Get(3), switches.Get(1), senderRate, delay);
  auto bnk  = topo.ConnectSwitchToSwitch (switches.Get(0), switches.Get(1), bottleneck, delay);

  topo.AssignIpv4Addresses (s1,  "10.1.1.0", "255.255.255.0");
  topo.AssignIpv4Addresses (s2,  "10.1.2.0", "255.255.255.0");
  topo.AssignIpv4Addresses (s3,  "10.1.3.0", "255.255.255.0");
  topo.AssignIpv4Addresses (d1,  "10.2.1.0", "255.255.255.0");
  topo.AssignIpv4Addresses (bnk, "10.3.1.0", "255.255.255.0");
  topo.PopulateRoutingTables ();
}

int main (int argc, char *argv[])
{
  std::string mode        = "phase3";
  std::string senderRate  = "10Gbps";
  std::string bottleneck  = "1Gbps";
  std::string delay       = "1us";
  uint32_t packetSize     = 1000;
  uint64_t messageSize    = 1000000;  // 1 MB per flow
  uint32_t numQueues      = 4;
  uint32_t initialCredit  = 65536;
  uint32_t fccl           = 65536;

  CommandLine cmd;
  cmd.AddValue ("mode",         "phase2 or phase3", mode);
  cmd.AddValue ("messageSize",  "Message size",     messageSize);
  cmd.AddValue ("numQueues",    "Physical queues",  numQueues);
  cmd.Parse (argc, argv);

  TopologyBuilder topo;
  NodeContainer hosts, switches;
  BuildTopology (topo, hosts, switches, senderRate, bottleneck, delay);

  uint16_t port    = 9;
  Time     tStart  = Seconds (0.1);
  Time     tStop   = Seconds (3.0);

  Ptr<Ipv4> rxIpv4   = hosts.Get(3)->GetObject<Ipv4>();
  Ipv4Address destAddr = rxIpv4->GetAddress(1,0).GetLocal();

  if (mode == "phase2")
    {
      Ptr<BasicTrafficReceiver> rx = CreateObject<BasicTrafficReceiver>();
      rx->Setup (port);
      hosts.Get(3)->AddApplication(rx);
      rx->SetStartTime(Seconds(0)); rx->SetStopTime(tStop);

      for (uint32_t i = 0; i < 3; ++i)
        {
          Ptr<BasicTrafficSender> tx = CreateObject<BasicTrafficSender>();
          tx->SetupMessage(i+1, InetSocketAddress(destAddr,port), packetSize, messageSize, DataRate(senderRate));
          hosts.Get(i)->AddApplication(tx);
          tx->SetStartTime(tStart); tx->SetStopTime(tStop);
        }
    }
  else  // phase3
    {
      // Install CbfcQueueDisc on SW1 bottleneck port
      NetDeviceContainer sw1sw2;
      // We need the bottleneck device — get it from the switch node
      Ptr<Node> sw1 = switches.Get(0);
      // The last device installed on sw1 is the SW1→SW2 link (sw1sw2.Get(0))
      // We query devices directly
      uint32_t nDev = sw1->GetNDevices();
      Ptr<NetDevice> bnkDev = sw1->GetDevice(nDev - 1);  // last installed

      NetDeviceContainer single; single.Add(bnkDev);
      TrafficControlHelper tchRemove;
      tchRemove.Uninstall(single);
      TrafficControlHelper tch;
      tch.SetRootQueueDisc("ns3::CbfcQueueDisc",
                           "NumQueues", UintegerValue(numQueues),
                           "QueueSize", UintegerValue(1000));
      tch.Install(single);

      Ptr<CbfcCreditManager> cm = CreateObject<CbfcCreditManager>();

      Ptr<CbfcTrafficReceiver> rx = CreateObject<CbfcTrafficReceiver>();
      rx->Setup(port);
      hosts.Get(3)->AddApplication(rx);
      rx->SetStartTime(Seconds(0)); rx->SetStopTime(tStop);

      for (uint32_t i = 0; i < 3; ++i)
        {
          uint16_t creditPort = 6000 + i;
          FlowStarMid mid(i, 3, i+1, 1);
          cm->InitializeFlow(mid, initialCredit, fccl);

          Ptr<Ipv4> sIpv4 = hosts.Get(i)->GetObject<Ipv4>();
          Ipv4Address sAddr = sIpv4->GetAddress(1,0).GetLocal();
          rx->RegisterSenderCredit(mid, InetSocketAddress(sAddr, creditPort));

          Ptr<CbfcTrafficSender> tx = CreateObject<CbfcTrafficSender>();
          tx->SetCreditManager(cm); tx->SetCreditListenPort(creditPort);
          tx->Setup(mid, InetSocketAddress(destAddr,port),
                    InetSocketAddress(destAddr,port),
                    packetSize, messageSize, DataRate(senderRate));
          hosts.Get(i)->AddApplication(tx);
          tx->SetStartTime(tStart); tx->SetStopTime(tStop);
        }
    }

  MetricsCollector::GetInstance().Reset();
  MetricsCollector::GetInstance().InstallTxRxDropHooks();
  MetricsCollector::GetInstance().InstallQueueHooks();

  Simulator::Stop(tStop);
  Simulator::Run();
  Simulator::Destroy();

  std::cout << "=== Phase 3 CBFC Comparison [mode=" << mode << "] ===" << std::endl;
  std::cout << "Tx:         " << MetricsCollector::GetInstance().GetTotalTxPackets()    << std::endl;
  std::cout << "Rx:         " << MetricsCollector::GetInstance().GetTotalRxPackets()    << std::endl;
  std::cout << "Drops:      " << MetricsCollector::GetInstance().GetTotalDroppedPackets()<< std::endl;
  std::cout << "Max Queue:  " << MetricsCollector::GetInstance().GetMaxQueueOccupancy() << std::endl;

  return 0;
}
