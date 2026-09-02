#include "baseline-rate-controller.h"
#include "ns3/log.h"
#include "../utils/metrics-collector.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("BaselineRateController");

NS_OBJECT_ENSURE_REGISTERED (BaselineRateController);

TypeId
BaselineRateController::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::BaselineRateController")
    .SetParent<Object> ()
    .SetGroupName("FlowStar")
    .AddConstructor<BaselineRateController> ()
  ;
  return tid;
}

BaselineRateController::BaselineRateController ()
  : m_initialRate ("10Gbps"),
    m_minRate ("100Mbps"),
    m_currentRate ("10Gbps"),
    m_multiplicativeDecreaseFactor (0.5),
    m_additiveIncreaseStep ("500Mbps"),
    m_recoveryInterval (MicroSeconds (100)),
    m_lastCnpTime (Seconds (0)),
    m_enabled (true)
{
}

BaselineRateController::~BaselineRateController ()
{
}

void
BaselineRateController::SetInitialRate (DataRate rate)
{
  m_initialRate = rate;
  m_currentRate = rate;
}

void
BaselineRateController::SetMinRate (DataRate rate)
{
  m_minRate = rate;
}

void
BaselineRateController::SetMultiplicativeDecreaseFactor (double factor)
{
  m_multiplicativeDecreaseFactor = factor;
}

void
BaselineRateController::SetAdditiveIncreaseStep (DataRate step)
{
  m_additiveIncreaseStep = step;
}

void
BaselineRateController::SetRecoveryInterval (Time interval)
{
  m_recoveryInterval = interval;
}

void
BaselineRateController::OnCnpReceived (uint32_t flowId)
{
  if (!m_enabled)
    return;

  m_lastCnpTime = Simulator::Now ();

  uint64_t newBitRate = static_cast<uint64_t> (
    m_currentRate.GetBitRate () * m_multiplicativeDecreaseFactor);

  if (newBitRate < m_minRate.GetBitRate ())
    {
      newBitRate = m_minRate.GetBitRate ();
    }

  NS_LOG_INFO ("CNP received for flow " << flowId
               << ": rate " << m_currentRate << " -> "
               << DataRate (newBitRate));

  m_currentRate = DataRate (newBitRate);
  MetricsCollector::GetInstance().RateChange(flowId, m_currentRate);
}

DataRate
BaselineRateController::GetCurrentRate (void) const
{
  return m_currentRate;
}

void
BaselineRateController::TryRecovery (uint32_t flowId)
{
  if (!m_enabled)
    return;

  Time elapsed = Simulator::Now () - m_lastCnpTime;

  if (elapsed >= m_recoveryInterval)
    {
      uint64_t newBitRate = m_currentRate.GetBitRate ()
                            + m_additiveIncreaseStep.GetBitRate ();

      if (newBitRate > m_initialRate.GetBitRate ())
        {
          newBitRate = m_initialRate.GetBitRate ();
        }

      if (newBitRate != m_currentRate.GetBitRate ())
        {
          NS_LOG_INFO ("Recovery: rate " << m_currentRate << " -> "
                       << DataRate (newBitRate));
          m_currentRate = DataRate (newBitRate);
          MetricsCollector::GetInstance().RateChange(flowId, m_currentRate);
        }
    }
}

} // namespace ns3
