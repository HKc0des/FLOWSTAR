#ifndef BASELINE_RATE_CONTROLLER_H
#define BASELINE_RATE_CONTROLLER_H

#include "ns3/object.h"
#include "ns3/data-rate.h"
#include "ns3/nstime.h"
#include "ns3/simulator.h"

namespace ns3 {

/**
 * \brief Baseline AIMD rate controller for end-to-end congestion control.
 *
 * Attached to the sender. Maintains a current target rate that starts
 * at the configured initial (line) rate. When a CNP is received, the
 * rate is multiplicatively decreased. When no CNPs arrive within a
 * recovery interval, the rate is additively increased.
 *
 * All parameters are configurable for reproducible experiments.
 */
class BaselineRateController : public Object {
public:
  static TypeId GetTypeId (void);

  BaselineRateController ();
  virtual ~BaselineRateController ();

  // Configuration
  void SetInitialRate (DataRate rate);
  void SetMinRate (DataRate rate);
  void SetMultiplicativeDecreaseFactor (double factor);
  void SetAdditiveIncreaseStep (DataRate step);
  void SetRecoveryInterval (Time interval);

  // Interface
  void OnCnpReceived (uint32_t flowId);
  DataRate GetCurrentRate (void) const;

  // Called periodically to attempt additive increase
  void TryRecovery (uint32_t flowId);

private:
  DataRate m_initialRate;
  DataRate m_minRate;
  DataRate m_currentRate;
  double m_multiplicativeDecreaseFactor;
  DataRate m_additiveIncreaseStep;
  Time m_recoveryInterval;
  Time m_lastCnpTime;
  bool m_enabled;
};

} // namespace ns3

#endif /* BASELINE_RATE_CONTROLLER_H */
