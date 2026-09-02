#ifndef FLOWSTAR_RECEIVER_H
#define FLOWSTAR_RECEIVER_H

#include "ns3/application.h"
#include "ns3/ptr.h"
#include "ns3/address.h"
#include "ns3/socket.h"
#include "../model/flowstar-mid.h"
#include "../model/flowstar-endpoint-controller.h"
#include <map>

namespace ns3 {

/**
 * \brief Integrated FlowStar receiver (Phase 4).
 *
 * Functions:
 * 1. Receives data packets.
 * 2. Generates credit updates back to sender (Phase 3 CBFC).
 * 3. Measures receiving rate via FlowStarEndpointController.
 * 4. Generates CNPs back to sender when rate drops (Endpoint Congestion).
 */
class FlowStarReceiver : public Application
{
public:
  static TypeId GetTypeId (void);

  FlowStarReceiver ();
  virtual ~FlowStarReceiver ();

  void Setup (uint16_t dataPort);

  /// Register sender's control addresses
  void RegisterSenderCredit (FlowStarMid mid, Address senderCreditAddr);
  void RegisterSenderCnp (FlowStarMid mid, Address senderCnpAddr, uint64_t expectedRateBps);
  void RegisterExpectedBytes (FlowStarMid mid, uint64_t expectedBytes);

  // Per-flow stats
  struct FlowStats {
    uint64_t expectedBytes;
    uint64_t bytesReceived;
    uint32_t packetsReceived;
    Time     firstPacketTime;
    Time     lastPacketTime;
    uint32_t cnpsSent;
    bool     completed;
  };

  const std::map<FlowStarMid, FlowStats>& GetFlowStats (void) const;

  /// Configure Endpoint Controller
  void SetCnpThreshold (double fraction);

protected:
  virtual void DoDispose (void);

private:
  virtual void StartApplication (void);
  virtual void StopApplication (void);

  void HandleRead (Ptr<Socket> socket);
  void SendCreditUpdate (FlowStarMid mid, uint32_t bytesFreed, Address dest);
  void SendCnp (FlowStarMid mid, uint64_t rateBps, Address dest);

  uint16_t    m_dataPort;
  Ptr<Socket> m_dataSocket;

  Ptr<FlowStarEndpointController> m_endpointController;
  double m_cnpThresholdFraction;

  std::map<FlowStarMid, FlowStats>  m_flowStats;
  std::map<FlowStarMid, Address>    m_senderCreditAddrs;
  
  struct CnpConfig {
    Address  addr;
    uint64_t expectedRateBps;
  };
  std::map<FlowStarMid, CnpConfig>  m_senderCnpConfigs;
};

} // namespace ns3

#endif /* FLOWSTAR_RECEIVER_H */
