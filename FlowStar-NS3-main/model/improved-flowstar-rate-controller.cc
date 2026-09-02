#include "improved-flowstar-rate-controller.h"
#include "../utils/metrics-collector.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("ImprovedFlowStarRateController");
NS_OBJECT_ENSURE_REGISTERED (ImprovedFlowStarRateController);

TypeId
ImprovedFlowStarRateController::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::ImprovedFlowStarRateController")
    .SetParent<FlowStarRateController> ()
    .SetGroupName ("FlowStar")
    .AddConstructor<ImprovedFlowStarRateController> ()
  ;
  return tid;
}

ImprovedFlowStarRateController::ImprovedFlowStarRateController ()
{
}

ImprovedFlowStarRateController::~ImprovedFlowStarRateController ()
{
}

void
ImprovedFlowStarRateController::OnBecn (FlowStarMid mid, uint32_t queueOccupancy)
{
  // Estimate severity based on queue occupancy (assuming max queue is 100)
  // Higher occupancy -> higher severity (max 1.0)
  double severity = std::min(1.0, (double)queueOccupancy / 100.0);
  m_flowCongestionSeverity[mid] = severity;

  NS_LOG_INFO ("Improved Rate Controller: BECN for MID " << mid.srcFlowId << "->" << mid.dstFlowId 
               << ", occupancy=" << queueOccupancy << ", severity=" << severity);

  // Call the base class to actually decrease the rate and reset to STEADY
  FlowStarRateController::OnBecn (mid, queueOccupancy);
}

void
ImprovedFlowStarRateController::DoRecoveryStep (FlowStarMid mid)
{
  // By default, FlowStarRateController uses fixed AIMD/MIMD parameters.
  // We will intercept the recovery step and modulate the parameters based on severity.
  auto it = m_flowStates.find (mid);
  if (it == m_flowStates.end ()) return;

  double severity = m_flowCongestionSeverity[mid];
  // Decay severity over time if no BECNs are received
  m_flowCongestionSeverity[mid] = severity * 0.9;
  
  // If severity is high, we scale down the recovery step
  // If severity is low, we recover faster
  double scaleFactor = 1.0 - (severity * 0.8); // 0.2x to 1.0x speed

  uint64_t currentBps = it->second.currentRate.GetBitRate ();
  uint64_t lineRateBps = it->second.lineRate.GetBitRate ();
  
  if (it->second.state == SLOW_RECOVERY)
    {
      // Additive step
      // Base class uses m_slowStep (e.g. 50Mbps)
      uint64_t stepBps = static_cast<uint64_t>(50000000 * scaleFactor);
      uint64_t newRateBps = currentBps + stepBps;
      
      if (newRateBps > lineRateBps)
        newRateBps = lineRateBps;
        
      it->second.currentRate = DataRate (newRateBps);
      NS_LOG_INFO ("I4 Slow Recovery: severity=" << severity << ", step=" << stepBps << " bps");
    }
  else if (it->second.state == AGGRESSIVE_RECOVERY)
    {
      // Proportional step
      // Base class uses 1.1x multiplier. We scale it: 1.02x to 1.1x
      double multiplier = 1.0 + (0.1 * scaleFactor);
      uint64_t newRateBps = static_cast<uint64_t>(currentBps * multiplier);
      
      if (newRateBps > lineRateBps)
        newRateBps = lineRateBps;
        
      it->second.currentRate = DataRate (newRateBps);
      NS_LOG_INFO ("I4 Aggressive Recovery: severity=" << severity << ", mult=" << multiplier);
    }
    
  MetricsCollector::GetInstance().RateChange(mid.srcFlowId, it->second.currentRate);

  // Still need to reschedule if not at line rate
  if (it->second.currentRate < it->second.lineRate)
    {
      it->second.recoveryEvent = Simulator::Schedule (m_recoveryInterval, &ImprovedFlowStarRateController::DoRecoveryStep, this, mid);
    }
  else
    {
      it->second.state = STEADY;
    }
}

} // namespace ns3
