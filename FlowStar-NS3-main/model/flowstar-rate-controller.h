#ifndef FLOWSTAR_RATE_CONTROLLER_H
#define FLOWSTAR_RATE_CONTROLLER_H

#include "ns3/object.h"
#include "ns3/data-rate.h"
#include "ns3/event-id.h"
#include "ns3/nstime.h"
#include "flowstar-mid.h"
#include <map>

namespace ns3 {

/**
 * \brief FlowStar sender-side rate controller.
 *
 * Implements the FlowStar rate decrease and recovery algorithms.
 *
 * States:
 * - STEADY: Transmitting at current rate.
 * - SLOW_RECOVERY: Additive increase per recovery interval.
 * - AGGRESSIVE_RECOVERY: Proportional increase per recovery interval.
 *
 * Rate Decrease on CNP: Uses the paper's exact CNP rate assignment/decrease logic.
 * Rate Decrease on BECN: Switch-notified fabric congestion decrease.
 *
 * NOTE: The parameters below are NS-3 Simulation Parameters used to configure
 * the implementation. They are not to be confused with the original FlowStar
 * algorithm definitions unless explicitly documented.
 */
class FlowStarRateController : public Object
{
public:
  static TypeId GetTypeId (void);

  FlowStarRateController ();
  virtual ~FlowStarRateController ();

  /// Initialize flow state with a line rate
  void InitializeFlow (FlowStarMid mid, DataRate lineRate);

  /// Get the current allowed transmission rate for pacing
  DataRate GetCurrentRate (FlowStarMid mid);

  /**
   * \brief Handle CNP (Endpoint Congestion)
   * \param mid The flow identifier
   * \param receivingRateBps The rate measured by the receiver
   */
  virtual void OnCnp (FlowStarMid mid, uint64_t receivingRateBps);

  /**
   * \brief Handle BECN (Fabric Congestion)
   * \param mid The flow identifier
   * \param queueOccupancy The bottleneck queue occupancy
   */
  virtual void OnBecn (FlowStarMid mid, uint32_t queueOccupancy);

  /// Record BECN path state (No load balancing decisions made here)
  virtual void RecordBecnPath (FlowStarMid mid, uint32_t switchId);

protected:
  enum RecoveryState {
    STEADY = 0,
    SLOW_RECOVERY,
    AGGRESSIVE_RECOVERY
  };

  struct FlowState {
    DataRate      currentRate;
    DataRate      lineRate;
    RecoveryState state;
    EventId       recoveryEvent;
    Time          lastCnpTime;
    bool          hasReceivedCnp;

    FlowState () : currentRate (0), lineRate (0), state (STEADY), lastCnpTime (Seconds (0)), hasReceivedCnp (false) {}
  };

  virtual void DoRecoveryStep (FlowStarMid mid);

protected:
  std::map<FlowStarMid, FlowState> m_flowStates;

  // NS-3 Simulation Parameters
  DataRate m_minRate;
  double   m_cnpDecreaseFactor;  // Multiplier for receivingRate on CNP
  double   m_becnDecreaseFactor; // Multiplier for currentRate on BECN
  DataRate m_slowStep;
  double   m_aggressiveFactor;
  double   m_transitionThresholdFraction;
  Time     m_recoveryInterval;
  Time     m_cnpMinInterval;
};

} // namespace ns3

#endif /* FLOWSTAR_RATE_CONTROLLER_H */
