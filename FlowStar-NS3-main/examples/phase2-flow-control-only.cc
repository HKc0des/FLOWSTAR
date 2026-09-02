/**
 * Phase 2 Experiment B: Flow Control Only (No Congestion Control).
 *
 * Uses SharedQueueFlowControl to create a lossless network.
 * Senders are paused when the bottleneck queue fills up.
 * No rate control — senders blast at line rate when not paused.
 * Queue should stay high but no packets should be dropped.
 */
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/applications-module.h"
#include "../model/topology-builder.h"
#include "../model/shared-queue-flow-control.h"
#include "../application/baseline-traffic-sender.h"
#include "../application/baseline-traffic-receiver.h"
#include "../utils/metrics-collector.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("Phase2FlowControlOnly");

int main (int argc, char *argv[])
{
  std::string senderDataRate = "10Gbps";
  std::string bottleneckDataRate = "1Gbps";
  std::string delay = "1us";
  uint32_t packetSize = 1000;
  uint64_t messageSize = 1000000; // 1 MB per flow
  uint32_t pauseThreshold = 800;
  uint32_t resumeThreshold = 500;

  CommandLine cmd;
  cmd.AddValue ("senderRate", "Sender link rate", senderDataRate);
  cmd.AddValue ("bottleneckRate", "Bottleneck link rate", bottleneckDataRate);
  cmd.AddValue ("messageSize", "Message size in bytes per flow", messageSize);
  cmd.AddValue ("pauseThreshold", "Queue pause threshold (packets)", pauseThreshold);
  cmd.AddValue ("resumeThreshold", "Queue resume threshold (packets)", resumeThreshold);
  cmd.Parse (argc, argv);

  TopologyBuilder topo;

  NodeContainer hosts = topo.CreateHosts (4);
  NodeContainer switches = topo.CreateSwitches (2);

  topo.InstallInternetStack (hosts);
  topo.InstallInternetStack (switches);

  NetDeviceContainer s1 = topo.ConnectHostToSwitch (hosts.Get(0), switches.Get(0), senderDataRate, delay);
  NetDeviceContainer s2 = topo.ConnectHostToSwitch (hosts.Get(1), switches.Get(0), senderDataRate, delay);
  NetDeviceContainer s3 = topo.ConnectHostToSwitch (hosts.Get(2), switches.Get(0), senderDataRate, delay);
  NetDeviceContainer d1 = topo.ConnectHostToSwitch (hosts.Get(3), switches.Get(1), senderDataRate, delay);
  NetDeviceContainer sw1sw2 = topo.ConnectSwitchToSwitch (switches.Get(0), switches.Get(1), bottleneckDataRate, delay);

  topo.AssignIpv4Addresses (s1, "10.1.1.0", "255.255.255.0");
  topo.AssignIpv4Addresses (s2, "10.1.2.0", "255.255.255.0");
  topo.AssignIpv4Addresses (s3, "10.1.3.0", "255.255.255.0");
  topo.AssignIpv4Addresses (d1, "10.2.1.0", "255.255.255.0");
  topo.AssignIpv4Addresses (sw1sw2, "10.3.1.0", "255.255.255.0");

  topo.PopulateRoutingTables ();

  // Create shared flow controller
  Ptr<SharedQueueFlowControl> flowControl = CreateObject<SharedQueueFlowControl> ();
  flowControl->SetThresholds (pauseThreshold, resumeThreshold);

  // Receiver (no CC)
  uint16_t dataPort = 9;
  uint16_t cnpPort = 5000;
  Ptr<BaselineTrafficReceiver> receiver = CreateObject<BaselineTrafficReceiver> ();
  receiver->Setup (dataPort, cnpPort);
  receiver->SetCcEnabled (false); // No congestion control for this experiment
  hosts.Get (3)->AddApplication (receiver);
  receiver->SetStartTime (Seconds (0.0));
  receiver->SetStopTime (Seconds (2.0));

  Ptr<Ipv4> ipv4 = hosts.Get (3)->GetObject<Ipv4> ();
  Ipv4Address destAddress = ipv4->GetAddress (1, 0).GetLocal ();

  for (uint32_t i = 0; i <= 2; ++i)
    {
      Ptr<BaselineTrafficSender> sender = CreateObject<BaselineTrafficSender> ();
      sender->SetFlowController (flowControl);
      // No rate controller — sender transmits at line rate when not paused
      sender->Setup (i+1, InetSocketAddress (destAddress, dataPort), packetSize, messageSize, DataRate (senderDataRate));
      sender->SetCnpPort (cnpPort + i + 1); // unique CNP port per sender
      hosts.Get (i)->AddApplication (sender);
      sender->SetStartTime (Seconds (0.1));
      sender->SetStopTime (Seconds (2.0));
    }

  // Hook up the flow control to the queue monitoring
  // We use the MetricsCollector's enqueue/dequeue to drive flow-control updates
  MetricsCollector::GetInstance().Reset();
  MetricsCollector::GetInstance().InstallTxRxDropHooks();
  MetricsCollector::GetInstance().InstallQueueHooks();

  // Schedule periodic flow-control updates from queue state
  for (double t = 0.1; t < 2.0; t += 0.000010) // every 10us
    {
      Simulator::Schedule (Seconds(t), [&flowControl]() {
        flowControl->UpdateQueueOccupancy (
          MetricsCollector::GetInstance().GetCurrentQueueOccupancy());
      });
    }

  Simulator::Stop (Seconds (2.0));
  Simulator::Run ();
  Simulator::Destroy ();

  std::cout << "=== Phase 2 Experiment B: Flow Control Only ===" << std::endl;
  std::cout << "Packets Sent (Tx): " << MetricsCollector::GetInstance().GetTotalTxPackets() << std::endl;
  std::cout << "Packets Received (Rx): " << MetricsCollector::GetInstance().GetTotalRxPackets() << std::endl;
  std::cout << "Packets Dropped: " << MetricsCollector::GetInstance().GetTotalDroppedPackets() << std::endl;
  std::cout << "Max Queue Occupancy: " << MetricsCollector::GetInstance().GetMaxQueueOccupancy() << " packets" << std::endl;

  return 0;
}
