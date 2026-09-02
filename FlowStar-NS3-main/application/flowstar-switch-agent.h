#ifndef FLOWSTAR_SWITCH_AGENT_H
#define FLOWSTAR_SWITCH_AGENT_H

#include "ns3/application.h"
#include "ns3/socket.h"
#include "ns3/address.h"
#include "ns3/nstime.h"
#include "../model/flowstar-mid.h"
#include "../model/cbfc-queue-disc.h"
#include <map>

namespace ns3 {

/**
 * \brief Switch-side agent for monitoring fabric congestion and generating BECNs.
 *
 * Hooks into the CbfcQueueDisc enqueue callback to monitor per-flow
 * queue occupancy. If the occupancy exceeds a threshold, it sends a
 * FlowStarBecnHeader directly to the registered sender's control port.
 *
 * Implements BECN rate limiting per flow.
 */
class FlowStarSwitchAgent : public Application
{
public:
  static TypeId GetTypeId (void);

  FlowStarSwitchAgent ();
  virtual ~FlowStarSwitchAgent ();

  /// Set the QueueDisc to monitor
  void SetQueueDisc (Ptr<CbfcQueueDisc> qdisc);

  /// Register sender's BECN receive address for a flow
  void RegisterSenderBecn (FlowStarMid mid, Address senderBecnAddr);

  /// Set switch ID for identification in BECNs
  void SetSwitchId (uint32_t id);

protected:
  virtual void DoDispose (void);

private:
  virtual void StartApplication (void);
  virtual void StopApplication (void);

  void OnEnqueue (FlowStarMid mid, uint32_t occupancy);
  void SendBecn (FlowStarMid mid, uint32_t occupancy, Address destAddr);

  Ptr<CbfcQueueDisc> m_qdisc;
  uint32_t           m_switchId;
  uint32_t           m_becnThreshold;
  Time               m_becnInterval;

  std::map<FlowStarMid, Address> m_senderBecnAddrs;
  std::map<FlowStarMid, Time>    m_lastBecnTimes;
};

} // namespace ns3

#endif /* FLOWSTAR_SWITCH_AGENT_H */
