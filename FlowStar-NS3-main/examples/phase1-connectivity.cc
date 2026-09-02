#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/applications-module.h"
#include "../model/topology-builder.h"
#include "../application/basic-traffic-sender.h"
#include "../application/basic-traffic-receiver.h"
#include "../utils/metrics-collector.h"
#include "../utils/csv-writer.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("Phase1Connectivity");

int main (int argc, char *argv[])
{
  std::string dataRate = "10Gbps";
  std::string delay = "1us";
  uint32_t packetSize = 1000;
  uint64_t messageSize = 1000000; // 1 MB
  std::string outputDir = ".";

  CommandLine cmd;
  cmd.AddValue ("dataRate", "Link Data Rate", dataRate);
  cmd.AddValue ("delay", "Link Delay", delay);
  cmd.AddValue ("packetSize", "Packet Size in bytes", packetSize);
  cmd.AddValue ("messageSize", "Total Message Size in bytes", messageSize);
  cmd.AddValue ("outputDir", "Output Directory", outputDir);
  cmd.Parse (argc, argv);

  TopologyBuilder topo;

  NodeContainer hosts = topo.CreateHosts (2);
  NodeContainer switches = topo.CreateSwitches (1);

  topo.InstallInternetStack (hosts);
  topo.InstallInternetStack (switches);

  NetDeviceContainer h0ToSw = topo.ConnectHostToSwitch (hosts.Get (0), switches.Get (0), dataRate, delay);
  NetDeviceContainer h1ToSw = topo.ConnectHostToSwitch (hosts.Get (1), switches.Get (0), dataRate, delay);

  topo.AssignIpv4Addresses (h0ToSw, "10.1.1.0", "255.255.255.0");
  topo.AssignIpv4Addresses (h1ToSw, "10.1.2.0", "255.255.255.0");

  topo.PopulateRoutingTables ();

  uint16_t port = 9;

  Ptr<BasicTrafficReceiver> receiver = CreateObject<BasicTrafficReceiver> ();
  receiver->Setup (port);
  hosts.Get (1)->AddApplication (receiver);
  receiver->SetStartTime (Seconds (0.0));
  receiver->SetStopTime (Seconds (2.0));

  Ptr<Ipv4> ipv4 = hosts.Get (1)->GetObject<Ipv4> ();
  Ipv4Address destAddress = ipv4->GetAddress (1, 0).GetLocal ();

  Ptr<BasicTrafficSender> sender = CreateObject<BasicTrafficSender> ();
  sender->SetupMessage (1 /* flowId */, InetSocketAddress (destAddress, port), packetSize, messageSize, DataRate (dataRate));
  hosts.Get (0)->AddApplication (sender);
  sender->SetStartTime (Seconds (0.1));
  sender->SetStopTime (Seconds (2.0));

  MetricsCollector::GetInstance().InstallTxRxDropHooks();
  MetricsCollector::GetInstance().InstallQueueHooks();

  Simulator::Stop (Seconds (2.0));
  Simulator::Run ();
  Simulator::Destroy ();

  std::cout << "Experiment 1: Basic Connectivity Complete.\n";
  std::cout << "Max Queue Occupancy: " << MetricsCollector::GetInstance().GetMaxQueueOccupancy() << " packets.\n";
  std::cout << "Packets Sent: " << MetricsCollector::GetInstance().GetTotalTxPackets() << "\n";
  std::cout << "Packets Received: " << MetricsCollector::GetInstance().GetTotalRxPackets() << "\n";
  std::cout << "Packets Dropped: " << MetricsCollector::GetInstance().GetTotalDroppedPackets() << "\n";

  return 0;
}
