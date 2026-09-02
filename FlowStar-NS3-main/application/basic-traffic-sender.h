#ifndef BASIC_TRAFFIC_SENDER_H
#define BASIC_TRAFFIC_SENDER_H

#include "ns3/application.h"
#include "ns3/ptr.h"
#include "ns3/address.h"
#include "ns3/socket.h"
#include "ns3/event-id.h"
#include "ns3/traced-callback.h"
#include "ns3/data-rate.h"

namespace ns3 {

class BasicTrafficSender : public Application {
public:
  static TypeId GetTypeId (void);

  BasicTrafficSender ();
  virtual ~BasicTrafficSender ();

  void Setup (uint32_t flowId, Address address, uint32_t packetSize, uint32_t numPackets, DataRate dataRate);
  void SetupMessage (uint32_t flowId, Address address, uint32_t packetSize, uint64_t messageSize, DataRate dataRate);

protected:
  virtual void DoDispose (void);

private:
  virtual void StartApplication (void);
  virtual void StopApplication (void);

  void SendPacket (void);
  void ScheduleNextTx (void);

  Ptr<Socket> m_socket;
  Address m_peer;
  uint32_t m_packetSize;
  uint32_t m_numPackets;
  DataRate m_dataRate;
  uint32_t m_flowId;
  uint32_t m_packetsSent;
  uint32_t m_seqNum;
  EventId m_sendEvent;
  bool m_running;
};

} // namespace ns3

#endif /* BASIC_TRAFFIC_SENDER_H */
