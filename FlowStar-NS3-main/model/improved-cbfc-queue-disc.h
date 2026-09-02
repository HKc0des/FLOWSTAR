#ifndef IMPROVED_CBFC_QUEUE_DISC_H
#define IMPROVED_CBFC_QUEUE_DISC_H

#include "cbfc-queue-disc.h"
#include "ns3/nstime.h"

namespace ns3 {

/**
 * \brief Improved CbfcQueueDisc with Congestion-aware Dynamic Queue Assignment.
 *
 * Implements I1 of Phase 5.
 * Uses a configurable scoring function:
 * score(q) = occupancyWeight * normalizedOccupancy(q) + trendWeight * normalizedTrend(q)
 */
class ImprovedCbfcQueueDisc : public CbfcQueueDisc
{
public:
  static TypeId GetTypeId (void);

  ImprovedCbfcQueueDisc ();
  ~ImprovedCbfcQueueDisc () override;

protected:
  uint32_t AssignQueue (FlowStarMid mid) override;
  
  // To track trend, we need previous occupancies
  void UpdateTrend (void);
  
private:
  double m_occupancyWeight;
  double m_trendWeight;
  
  std::vector<uint32_t> m_prevOccupancy;
  std::vector<double> m_trend;
  Time m_lastTrendUpdate;
};

} // namespace ns3

#endif /* IMPROVED_CBFC_QUEUE_DISC_H */
