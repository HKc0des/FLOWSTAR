#ifndef FLOWSTAR_SENDER_H
#define FLOWSTAR_SENDER_H

#include "ns3/application.h"
#include "ns3/ptr.h"
#include "ns3/socket.h"
#include "ns3/data-rate.h"
#include "ns3/event-id.h"
#include "../model/flowstar-mid.h"
#include "../model/cbfc-credit-manager.h"
#include "../model/flowstar-rate-controller.h"

namespace ns3 {

/**
 * \brief Integrated FlowStar sender (Phase 4).
 *
 * Implements:
 * 1. CBFC Phase 3 credit gating (Wait for credit).
 * 2. Original FlowStar Phase 4 rate pacing (Wait for rate token).
 * 3. Listens for Credit Updates, CNPs, and BECNs on specific ports.
 */
class FlowStarSender : public Application
{
public:
  static TypeId GetTypeId (void);

  FlowStarSender ();
  virtual ~FlowStarSender ();

  void Setup (FlowStarMid mid, Address peer, uint32_t packetSize, uint32_t numPackets, DataRate lineRate);

  void RegisterCreditManager (Ptr<CbfcCreditManager> creditManager);
  void RegisterRateController (Ptr<FlowStarRateController> rateController);

  void SetListenPorts (uint16_t creditPort, uint16_t cnpPort, uint16_t becnPort);

protected:
  virtual void DoDispose (void);

private:
  virtual void StartApplication (void);
  virtual void StopApplication (void);

  void ScheduleNextTx (void);
  void SendPacket (void);

  void HandleCredit (Ptr<Socket> socket);
  void HandleCnp (Ptr<Socket> socket);
  void HandleBecn (Ptr<Socket> socket);

  FlowStarMid m_mid;
  Address     m_peer;
  uint32_t    m_packetSize;
  uint32_t    m_numPackets;
  uint32_t    m_packetsSent;
  DataRate    m_lineRate;

  uint16_t    m_creditPort;
  uint16_t    m_cnpPort;
  uint16_t    m_becnPort;

  Ptr<Socket> m_dataSocket;
  Ptr<Socket> m_creditSocket;
  Ptr<Socket> m_cnpSocket;
  Ptr<Socket> m_becnSocket;

  Ptr<CbfcCreditManager>     m_creditManager;
  Ptr<FlowStarRateController> m_rateController;

  EventId m_sendEvent;
};

} // namespace ns3

#endif /* FLOWSTAR_SENDER_H */
