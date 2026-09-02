#ifndef CBFC_TRAFFIC_RECEIVER_H
#define CBFC_TRAFFIC_RECEIVER_H

#include "ns3/application.h"
#include "ns3/ptr.h"
#include "ns3/address.h"
#include "ns3/socket.h"
#include "../model/flowstar-mid.h"
#include <map>

namespace ns3 {

/**
 * \brief CBFC traffic receiver that sends credit updates back to senders.
 *
 * On receiving each data packet, it extracts the FlowStarMidTag and
 * sends a CreditUpdateHeader(mid, bytesReceived) to the corresponding
 * sender's credit listen port.
 *
 * Phase 3 simplification: credit updates are receiver-originated.
 * In a hardware implementation, the switch would send credits as buffer
 * space is freed on dequeue. Here the receiver serves as the credit proxy
 * since bytes received == bytes freed at the conceptual switch buffer.
 *
 * Maintains per-flow statistics: bytes received, packets, FCT.
 */
class CbfcTrafficReceiver : public Application
{
public:
  static TypeId GetTypeId (void);

  CbfcTrafficReceiver ();
  virtual ~CbfcTrafficReceiver ();

  void Setup (uint16_t dataPort);

  /// Register sender's control addresses
  void RegisterSenderCredit (FlowStarMid mid, Address senderCreditAddr);
  void RegisterExpectedBytes (FlowStarMid mid, uint64_t expectedBytes);

  // Per-flow stats
  struct FlowStats {
    uint64_t expectedBytes;
    uint64_t bytesReceived;
    uint32_t packetsReceived;
    Time     firstPacketTime;
    Time     lastPacketTime;
    bool     completed;
  };

  const std::map<FlowStarMid, FlowStats>& GetFlowStats (void) const;

protected:
  virtual void DoDispose (void);

private:
  virtual void StartApplication (void);
  virtual void StopApplication (void);

  void HandleRead (Ptr<Socket> socket);
  void SendCreditUpdate (FlowStarMid mid, uint32_t bytesFreed, Address senderCreditAddr);

  uint16_t    m_dataPort;
  Ptr<Socket> m_dataSocket;

  std::map<FlowStarMid, FlowStats>  m_flowStats;
  std::map<FlowStarMid, Address>    m_senderCreditAddrs;
};

} // namespace ns3

#endif /* CBFC_TRAFFIC_RECEIVER_H */
