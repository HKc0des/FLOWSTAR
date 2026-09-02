#include "shared-queue-flow-control.h"
#include "ns3/log.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("SharedQueueFlowControl");

NS_OBJECT_ENSURE_REGISTERED (SharedQueueFlowControl);

TypeId
SharedQueueFlowControl::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::SharedQueueFlowControl")
    .SetParent<Object> ()
    .SetGroupName("FlowStar")
    .AddConstructor<SharedQueueFlowControl> ()
  ;
  return tid;
}

SharedQueueFlowControl::SharedQueueFlowControl ()
  : m_pauseThreshold (800),
    m_resumeThreshold (500),
    m_currentOccupancy (0),
    m_paused (false)
{
}

SharedQueueFlowControl::~SharedQueueFlowControl ()
{
}

void
SharedQueueFlowControl::SetThresholds (uint32_t pauseThreshold, uint32_t resumeThreshold)
{
  m_pauseThreshold = pauseThreshold;
  m_resumeThreshold = resumeThreshold;
}

void
SharedQueueFlowControl::UpdateQueueOccupancy (uint32_t occupancy)
{
  m_currentOccupancy = occupancy;

  if (!m_paused && m_currentOccupancy >= m_pauseThreshold)
    {
      m_paused = true;
      NS_LOG_INFO ("Queue occupancy " << occupancy << " >= PauseThreshold (" << m_pauseThreshold << "), PAUSING senders");
    }
  else if (m_paused && m_currentOccupancy <= m_resumeThreshold)
    {
      m_paused = false;
      NS_LOG_INFO ("Queue occupancy " << occupancy << " <= ResumeThreshold (" << m_resumeThreshold << "), RESUMING senders");
    }
}

bool
SharedQueueFlowControl::CanTransmit (uint32_t flowId, uint32_t packetSize) const
{
  // For Phase 2 baseline, this is a shared state, independent of flowId.
  return !m_paused;
}

} // namespace ns3
