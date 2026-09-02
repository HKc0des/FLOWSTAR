#include "flowstar-endpoint-controller.h"
#include "ns3/simulator.h"
#include "ns3/log.h"
#include "ns3/double.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("FlowStarEndpointController");

NS_OBJECT_ENSURE_REGISTERED (FlowStarEndpointController);

TypeId
FlowStarEndpointController::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::FlowStarEndpointController")
    .SetParent<Object> ()
    .SetGroupName ("FlowStar")
    .AddConstructor<FlowStarEndpointController> ()
    .AddAttribute ("MeasurementWindow",
                   "Time window over which to measure receiving rate",
                   TimeValue (MicroSeconds (100)),
                   MakeTimeAccessor (&FlowStarEndpointController::m_measurementWindow),
                   MakeTimeChecker ())
    .AddAttribute ("CnpMinInterval",
                   "Minimum time between consecutive CNPs for the same flow",
                   TimeValue (MicroSeconds (10)),
                   MakeTimeAccessor (&FlowStarEndpointController::m_cnpMinInterval),
                   MakeTimeChecker ())
  ;
  return tid;
}

FlowStarEndpointController::FlowStarEndpointController ()
{
}

FlowStarEndpointController::~FlowStarEndpointController ()
{
}

void
FlowStarEndpointController::OnPacketReceived (FlowStarMid mid, uint32_t packetSize)
{
  Time now = Simulator::Now ();
  UpdateMeasurement (mid, now);
  m_flowStates[mid].bytesInWindow += packetSize;
}

void
FlowStarEndpointController::UpdateMeasurement (FlowStarMid mid, Time now)
{
  auto& state = m_flowStates[mid];

  if (state.windowStart.IsStrictlyPositive () == false && state.windowStart.IsZero ())
    {
      state.windowStart = now;
      return;
    }

  Time elapsed = now - state.windowStart;
  if (elapsed >= m_measurementWindow)
    {
      // Calculate bits per second
      double bits = state.bytesInWindow * 8.0;
      double seconds = elapsed.GetSeconds ();
      if (seconds > 0)
        {
          state.lastMeasuredRateBps = static_cast<uint64_t>(bits / seconds);
        }
      
      // Start new window (we can either slide or tumble; tumbling is simpler for now)
      state.windowStart = now;
      state.bytesInWindow = 0;
    }
}

uint64_t
FlowStarEndpointController::GetReceivingRateBps (FlowStarMid mid)
{
  Time now = Simulator::Now ();
  UpdateMeasurement (mid, now);
  return m_flowStates[mid].lastMeasuredRateBps;
}

bool
FlowStarEndpointController::ShouldGenerateCnp (FlowStarMid mid, uint64_t expectedRateBps, double thresholdFraction)
{
  uint64_t currentRate = GetReceivingRateBps (mid);
  uint64_t threshold = static_cast<uint64_t>(expectedRateBps * thresholdFraction);

  // If rate hasn't dropped, no CNP
  if (currentRate >= threshold)
    {
      return false;
    }

  // If rate dropped, check rate limiting
  Time now = Simulator::Now ();
  auto& state = m_flowStates[mid];
  if (state.lastCnpTime.IsStrictlyPositive () || state.lastCnpTime.IsZero())
    {
       if ((now - state.lastCnpTime) < m_cnpMinInterval)
         {
           return false; // too soon
         }
    }

  return true;
}

void
FlowStarEndpointController::RecordCnpSent (FlowStarMid mid)
{
  m_flowStates[mid].lastCnpTime = Simulator::Now ();
}

} // namespace ns3
