/**
 * Phase 3 Experiment: Victim-flow scenario.
 *
 * Topology:
 *   Sender A → SW1 → bottleneck → SW2 → Receiver A  (congested flow)
 *   Sender B → SW1 → same link  → SW2 → Receiver B  (victim flow)
 *
 * Config A: Shared queue (Phase 2 no-control baseline)
 *   → A's queue buildup causes HoL blocking for B
 *
 * Config B: Per-flow CBFC (Phase 3)
 *   → Separate queues isolate A's state from B's
 *   → Physical link still shared, so B does not get full line rate when A saturates
 *   → But B's FCT improves due to HoL isolation
 *
 * Run: --mode=shared  or  --mode=cbfc
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
NS_LOG_COMPONENT_DEFINE ("Phase3VictimFlow");

int main (int argc, char *argv[])
{
  std::string mode         = "cbfc";      // "shared" or "cbfc"
  std::string senderRate   = "10Gbps";
  std::string bottleneck   = "1Gbps";
  std::string delay        = "1us";
  uint32_t packetSize      = 1000;
  uint64_t msgSizeA        = 5000000;  // 5 MB (heavy, congested flow)
  uint64_t msgSizeB        = 500000;   // 0.5 MB (victim flow)
  uint32_t numQueues       = 4;
  uint32_t initialCredit   = 65536;
  uint32_t fccl            = 65536;

  CommandLine cmd;
  cmd.AddValue ("mode",       "shared or cbfc",          mode);
  cmd.AddValue ("msgSizeA",   "Message size for flow A", msgSizeA);
  cmd.AddValue ("msgSizeB",   "Message size for flow B", msgSizeB);
  cmd.AddValue ("numQueues",  "Physical queues (CBFC)",  numQueues);
  cmd.Parse (argc, argv);

  TopologyBuilder topo;
  // Hosts: 0=SenderA, 1=SenderB, 2=ReceiverA, 3=ReceiverB
  NodeContainer hosts    = topo.CreateHosts (4);
  NodeContainer switches = topo.CreateSwitches (2);

  topo.InstallInternetStack (hosts);
  topo.InstallInternetStack (switches);

  NetDeviceContainer lnkA  = topo.ConnectHostToSwitch (hosts.Get(0), switches.Get(0), senderRate, delay);
  NetDeviceContainer lnkB  = topo.ConnectHostToSwitch (hosts.Get(1), switches.Get(0), senderRate, delay);
  NetDeviceContainer lnkRA = topo.ConnectHostToSwitch (hosts.Get(2), switches.Get(1), senderRate, delay);
  NetDeviceContainer lnkRB = topo.ConnectHostToSwitch (hosts.Get(3), switches.Get(1), senderRate, delay);
  NetDeviceContainer sw1sw2 = topo.ConnectSwitchToSwitch (switches.Get(0), switches.Get(1), bottleneck, delay);

  topo.AssignIpv4Addresses (lnkA,  "10.1.1.0", "255.255.255.0");
  topo.AssignIpv4Addresses (lnkB,  "10.1.2.0", "255.255.255.0");
  topo.AssignIpv4Addresses (lnkRA, "10.2.1.0", "255.255.255.0");
  topo.AssignIpv4Addresses (lnkRB, "10.2.2.0", "255.255.255.0");
  topo.AssignIpv4Addresses (sw1sw2,"10.3.1.0", "255.255.255.0");

  topo.PopulateRoutingTables ();

  uint16_t portA = 9, portB = 10;
  Ptr<Ipv4> ipA = hosts.Get(2)->GetObject<Ipv4>(); Ipv4Address addrA = ipA->GetAddress(1,0).GetLocal();
  Ptr<Ipv4> ipB = hosts.Get(3)->GetObject<Ipv4>(); Ipv4Address addrB = ipB->GetAddress(1,0).GetLocal();
  Time t0 = Seconds(0.1), tStop = Seconds(3.0);

  if (mode == "shared")
    {
      // ---- Shared queue baseline ----
      Ptr<BasicTrafficReceiver> rxA = CreateObject<BasicTrafficReceiver>(); rxA->Setup(portA);
      hosts.Get(2)->AddApplication(rxA); rxA->SetStartTime(Seconds(0)); rxA->SetStopTime(tStop);

      Ptr<BasicTrafficReceiver> rxB = CreateObject<BasicTrafficReceiver>(); rxB->Setup(portB);
      hosts.Get(3)->AddApplication(rxB); rxB->SetStartTime(Seconds(0)); rxB->SetStopTime(tStop);

      Ptr<BasicTrafficSender> sndA = CreateObject<BasicTrafficSender>();
      sndA->SetupMessage(1, InetSocketAddress(addrA, portA), packetSize, msgSizeA, DataRate(senderRate));
      hosts.Get(0)->AddApplication(sndA); sndA->SetStartTime(t0); sndA->SetStopTime(tStop);

      Ptr<BasicTrafficSender> sndB = CreateObject<BasicTrafficSender>();
      sndB->SetupMessage(2, InetSocketAddress(addrB, portB), packetSize, msgSizeB, DataRate(senderRate));
      hosts.Get(1)->AddApplication(sndB); sndB->SetStartTime(t0); sndB->SetStopTime(tStop);
    }
  else
    {
      // ---- Per-flow CBFC ----
      TrafficControlHelper tchRemove;
      tchRemove.Uninstall(sw1sw2.Get(0));
      TrafficControlHelper tch;
      tch.SetRootQueueDisc("ns3::CbfcQueueDisc",
                           "NumQueues", UintegerValue(numQueues),
                           "QueueSize", UintegerValue(1000));
      QueueDiscContainer qdiscs = tch.Install(sw1sw2.Get(0));

      Ptr<CbfcQueueDisc> disc = DynamicCast<CbfcQueueDisc>(qdiscs.Get(0));

      Ptr<CbfcCreditManager> cm = CreateObject<CbfcCreditManager>();
      FlowStarMid midA(0,2,1,1), midB(1,3,2,1);
      cm->InitializeFlow(midA, initialCredit, fccl);
      cm->InitializeFlow(midB, initialCredit, fccl);

      uint16_t creditA = 6001, creditB = 6002;

      // Receivers
      Ptr<CbfcTrafficReceiver> rxA = CreateObject<CbfcTrafficReceiver>(); rxA->Setup(portA);
      Ptr<Ipv4> sa = hosts.Get(0)->GetObject<Ipv4>();
      rxA->RegisterSenderCredit(midA, InetSocketAddress(sa->GetAddress(1,0).GetLocal(), creditA));
      hosts.Get(2)->AddApplication(rxA); rxA->SetStartTime(Seconds(0)); rxA->SetStopTime(tStop);

      Ptr<CbfcTrafficReceiver> rxB = CreateObject<CbfcTrafficReceiver>(); rxB->Setup(portB);
      Ptr<Ipv4> sb = hosts.Get(1)->GetObject<Ipv4>();
      rxB->RegisterSenderCredit(midB, InetSocketAddress(sb->GetAddress(1,0).GetLocal(), creditB));
      hosts.Get(3)->AddApplication(rxB); rxB->SetStartTime(Seconds(0)); rxB->SetStopTime(tStop);

      // Senders
      Ptr<CbfcTrafficSender> sndA = CreateObject<CbfcTrafficSender>();
      sndA->SetCreditManager(cm); sndA->SetCreditListenPort(creditA);
      sndA->Setup(midA, InetSocketAddress(addrA,portA), InetSocketAddress(addrA,portA),
                  packetSize, msgSizeA, DataRate(senderRate));
      hosts.Get(0)->AddApplication(sndA); sndA->SetStartTime(t0); sndA->SetStopTime(tStop);

      Ptr<CbfcTrafficSender> sndB = CreateObject<CbfcTrafficSender>();
      sndB->SetCreditManager(cm); sndB->SetCreditListenPort(creditB);
      sndB->Setup(midB, InetSocketAddress(addrB,portB), InetSocketAddress(addrB,portB),
                  packetSize, msgSizeB, DataRate(senderRate));
      hosts.Get(1)->AddApplication(sndB); sndB->SetStartTime(t0); sndB->SetStopTime(tStop);
    }

  MetricsCollector::GetInstance().Reset();
  MetricsCollector::GetInstance().InstallTxRxDropHooks();
  MetricsCollector::GetInstance().InstallQueueHooks();

  Simulator::Stop(tStop);
  Simulator::Run();
  Simulator::Destroy();

  std::cout << "=== Phase 3 Victim Flow [mode=" << mode << "] ===" << std::endl;
  std::cout << "Tx: " << MetricsCollector::GetInstance().GetTotalTxPackets() << std::endl;
  std::cout << "Rx: " << MetricsCollector::GetInstance().GetTotalRxPackets() << std::endl;
  std::cout << "Drops: " << MetricsCollector::GetInstance().GetTotalDroppedPackets() << std::endl;
  std::cout << "Max Queue: " << MetricsCollector::GetInstance().GetMaxQueueOccupancy() << std::endl;

  return 0;
}
