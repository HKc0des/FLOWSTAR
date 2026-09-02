#ifndef BASIC_TRAFFIC_RECEIVER_H
#define BASIC_TRAFFIC_RECEIVER_H

#include "ns3/application.h"
#include "ns3/ptr.h"
#include "ns3/address.h"
#include "ns3/socket.h"
#include <map>

namespace ns3 {

class BasicTrafficReceiver : public Application {
public:
  static TypeId GetTypeId (void);

  BasicTrafficReceiver ();
  virtual ~BasicTrafficReceiver ();

  void Setup (uint16_t port);

protected:
  virtual void DoDispose (void);

private:
  virtual void StartApplication (void);
  virtual void StopApplication (void);

  void HandleRead (Ptr<Socket> socket);

  uint16_t m_port;
  Ptr<Socket> m_socket;
  
  struct FlowStats {
    uint64_t bytesReceived;
    uint32_t packetsReceived;
    Time firstPacketTime;
    Time lastPacketTime;
  };
  std::map<uint32_t, FlowStats> m_flowStats;
};

} // namespace ns3

#endif /* BASIC_TRAFFIC_RECEIVER_H */
