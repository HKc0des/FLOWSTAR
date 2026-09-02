#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/applications-module.h"
#include "../model/topology-builder.h"
#include "../application/basic-traffic-sender.h"
#include "../application/basic-traffic-receiver.h"
#include "../utils/metrics-collector.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("Phase1MultipleFlows");

int main (int argc, char *argv[])
{
  std::string dataRate = "10Gbps";
  std::string delay = "1us";
  uint32_t packetSize = 1000;
  uint64_t messageSize = 500000; // 500 KB per flow

  CommandLine cmd;
  cmd.Parse (argc, argv);

  TopologyBuilder topo;

  NodeContainer hosts = topo.CreateHosts (6);
  NodeContainer switches = topo.CreateSwitches (2);

  topo.InstallInternetStack (hosts);
  topo.InstallInternetStack (switches);

  // S1(0), S2(1), S3(2) connected to SW1(0)
  NetDeviceContainer s1 = topo.ConnectHostToSwitch (hosts.Get(0), switches.Get(0), dataRate, delay);
  NetDeviceContainer s2 = topo.ConnectHostToSwitch (hosts.Get(1), switches.Get(0), dataRate, delay);
  NetDeviceContainer s3 = topo.ConnectHostToSwitch (hosts.Get(2), switches.Get(0), dataRate, delay);

  // D1(3), D2(4), D3(5) connected to SW2(1)
  NetDeviceContainer d1 = topo.ConnectHostToSwitch (hosts.Get(3), switches.Get(1), dataRate, delay);
  NetDeviceContainer d2 = topo.ConnectHostToSwitch (hosts.Get(4), switches.Get(1), dataRate, delay);
  NetDeviceContainer d3 = topo.ConnectHostToSwitch (hosts.Get(5), switches.Get(1), dataRate, delay);

  // Connect SW1 to SW2
  NetDeviceContainer sw1sw2 = topo.ConnectSwitchToSwitch (switches.Get(0), switches.Get(1), dataRate, delay);

  // IP Assignment
  topo.AssignIpv4Addresses (s1, "10.1.1.0", "255.255.255.0");
  topo.AssignIpv4Addresses (s2, "10.1.2.0", "255.255.255.0");
  topo.AssignIpv4Addresses (s3, "10.1.3.0", "255.255.255.0");
  topo.AssignIpv4Addresses (d1, "10.2.1.0", "255.255.255.0");
  topo.AssignIpv4Addresses (d2, "10.2.2.0", "255.255.255.0");
  topo.AssignIpv4Addresses (d3, "10.2.3.0", "255.255.255.0");
  topo.AssignIpv4Addresses (sw1sw2, "10.3.1.0", "255.255.255.0");

  topo.PopulateRoutingTables ();

  uint16_t port = 9;
  for (uint32_t i = 3; i <= 5; ++i)
    {
      Ptr<BasicTrafficReceiver> receiver = CreateObject<BasicTrafficReceiver> ();
      receiver->Setup (port);
      hosts.Get (i)->AddApplication (receiver);
      receiver->SetStartTime (Seconds (0.0));
      receiver->SetStopTime (Seconds (2.0));
    }

  for (uint32_t i = 0; i <= 2; ++i)
    {
      Ptr<Ipv4> ipv4 = hosts.Get (i+3)->GetObject<Ipv4> ();
      Ipv4Address destAddress = ipv4->GetAddress (1, 0).GetLocal ();

      Ptr<BasicTrafficSender> sender = CreateObject<BasicTrafficSender> ();
      sender->SetupMessage (i+1 /* flowId */, InetSocketAddress (destAddress, port), packetSize, messageSize, DataRate (dataRate));
      hosts.Get (i)->AddApplication (sender);
      sender->SetStartTime (Seconds (0.1 + i*0.01)); // start slightly staggered
      sender->SetStopTime (Seconds (2.0));
    }

  MetricsCollector::GetInstance().InstallTxRxDropHooks();
  MetricsCollector::GetInstance().InstallQueueHooks();

  Simulator::Stop (Seconds (2.0));
  Simulator::Run ();
  Simulator::Destroy ();

  std::cout << "Experiment 2: Multiple Flows Complete.\n";
  std::cout << "Max Queue Occupancy: " << MetricsCollector::GetInstance().GetMaxQueueOccupancy() << " packets.\n";

  return 0;
}
