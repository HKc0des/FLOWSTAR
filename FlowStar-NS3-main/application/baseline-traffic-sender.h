#ifndef BASELINE_TRAFFIC_SENDER_H
#define BASELINE_TRAFFIC_SENDER_H

#include "ns3/application.h"
#include "ns3/ptr.h"
#include "ns3/address.h"
#include "ns3/socket.h"
#include "ns3/event-id.h"
#include "ns3/data-rate.h"
#include "../model/shared-queue-flow-control.h"
#include "../model/baseline-rate-controller.h"

namespace ns3 {

/**
 * \brief A traffic sender that integrates with SharedQueueFlowControl
 *        and BaselineRateController for Phase 2 baseline experiments.
 *
 * Before sending, it checks CanTransmit() from the flow controller.
 * Pacing is determined by the BaselineRateController's current rate.
 * Listens on a CNP port for incoming congestion notifications.
 */
class BaselineTrafficSender : public Application {
public:
  static TypeId GetTypeId (void);

  BaselineTrafficSender ();
  virtual ~BaselineTrafficSender ();

  void Setup (uint32_t flowId, Address destAddress, uint32_t packetSize,
              uint64_t messageSize, DataRate initialRate);

  void SetFlowController (Ptr<SharedQueueFlowControl> fc);
  void SetRateController (Ptr<BaselineRateController> rc);
  void SetCnpPort (uint16_t port);

protected:
  virtual void DoDispose (void);

private:
  virtual void StartApplication (void);
  virtual void StopApplication (void);

  void SendPacket (void);
  void ScheduleNextTx (void);
  void HandleCnp (Ptr<Socket> socket);
  void RetryAfterPause (void);

  Ptr<Socket> m_socket;       // data socket
  Ptr<Socket> m_cnpSocket;    // socket to receive CNPs
  Address m_peer;
  uint32_t m_packetSize;
  uint32_t m_numPackets;
  uint32_t m_flowId;
  uint32_t m_packetsSent;
  uint32_t m_seqNum;
  EventId m_sendEvent;
  EventId m_retryEvent;
  bool m_running;
  uint16_t m_cnpPort;

  Ptr<SharedQueueFlowControl> m_flowController;
  Ptr<BaselineRateController> m_rateController;
};

} // namespace ns3

#endif /* BASELINE_TRAFFIC_SENDER_H */
