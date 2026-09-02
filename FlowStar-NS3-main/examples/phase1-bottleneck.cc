#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/applications-module.h"
#include "../model/topology-builder.h"
#include "../application/basic-traffic-sender.h"
#include "../application/basic-traffic-receiver.h"
#include "../utils/metrics-collector.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("Phase1Bottleneck");

int main (int argc, char *argv[])
{
  std::string senderDataRate = "10Gbps";
  std::string bottleneckDataRate = "10Gbps";
  bool useBottleneck = false;
  std::string delay = "1us";
  uint32_t packetSize = 1000;
  uint64_t messageSize = 1000000; // 1 MB per flow

  CommandLine cmd;
  cmd.AddValue ("bottleneck", "Use bottleneck link (1Gbps)", useBottleneck);
  cmd.Parse (argc, argv);

  if (useBottleneck) {
    bottleneckDataRate = "1Gbps";
  }

  TopologyBuilder topo;

  // 3 senders (0,1,2), 1 receiver (3)
  NodeContainer hosts = topo.CreateHosts (4);
  NodeContainer switches = topo.CreateSwitches (2);

  topo.InstallInternetStack (hosts);
  topo.InstallInternetStack (switches);

  // S1(0), S2(1), S3(2) connected to SW1(0)
  NetDeviceContainer s1 = topo.ConnectHostToSwitch (hosts.Get(0), switches.Get(0), senderDataRate, delay);
  NetDeviceContainer s2 = topo.ConnectHostToSwitch (hosts.Get(1), switches.Get(0), senderDataRate, delay);
  NetDeviceContainer s3 = topo.ConnectHostToSwitch (hosts.Get(2), switches.Get(0), senderDataRate, delay);

  // D1(3) connected to SW2(1)
  NetDeviceContainer d1 = topo.ConnectHostToSwitch (hosts.Get(3), switches.Get(1), senderDataRate, delay);

  // Connect SW1 to SW2 (THE BOTTLENECK)
  NetDeviceContainer sw1sw2 = topo.ConnectSwitchToSwitch (switches.Get(0), switches.Get(1), bottleneckDataRate, delay);

  // IP Assignment
  topo.AssignIpv4Addresses (s1, "10.1.1.0", "255.255.255.0");
  topo.AssignIpv4Addresses (s2, "10.1.2.0", "255.255.255.0");
  topo.AssignIpv4Addresses (s3, "10.1.3.0", "255.255.255.0");
  topo.AssignIpv4Addresses (d1, "10.2.1.0", "255.255.255.0");
  topo.AssignIpv4Addresses (sw1sw2, "10.3.1.0", "255.255.255.0");

  topo.PopulateRoutingTables ();

  uint16_t port = 9;
  Ptr<BasicTrafficReceiver> receiver = CreateObject<BasicTrafficReceiver> ();
  receiver->Setup (port);
  hosts.Get (3)->AddApplication (receiver);
  receiver->SetStartTime (Seconds (0.0));
  receiver->SetStopTime (Seconds (2.0));

  Ptr<Ipv4> ipv4 = hosts.Get (3)->GetObject<Ipv4> ();
  Ipv4Address destAddress = ipv4->GetAddress (1, 0).GetLocal ();

  for (uint32_t i = 0; i <= 2; ++i)
    {
      Ptr<BasicTrafficSender> sender = CreateObject<BasicTrafficSender> ();
      sender->SetupMessage (i+1, InetSocketAddress (destAddress, port), packetSize, messageSize, DataRate (senderDataRate));
      hosts.Get (i)->AddApplication (sender);
      sender->SetStartTime (Seconds (0.1)); 
      sender->SetStopTime (Seconds (2.0));
    }

  MetricsCollector::GetInstance().InstallTxRxDropHooks();
  MetricsCollector::GetInstance().InstallQueueHooks();

  Simulator::Stop (Seconds (2.0));
  Simulator::Run ();
  Simulator::Destroy ();

  std::cout << "Experiment 3: Bottleneck Configuration (" << (useBottleneck ? "1Gbps Bottleneck" : "No Bottleneck") << ")\n";
  std::cout << "Max Queue Occupancy: " << MetricsCollector::GetInstance().GetMaxQueueOccupancy() << " packets.\n";
  std::cout << "Packets Dropped: " << MetricsCollector::GetInstance().GetTotalDroppedPackets() << "\n";
  
  return 0;
}
