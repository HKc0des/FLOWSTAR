#ifndef BASELINE_CONGESTION_DETECTOR_H
#define BASELINE_CONGESTION_DETECTOR_H

#include "ns3/object.h"
#include "ns3/packet.h"
#include "ns3/nstime.h"

namespace ns3 {

/**
 * \brief Detects congestion at a shared queue and applies ECN marks
 *        to transiting data packets.
 *
 * Installed on a switch node. Monitors per-enqueue occupancy and,
 * when occupancy exceeds the configurable CongestionThreshold,
 * adds an EcnTag to the packet being enqueued.
 */
class BaselineCongestionDetector : public Object {
public:
  static TypeId GetTypeId (void);

  BaselineCongestionDetector ();
  virtual ~BaselineCongestionDetector ();

  void SetCongestionThreshold (uint32_t threshold);
  uint32_t GetCongestionThreshold (void) const;

  // Called on every enqueue event to update occupancy and optionally mark
  void OnEnqueue (uint32_t currentOccupancy, Ptr<Packet> packet);

  bool IsCongested (void) const;

private:
  uint32_t m_congestionThreshold;
  bool m_congested;
};

} // namespace ns3

#endif /* BASELINE_CONGESTION_DETECTOR_H */
