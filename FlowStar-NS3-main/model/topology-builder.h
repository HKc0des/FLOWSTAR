#ifndef TOPOLOGY_BUILDER_H
#define TOPOLOGY_BUILDER_H

#include "ns3/node-container.h"
#include "ns3/net-device-container.h"
#include "ns3/point-to-point-helper.h"
#include "ns3/internet-stack-helper.h"
#include "ns3/ipv4-address-helper.h"
#include <string>
#include <vector>

namespace ns3 {

class TopologyBuilder {
public:
  TopologyBuilder ();

  NodeContainer CreateHosts (uint32_t n);
  NodeContainer CreateSwitches (uint32_t n);

  void InstallInternetStack (NodeContainer hosts);

  // Connect host to switch, returning the host's NetDevice and switch's NetDevice
  NetDeviceContainer ConnectHostToSwitch (Ptr<Node> host, Ptr<Node> sw, std::string dataRate, std::string delay);

  // Connect switch to switch
  NetDeviceContainer ConnectSwitchToSwitch (Ptr<Node> sw1, Ptr<Node> sw2, std::string dataRate, std::string delay);

  // Finalize is not needed for L3 routers
  void FinalizeSwitch (Ptr<Node> sw, NetDeviceContainer switchPorts) {}

  void AssignIpv4Addresses (NetDeviceContainer hostDevices, const char* network, const char* mask);

  void PopulateRoutingTables ();

private:
  PointToPointHelper GetP2pHelper (std::string dataRate, std::string delay);

  InternetStackHelper m_internet;
};

} // namespace ns3

#endif /* TOPOLOGY_BUILDER_H */
