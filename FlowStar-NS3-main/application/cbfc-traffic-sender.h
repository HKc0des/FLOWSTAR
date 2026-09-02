#ifndef CBFC_TRAFFIC_SENDER_H
#define CBFC_TRAFFIC_SENDER_H

#include "ns3/application.h"
#include "ns3/ptr.h"
#include "ns3/address.h"
#include "ns3/socket.h"
#include "ns3/event-id.h"
#include "ns3/data-rate.h"
#include "../model/flowstar-mid.h"
#include "../model/cbfc-credit-manager.h"

namespace ns3 {

/**
 * \brief Credit-gated traffic sender for Phase 3 CBFC.
 *
 * Before sending each packet, checks CbfcCreditManager::CanTransmit().
 * If credit is insufficient, backs off and retries.
 * On each send, calls ConsumeCredit() to deduct FCTBS.
 *
 * Listens on a dedicated port for CreditUpdate control packets from the
 * receiver. On receipt, calls ReplenishCredit() on the credit manager.
 *
 * Tags every data packet with FlowStarMidTag so that CbfcQueueDisc at
 * the switch can perform Dynamic Queue Assignment.
 */
class CbfcTrafficSender : public Application
{
public:
  static TypeId GetTypeId (void);

  CbfcTrafficSender ();
  virtual ~CbfcTrafficSender ();

  void Setup (FlowStarMid mid,
              Address destDataAddress,
              Address destCreditAddress,
              uint32_t packetSize,
              uint64_t messageSize,
              DataRate dataRate);

  void SetCreditManager (Ptr<CbfcCreditManager> cm);
  void SetCreditListenPort (uint16_t port);

  // Accessors for experiment reporting
  uint32_t GetPacketsSent   (void) const;
  uint64_t GetBytesSent     (void) const;
  Time     GetStartTime     (void) const;
  Time     GetEndTime       (void) const;

protected:
  virtual void DoDispose (void);

private:
  virtual void StartApplication (void);
  virtual void StopApplication (void);

  void SendPacket (void);
  void ScheduleNextTx (void);
  void RetryAfterBlock (void);
  void HandleCreditUpdate (Ptr<Socket> socket);

  Ptr<Socket>  m_dataSocket;
  Ptr<Socket>  m_creditSocket;
  Address      m_destData;
  Address      m_destCredit;   ///< receiver's credit listen address (unused in Phase 3 — receiver reads from data socket addr)
  uint32_t     m_packetSize;
  uint32_t     m_numPackets;
  uint32_t     m_packetsSent;
  uint32_t     m_seqNum;
  uint64_t     m_bytesSent;
  EventId      m_sendEvent;
  EventId      m_retryEvent;
  bool         m_running;
  uint16_t     m_creditListenPort;
  DataRate     m_dataRate;
  FlowStarMid  m_mid;
  Time         m_actualStart;
  Time         m_actualEnd;

  Ptr<CbfcCreditManager> m_creditManager;
};

} // namespace ns3

#endif /* CBFC_TRAFFIC_SENDER_H */
