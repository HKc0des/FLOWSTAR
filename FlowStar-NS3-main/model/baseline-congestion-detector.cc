#include "baseline-congestion-detector.h"
#include "baseline-control-message.h"
#include "ns3/log.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("BaselineCongestionDetector");

NS_OBJECT_ENSURE_REGISTERED (BaselineCongestionDetector);

TypeId
BaselineCongestionDetector::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::BaselineCongestionDetector")
    .SetParent<Object> ()
    .SetGroupName("FlowStar")
    .AddConstructor<BaselineCongestionDetector> ()
  ;
  return tid;
}

BaselineCongestionDetector::BaselineCongestionDetector ()
  : m_congestionThreshold (500),
    m_congested (false)
{
}

BaselineCongestionDetector::~BaselineCongestionDetector ()
{
}

void
BaselineCongestionDetector::SetCongestionThreshold (uint32_t threshold)
{
  m_congestionThreshold = threshold;
}

uint32_t
BaselineCongestionDetector::GetCongestionThreshold (void) const
{
  return m_congestionThreshold;
}

void
BaselineCongestionDetector::OnEnqueue (uint32_t currentOccupancy, Ptr<Packet> packet)
{
  if (currentOccupancy >= m_congestionThreshold)
    {
      m_congested = true;
      // Mark the packet with ECN
      EcnTag ecnTag (true);
      packet->AddPacketTag (ecnTag);
      NS_LOG_INFO ("Congestion detected (occupancy=" << currentOccupancy
                   << " >= threshold=" << m_congestionThreshold
                   << "), ECN marking packet");
    }
  else
    {
      m_congested = false;
    }
}

bool
BaselineCongestionDetector::IsCongested (void) const
{
  return m_congested;
}

} // namespace ns3
