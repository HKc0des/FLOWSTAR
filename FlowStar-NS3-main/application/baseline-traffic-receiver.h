#ifndef BASELINE_TRAFFIC_RECEIVER_H
#define BASELINE_TRAFFIC_RECEIVER_H

#include "ns3/application.h"
#include "ns3/ptr.h"
#include "ns3/address.h"
#include "ns3/socket.h"
#include <map>

namespace ns3 {

/**
 * \brief A traffic receiver that inspects incoming data packets for
 *        ECN marks and generates CNP packets back to the sender.
 *
 * This implements the receiver side of the baseline ECN/CNP congestion
 * control loop: Switch marks → Receiver detects → Receiver sends CNP.
 */
class BaselineTrafficReceiver : public Application {
public:
  static TypeId GetTypeId (void);

  BaselineTrafficReceiver ();
  virtual ~BaselineTrafficReceiver ();

  void Setup (uint16_t dataPort, uint16_t cnpPort);
  void SetCcEnabled (bool enabled);

  // Store sender addresses for CNP delivery (flowId -> sender address)
  void RegisterSender (uint32_t flowId, Address senderAddress);
  void RegisterExpectedBytes (uint32_t flowId, uint64_t expectedBytes);

protected:
  virtual void DoDispose (void);

private:
  virtual void StartApplication (void);
  virtual void StopApplication (void);

  void HandleRead (Ptr<Socket> socket);
  void SendCnp (uint32_t flowId, Address senderAddress);

  uint16_t m_dataPort;
  uint16_t m_cnpPort;
  Ptr<Socket> m_dataSocket;
  bool m_ccEnabled;

  struct FlowStats {
    uint64_t expectedBytes;
    uint64_t bytesReceived;
    uint32_t packetsReceived;
    Time firstPacketTime;
    Time lastPacketTime;
    uint32_t ecnMarkedPackets;
    bool completed;
  };
  std::map<uint32_t, FlowStats> m_flowStats;
  std::map<uint32_t, Address> m_senderAddresses;
};

} // namespace ns3

#endif /* BASELINE_TRAFFIC_RECEIVER_H */
