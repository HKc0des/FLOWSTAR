#include "flowstar-rate-controller.h"
#include "ns3/simulator.h"
#include "ns3/log.h"
#include "ns3/double.h"
#include "../utils/metrics-collector.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("FlowStarRateController");

NS_OBJECT_ENSURE_REGISTERED (FlowStarRateController);

TypeId
FlowStarRateController::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::FlowStarRateController")
    .SetParent<Object> ()
    .SetGroupName ("FlowStar")
    .AddConstructor<FlowStarRateController> ()
    .AddAttribute ("MinRate",
                   "Minimum transmission rate",
                   DataRateValue (DataRate ("100Mbps")),
                   MakeDataRateAccessor (&FlowStarRateController::m_minRate),
                   MakeDataRateChecker ())
    .AddAttribute ("CnpDecreaseFactor",
                   "Factor applied to receiving rate on CNP (rate = receivingRate * factor)",
                   DoubleValue (1.0),
                   MakeDoubleAccessor (&FlowStarRateController::m_cnpDecreaseFactor),
                   MakeDoubleChecker<double> ())
    .AddAttribute ("BecnDecreaseFactor",
                   "Multiplicative decrease factor applied on BECN",
                   DoubleValue (0.5),
                   MakeDoubleAccessor (&FlowStarRateController::m_becnDecreaseFactor),
                   MakeDoubleChecker<double> ())
    .AddAttribute ("SlowStep",
                   "Additive increase step during SLOW_RECOVERY",
                   DataRateValue (DataRate ("100Mbps")),
                   MakeDataRateAccessor (&FlowStarRateController::m_slowStep),
                   MakeDataRateChecker ())
    .AddAttribute ("AggressiveFactor",
                   "Proportional increase factor during AGGRESSIVE_RECOVERY",
                   DoubleValue (0.1),
                   MakeDoubleAccessor (&FlowStarRateController::m_aggressiveFactor),
                   MakeDoubleChecker<double> ())
    .AddAttribute ("TransitionThresholdFraction",
                   "Fraction of lineRate to transition from SLOW to AGGRESSIVE",
                   DoubleValue (0.5),
                   MakeDoubleAccessor (&FlowStarRateController::m_transitionThresholdFraction),
                   MakeDoubleChecker<double> ())
    .AddAttribute ("RecoveryInterval",
                   "Time between recovery steps",
                   TimeValue (MicroSeconds (50)),
                   MakeTimeAccessor (&FlowStarRateController::m_recoveryInterval),
                   MakeTimeChecker ())
    .AddAttribute ("CnpMinInterval",
                   "Minimum time between responding to CNPs",
                   TimeValue (MicroSeconds (10)),
                   MakeTimeAccessor (&FlowStarRateController::m_cnpMinInterval),
                   MakeTimeChecker ())
  ;
  return tid;
}

FlowStarRateController::FlowStarRateController ()
{
}

FlowStarRateController::~FlowStarRateController ()
{
}

void
FlowStarRateController::InitializeFlow (FlowStarMid mid, DataRate lineRate)
{
  FlowState state;
  state.lineRate = lineRate;
  state.currentRate = lineRate; // Start at line rate
  state.state = STEADY;
  m_flowStates[mid] = state;
}

DataRate
FlowStarRateController::GetCurrentRate (FlowStarMid mid)
{
  if (m_flowStates.find (mid) != m_flowStates.end ())
    {
      return m_flowStates[mid].currentRate;
    }
  return m_minRate;
}

void
FlowStarRateController::OnCnp (FlowStarMid mid, uint64_t receivingRateBps)
{
  auto it = m_flowStates.find (mid);
  if (it == m_flowStates.end ())
    return;

  FlowState& state = it->second;
  Time now = Simulator::Now ();

  if (state.hasReceivedCnp)
    {
       if ((now - state.lastCnpTime) < m_cnpMinInterval)
         {
           return; // Rate limit CNP processing
         }
    }
  state.hasReceivedCnp = true;
  state.lastCnpTime = now;

  // Rate decrease logic
  uint64_t newRateBps = static_cast<uint64_t>(receivingRateBps * m_cnpDecreaseFactor);
  DataRate newRate (newRateBps);

  if (newRate < m_minRate)
    newRate = m_minRate;

  NS_LOG_INFO ("CNP received for MID " << mid.srcFlowId << "->" << mid.dstFlowId
               << " RecvRate=" << receivingRateBps << " bps, NewRate=" << newRate);

  state.currentRate = newRate;
  state.state = SLOW_RECOVERY;
  MetricsCollector::GetInstance().RateChange(mid.srcFlowId, state.currentRate);

  // Restart recovery timer
  if (!state.recoveryEvent.IsExpired ())
    {
      state.recoveryEvent.Cancel ();
    }
  state.recoveryEvent = Simulator::Schedule (m_recoveryInterval, &FlowStarRateController::DoRecoveryStep, this, mid);
}

void
FlowStarRateController::OnBecn (FlowStarMid mid, uint32_t queueOccupancy)
{
  auto it = m_flowStates.find (mid);
  if (it == m_flowStates.end ()) return;

  // Decrease rate on BECN
  DataRate newRate (it->second.currentRate.GetBitRate () * m_becnDecreaseFactor);
  if (newRate < m_minRate)
    {
      newRate = m_minRate;
    }
  it->second.currentRate = newRate;
  
  // Transition to STEADY, suspending recovery until the next interval
  it->second.state = STEADY;
  MetricsCollector::GetInstance().RateChange(mid.srcFlowId, newRate);

  NS_LOG_INFO ("BECN received for MID " << mid.srcFlowId << "->" << mid.dstFlowId
               << " NewRate=" << newRate);
  // We'll keep it simple: rate drops, recovery continues if active.
}

void
FlowStarRateController::RecordBecnPath (FlowStarMid mid, uint32_t switchId)
{
  // ONLY state recording. NO route changing.
  NS_LOG_INFO ("Recorded BECN path state for MID " << mid.srcFlowId << "->" << mid.dstFlowId
               << " at switch " << switchId);
}

void
FlowStarRateController::DoRecoveryStep (FlowStarMid mid)
{
  auto it = m_flowStates.find (mid);
  if (it == m_flowStates.end ())
    return;

  FlowState& state = it->second;

  if (state.state == STEADY)
    return;

  DataRate transitionThreshold (static_cast<uint64_t>(state.lineRate.GetBitRate () * m_transitionThresholdFraction));

  if (state.state == SLOW_RECOVERY)
    {
      state.currentRate = state.currentRate + m_slowStep;
      if (state.currentRate > transitionThreshold)
        {
          state.state = AGGRESSIVE_RECOVERY;
        }
    }
  else if (state.state == AGGRESSIVE_RECOVERY)
    {
      uint64_t currentBps = state.currentRate.GetBitRate ();
      uint64_t lineBps = state.lineRate.GetBitRate ();
      uint64_t addedBps = static_cast<uint64_t>((lineBps - currentBps) * m_aggressiveFactor);
      state.currentRate = DataRate (currentBps + addedBps);
    }

  MetricsCollector::GetInstance().RateChange(mid.srcFlowId, state.currentRate);

  if (state.currentRate >= state.lineRate)
    {
      state.currentRate = state.lineRate;
      state.state = STEADY;
      return; // Do not reschedule
    }

  state.recoveryEvent = Simulator::Schedule (m_recoveryInterval, &FlowStarRateController::DoRecoveryStep, this, mid);
}

} // namespace ns3
