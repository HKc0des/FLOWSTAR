#ifndef FLOWSTAR_ENDPOINT_CONTROLLER_H
#define FLOWSTAR_ENDPOINT_CONTROLLER_H

#include "ns3/object.h"
#include "ns3/nstime.h"
#include "ns3/data-rate.h"
#include "flowstar-mid.h"
#include <map>

namespace ns3 {

/**
 * \brief FlowStar per-flow receiving rate measurement at the endpoint.
 *
 * Maintains a time-window-based byte count to measure the current
 * receiving rate of each flow. Determines when a CNP should be generated
 * based on a threshold and a minimum interval (to avoid CNP storms).
 */
class FlowStarEndpointController : public Object
{
public:
  static TypeId GetTypeId (void);

  FlowStarEndpointController ();
  virtual ~FlowStarEndpointController ();

  /// Called when a packet is received for this flow
  void OnPacketReceived (FlowStarMid mid, uint32_t packetSize);

  /// Get the currently measured receiving rate for this flow
  uint64_t GetReceivingRateBps (FlowStarMid mid);

  /**
   * \brief Checks if a CNP should be generated.
   *
   * \param mid The flow
   * \param expectedRateBps The rate we expect to be receiving at (e.g. line rate or sender rate)
   * \param thresholdFraction The fraction of expectedRateBps below which we trigger CNP
   * \return true if a CNP should be generated now.
   */
  bool ShouldGenerateCnp (FlowStarMid mid, uint64_t expectedRateBps, double thresholdFraction);

  /// Mark that a CNP was just sent, to enforce rate limiting
  void RecordCnpSent (FlowStarMid mid);

private:
  struct FlowState {
    uint64_t bytesInWindow;
    Time     windowStart;
    uint64_t lastMeasuredRateBps;
    Time     lastCnpTime;

    FlowState () : bytesInWindow (0), windowStart (Seconds (0)), lastMeasuredRateBps (0), lastCnpTime (Seconds (0)) {}
  };

  void UpdateMeasurement (FlowStarMid mid, Time now);

  Time m_measurementWindow;
  Time m_cnpMinInterval;

  std::map<FlowStarMid, FlowState> m_flowStates;
};

} // namespace ns3

#endif /* FLOWSTAR_ENDPOINT_CONTROLLER_H */
