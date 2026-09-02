#ifndef IMPROVED_FLOWSTAR_RATE_CONTROLLER_H
#define IMPROVED_FLOWSTAR_RATE_CONTROLLER_H

#include "flowstar-rate-controller.h"
#include "ns3/log.h"
#include "ns3/simulator.h"

namespace ns3 {

/**
 * \brief I4: Congestion-aware Rate Recovery
 *
 * Replaces the static recovery parameters of FlowStar with dynamic
 * ones based on the explicit queue occupancy feedback from BECNs.
 */
class ImprovedFlowStarRateController : public FlowStarRateController
{
public:
  static TypeId GetTypeId (void);

  ImprovedFlowStarRateController ();
  virtual ~ImprovedFlowStarRateController ();

  virtual void OnBecn (FlowStarMid mid, uint32_t queueOccupancy) override;
  virtual void DoRecoveryStep (FlowStarMid mid) override;

private:
  // Track severity per flow based on recent BECN occupancy
  std::map<FlowStarMid, double> m_flowCongestionSeverity;
};

} // namespace ns3

#endif /* IMPROVED_FLOWSTAR_RATE_CONTROLLER_H */
