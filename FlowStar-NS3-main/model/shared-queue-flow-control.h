#ifndef SHARED_QUEUE_FLOW_CONTROL_H
#define SHARED_QUEUE_FLOW_CONTROL_H

#include "ns3/object.h"
#include "ns3/queue-item.h"

namespace ns3 {

class SharedQueueFlowControl : public Object {
public:
  static TypeId GetTypeId (void);

  SharedQueueFlowControl ();
  virtual ~SharedQueueFlowControl ();

  void SetThresholds (uint32_t pauseThreshold, uint32_t resumeThreshold);

  // Called by QueueMonitor when queue changes
  void UpdateQueueOccupancy (uint32_t occupancy);

  // Called by SenderGate
  bool CanTransmit (uint32_t flowId, uint32_t packetSize) const;

private:
  uint32_t m_pauseThreshold;
  uint32_t m_resumeThreshold;
  uint32_t m_currentOccupancy;
  bool m_paused;
};

} // namespace ns3

#endif /* SHARED_QUEUE_FLOW_CONTROL_H */
