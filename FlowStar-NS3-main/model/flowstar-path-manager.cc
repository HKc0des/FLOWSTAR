#include "flowstar-path-manager.h"
#include "ns3/log.h"
#include "ns3/simulator.h"
#include <limits>

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("FlowStarPathManager");
NS_OBJECT_ENSURE_REGISTERED (FlowStarPathManager);

TypeId
FlowStarPathManager::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::FlowStarPathManager")
    .SetParent<Object> ()
    .SetGroupName ("FlowStar")
    .AddConstructor<FlowStarPathManager> ()
  ;
  return tid;
}

FlowStarPathManager::FlowStarPathManager ()
{
}

FlowStarPathManager::~FlowStarPathManager ()
{
}

void
FlowStarPathManager::AddPath (uint32_t dstNodeId, Ipv4Address pathIp)
{
  PathInfo info;
  info.ip = pathIp;
  info.congestionScore = 0.0;
  info.lastUpdateTime = Seconds(0);
  m_nodePaths[dstNodeId].push_back (info);
  
  NS_LOG_INFO ("Added path " << pathIp << " for destination node " << dstNodeId);
}

void
FlowStarPathManager::UpdatePathCongestion (Ipv4Address pathIp, double congestionScore)
{
  // Smooth the score: new_score = 0.8 * old_score + 0.2 * new_score
  for (auto& pair : m_nodePaths)
    {
      for (auto& path : pair.second)
        {
          if (path.ip == pathIp)
            {
              path.congestionScore = 0.8 * path.congestionScore + 0.2 * congestionScore;
              path.lastUpdateTime = Simulator::Now();
              NS_LOG_INFO ("Updated congestion for path " << pathIp << " to " << path.congestionScore);
              return;
            }
        }
    }
}

Ipv4Address
FlowStarPathManager::GetBestPath (uint32_t dstNodeId)
{
  auto it = m_nodePaths.find (dstNodeId);
  if (it == m_nodePaths.end () || it->second.empty ())
    {
      NS_LOG_WARN ("No explicit paths configured for node " << dstNodeId << ". Returning broadcast dummy.");
      return Ipv4Address::GetBroadcast();
    }

  double minScore = std::numeric_limits<double>::max();
  Ipv4Address bestIp = it->second.front().ip;
  Time now = Simulator::Now();

  for (auto& path : it->second)
    {
      // Decay score if it hasn't been updated in a while (e.g. > 1ms)
      if ((now - path.lastUpdateTime).GetSeconds() > 0.001)
        {
          path.congestionScore *= 0.5; // decay
          path.lastUpdateTime = now;
        }

      if (path.congestionScore < minScore)
        {
          minScore = path.congestionScore;
          bestIp = path.ip;
        }
    }

  NS_LOG_INFO ("Selected best path " << bestIp << " for node " << dstNodeId << " (score=" << minScore << ")");
  return bestIp;
}

} // namespace ns3
