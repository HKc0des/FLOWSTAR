#include "topology-builder.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/string.h"

namespace ns3 {

TopologyBuilder::TopologyBuilder ()
{
}

NodeContainer
TopologyBuilder::CreateHosts (uint32_t n)
{
  NodeContainer hosts;
  hosts.Create (n);
  return hosts;
}

NodeContainer
TopologyBuilder::CreateSwitches (uint32_t n)
{
  NodeContainer switches;
  switches.Create (n);
  return switches;
}

void
TopologyBuilder::InstallInternetStack (NodeContainer hosts)
{
  m_internet.Install (hosts);
}

PointToPointHelper
TopologyBuilder::GetP2pHelper (std::string dataRate, std::string delay)
{
  PointToPointHelper p2p;
  p2p.SetDeviceAttribute ("DataRate", StringValue (dataRate));
  p2p.SetChannelAttribute ("Delay", StringValue (delay));
  // Explicitly set one shared DropTail queue per output link
  p2p.SetQueue ("ns3::DropTailQueue", "MaxSize", StringValue ("1000p"));
  return p2p;
}

NetDeviceContainer
TopologyBuilder::ConnectHostToSwitch (Ptr<Node> host, Ptr<Node> sw, std::string dataRate, std::string delay)
{
  PointToPointHelper p2p = GetP2pHelper (dataRate, delay);
  return p2p.Install (host, sw);
}

NetDeviceContainer
TopologyBuilder::ConnectSwitchToSwitch (Ptr<Node> sw1, Ptr<Node> sw2, std::string dataRate, std::string delay)
{
  PointToPointHelper p2p = GetP2pHelper (dataRate, delay);
  return p2p.Install (sw1, sw2);
}



void
TopologyBuilder::AssignIpv4Addresses (NetDeviceContainer hostDevices, const char* network, const char* mask)
{
  Ipv4AddressHelper ipv4;
  ipv4.SetBase (Ipv4Address (network), Ipv4Mask (mask));
  ipv4.Assign (hostDevices);
}

void
TopologyBuilder::PopulateRoutingTables ()
{
  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();
}

} // namespace ns3
