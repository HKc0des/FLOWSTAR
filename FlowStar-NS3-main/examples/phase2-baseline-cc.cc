/**
 * Phase 2 Experiment C: Flow Control + Baseline ECN/CNP Congestion Control.
 *
 * Uses SharedQueueFlowControl for lossless operation and
 * BaselineRateController + BaselineCongestionDetector + ECN/CNP
 * for end-to-end congestion control.
 *
 * The switch marks packets with ECN when queue occupancy exceeds
 * the congestion threshold. The receiver detects the mark and sends
 * a CNP back to the sender, which reduces its rate multiplicatively.
 * Rate recovers additively when no CNPs are received.
 */
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/applications-module.h"
#include "../model/topology-builder.h"
#include "../model/shared-queue-flow-control.h"
#include "../model/baseline-congestion-detector.h"
#include "../model/baseline-rate-controller.h"
#include "../application/baseline-traffic-sender.h"
#include "../application/baseline-traffic-receiver.h"
#include "../utils/metrics-collector.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("Phase2BaselineCC");

// Global pointers for the callback-driven congestion detector
Ptr<BaselineCongestionDetector> g_congestionDetector;

void EnqueueWithEcn (std::string context, Ptr<const Packet> p)
{
  // We need a mutable copy to add the ECN tag
  Ptr<Packet> mutablePacket = p->Copy ();
  uint32_t occupancy = MetricsCollector::GetInstance().GetCurrentQueueOccupancy ();
  g_congestionDetector->OnEnqueue (occupancy, mutablePacket);
}

int main (int argc, char *argv[])
{
  std::string senderDataRate = "10Gbps";
  std::string bottleneckDataRate = "1Gbps";
  std::string delay = "1us";
  uint32_t packetSize = 1000;
  uint64_t messageSize = 1000000; // 1 MB per flow
  uint32_t pauseThreshold = 800;
  uint32_t resumeThreshold = 500;
  uint32_t congestionThreshold = 400;
  double mdFactor = 0.5;
  std::string aiStep = "500Mbps";
  std::string recoveryInterval = "100us";
  std::string minRate = "100Mbps";

  CommandLine cmd;
  cmd.AddValue ("senderRate", "Sender link rate", senderDataRate);
  cmd.AddValue ("bottleneckRate", "Bottleneck link rate", bottleneckDataRate);
  cmd.AddValue ("messageSize", "Message size in bytes per flow", messageSize);
  cmd.AddValue ("pauseThreshold", "Queue pause threshold (packets)", pauseThreshold);
  cmd.AddValue ("resumeThreshold", "Queue resume threshold (packets)", resumeThreshold);
  cmd.AddValue ("congestionThreshold", "Queue congestion marking threshold (packets)", congestionThreshold);
  cmd.AddValue ("mdFactor", "Multiplicative decrease factor", mdFactor);
  cmd.AddValue ("aiStep", "Additive increase step", aiStep);
  cmd.AddValue ("recoveryInterval", "Recovery interval", recoveryInterval);
  cmd.AddValue ("minRate", "Minimum sender rate", minRate);
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

  // Create congestion detector (on the switch)
  g_congestionDetector = CreateObject<BaselineCongestionDetector> ();
  g_congestionDetector->SetCongestionThreshold (congestionThreshold);

  // Receiver (CC enabled)
  uint16_t dataPort = 9;
  uint16_t cnpPortBase = 5000;
  Ptr<BaselineTrafficReceiver> receiver = CreateObject<BaselineTrafficReceiver> ();
  receiver->Setup (dataPort, cnpPortBase);
  receiver->SetCcEnabled (true);
  hosts.Get (3)->AddApplication (receiver);
  receiver->SetStartTime (Seconds (0.0));
  receiver->SetStopTime (Seconds (2.0));

  Ptr<Ipv4> ipv4Dest = hosts.Get (3)->GetObject<Ipv4> ();
  Ipv4Address destAddress = ipv4Dest->GetAddress (1, 0).GetLocal ();

  for (uint32_t i = 0; i <= 2; ++i)
    {
      // Per-sender rate controller
      Ptr<BaselineRateController> rateController = CreateObject<BaselineRateController> ();
      rateController->SetInitialRate (DataRate (senderDataRate));
      rateController->SetMinRate (DataRate (minRate));
      rateController->SetMultiplicativeDecreaseFactor (mdFactor);
      rateController->SetAdditiveIncreaseStep (DataRate (aiStep));
      rateController->SetRecoveryInterval (Time (recoveryInterval));

      uint16_t senderCnpPort = cnpPortBase + i + 1;

      Ptr<BaselineTrafficSender> sender = CreateObject<BaselineTrafficSender> ();
      sender->SetFlowController (flowControl);
      sender->SetRateController (rateController);
      sender->SetCnpPort (senderCnpPort);
      sender->Setup (i+1, InetSocketAddress (destAddress, dataPort), packetSize, messageSize, DataRate (senderDataRate));
      hosts.Get (i)->AddApplication (sender);
      sender->SetStartTime (Seconds (0.1));
      sender->SetStopTime (Seconds (2.0));

      // Register sender address with receiver for CNP delivery
      Ptr<Ipv4> ipv4Sender = hosts.Get (i)->GetObject<Ipv4> ();
      Ipv4Address senderAddr = ipv4Sender->GetAddress (1, 0).GetLocal ();
      receiver->RegisterSender (i+1, InetSocketAddress (senderAddr, senderCnpPort));
    }

  MetricsCollector::GetInstance().Reset();
  MetricsCollector::GetInstance().InstallTxRxDropHooks();
  MetricsCollector::GetInstance().InstallQueueHooks();

  // Schedule periodic flow-control updates from queue state
  for (double t = 0.1; t < 2.0; t += 0.000010)
    {
      Simulator::Schedule (Seconds(t), [&flowControl]() {
        flowControl->UpdateQueueOccupancy (
          MetricsCollector::GetInstance().GetCurrentQueueOccupancy());
      });
    }

  Simulator::Stop (Seconds (2.0));
  Simulator::Run ();
  Simulator::Destroy ();

  std::cout << "=== Phase 2 Experiment C: Flow Control + Baseline CC ===" << std::endl;
  std::cout << "Packets Sent (Tx): " << MetricsCollector::GetInstance().GetTotalTxPackets() << std::endl;
  std::cout << "Packets Received (Rx): " << MetricsCollector::GetInstance().GetTotalRxPackets() << std::endl;
  std::cout << "Packets Dropped: " << MetricsCollector::GetInstance().GetTotalDroppedPackets() << std::endl;
  std::cout << "Max Queue Occupancy: " << MetricsCollector::GetInstance().GetMaxQueueOccupancy() << " packets" << std::endl;

  return 0;
}
