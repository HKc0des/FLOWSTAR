#ifndef FLOWSTAR_PATH_MANAGER_H
#define FLOWSTAR_PATH_MANAGER_H

#include "ns3/object.h"
#include "ns3/ipv4-address.h"
#include "ns3/nstime.h"
#include <map>
#include <vector>

namespace ns3 {

/**
 * \brief I3: Adaptive Multipath Routing Flow-Level Path Manager
 *
 * Maintains a set of possible destination IP addresses (representing 
 * distinct network paths) for a given logical destination node.
 * Evaluates congestion scores for paths and selects the least-congested 
 * path at flow initialization.
 */
class FlowStarPathManager : public Object
{
public:
  static TypeId GetTypeId (void);

  FlowStarPathManager ();
  ~FlowStarPathManager () override;

  /// Add a path (represented by an IP address) to a logical destination node
  void AddPath (uint32_t dstNodeId, Ipv4Address pathIp);

  /// Called when a CNP/BECN provides congestion feedback for a specific path
  void UpdatePathCongestion (Ipv4Address pathIp, double congestionScore);

  /// Select the least congested path for a new flow to dstNodeId
  Ipv4Address GetBestPath (uint32_t dstNodeId);

private:
  struct PathInfo {
    Ipv4Address ip;
    double congestionScore;
    Time lastUpdateTime;
  };

  std::map<uint32_t, std::vector<PathInfo>> m_nodePaths;
};

} // namespace ns3

#endif /* FLOWSTAR_PATH_MANAGER_H */
