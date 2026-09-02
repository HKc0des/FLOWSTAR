/**
 * Phase 4 Experiment: Bottleneck topology
 *
 * Simulates a standard 3-sender bottleneck topology.
 * Mode "flowstar" runs the original FlowStar algorithm from Phase 4.
 * Other modes run previous phase configurations for comparison.
 */
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/traffic-control-module.h"
#include "../model/topology-builder.h"
#include "../model/cbfc-queue-disc.h"
#include "../model/cbfc-credit-manager.h"
#include "../model/flowstar-mid.h"
#include "../model/flowstar-rate-controller.h"
#include "../model/flowstar-endpoint-controller.h"
#include "../application/flowstar-sender.h"
#include "../application/flowstar-receiver.h"
#include "../application/flowstar-switch-agent.h"
#include "../utils/metrics-collector.h"
#include <iostream>

using namespace ns3;
NS_LOG_COMPONENT_DEFINE ("Phase4Bottleneck");

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
  std::string mode        = "flowstar";
  std::string senderRate  = "10Gbps";
  std::string bottleneck  = "1Gbps";
  std::string delay       = "1us";
  uint32_t packetSize     = 1000;
  uint32_t numPackets     = 10000;
  uint32_t numQueues      = 4;
  uint32_t initialCredit  = 65536;
  uint32_t fccl           = 65536;

  CommandLine cmd;
  cmd.AddValue ("mode",       "flowstar, cbfc, or baseline", mode);
  cmd.AddValue ("numPackets", "Packets per flow", numPackets);
  cmd.Parse (argc, argv);

  TopologyBuilder topo;
  NodeContainer hosts, switches;
  BuildTopology (topo, hosts, switches, senderRate, bottleneck, delay);

  uint16_t dataPort = 9;
  Time     tStart   = Seconds (0.1);
  Time     tStop    = Seconds (3.0);

  Ptr<Ipv4> rxIpv4     = hosts.Get(3)->GetObject<Ipv4>();
  Ipv4Address destAddr = rxIpv4->GetAddress(1,0).GetLocal();

  if (mode == "flowstar")
    {
      // 1. QueueDisc on Switch 1 bottleneck port
      Ptr<Node> sw1 = switches.Get(0);
      Ptr<NetDevice> bnkDev = sw1->GetDevice(sw1->GetNDevices() - 1);
      NetDeviceContainer single; single.Add(bnkDev);
      
      TrafficControlHelper tchRemove; tchRemove.Uninstall(single);
      TrafficControlHelper tch;
      tch.SetRootQueueDisc("ns3::CbfcQueueDisc", "NumQueues", UintegerValue(numQueues));
      QueueDiscContainer qdCont = tch.Install(single);

      Ptr<CbfcQueueDisc> qdisc = DynamicCast<CbfcQueueDisc>(qdCont.Get(0));

      // 2. Switch Agent on Switch 1
      Ptr<FlowStarSwitchAgent> swAgent = CreateObject<FlowStarSwitchAgent>();
      swAgent->SetSwitchId(1);
      swAgent->SetQueueDisc(qdisc);
      sw1->AddApplication(swAgent);
      swAgent->SetStartTime(Seconds(0)); swAgent->SetStopTime(tStop);

      // 3. Credit Manager (Shared per host typically, but we do one global for ease here)
      Ptr<CbfcCreditManager> cm = CreateObject<CbfcCreditManager>();

      // 4. Receiver
      Ptr<FlowStarReceiver> rx = CreateObject<FlowStarReceiver>();
      rx->Setup(dataPort);
      hosts.Get(3)->AddApplication(rx);
      rx->SetStartTime(Seconds(0)); rx->SetStopTime(tStop);

      // 5. Senders
      uint64_t lineRateBps = DataRate(senderRate).GetBitRate();

      for (uint32_t i = 0; i < 3; ++i)
        {
          uint16_t creditPort = 6000 + i;
          uint16_t cnpPort    = 7000 + i;
          uint16_t becnPort   = 8000 + i;
          
          FlowStarMid mid(i, 3, i+1, 1);
          cm->InitializeFlow(mid, initialCredit, fccl);

          Ptr<Ipv4> sIpv4 = hosts.Get(i)->GetObject<Ipv4>();
          Ipv4Address sAddr = sIpv4->GetAddress(1,0).GetLocal();
          
          rx->RegisterSenderCredit(mid, InetSocketAddress(sAddr, creditPort));
          rx->RegisterSenderCnp(mid, InetSocketAddress(sAddr, cnpPort), lineRateBps);
          swAgent->RegisterSenderBecn(mid, InetSocketAddress(sAddr, becnPort));

          Ptr<FlowStarRateController> rc = CreateObject<FlowStarRateController>();
          
          Ptr<FlowStarSender> tx = CreateObject<FlowStarSender>();
          tx->RegisterCreditManager(cm);
          tx->RegisterRateController(rc);
          tx->SetListenPorts(creditPort, cnpPort, becnPort);
          tx->Setup(mid, InetSocketAddress(destAddr, dataPort), packetSize, numPackets, DataRate(senderRate));
          
          hosts.Get(i)->AddApplication(tx);
          tx->SetStartTime(tStart); tx->SetStopTime(tStop);
        }
    }
  else
    {
      NS_LOG_ERROR("Mode not fully implemented in this example yet, run phase3-cbfc-comparison for older modes.");
      return 1;
    }

  MetricsCollector::GetInstance().Reset();
  MetricsCollector::GetInstance().InstallTxRxDropHooks();
  MetricsCollector::GetInstance().InstallQueueHooks();

  Simulator::Stop(tStop);
  Simulator::Run();
  Simulator::Destroy();

  std::cout << "=== Phase 4 Bottleneck [mode=" << mode << "] ===" << std::endl;
  std::cout << "Tx:         " << MetricsCollector::GetInstance().GetTotalTxPackets()    << std::endl;
  std::cout << "Rx:         " << MetricsCollector::GetInstance().GetTotalRxPackets()    << std::endl;
  std::cout << "Drops:      " << MetricsCollector::GetInstance().GetTotalDroppedPackets()<< std::endl;
  std::cout << "Max Queue:  " << MetricsCollector::GetInstance().GetMaxQueueOccupancy() << std::endl;

  return 0;
}
