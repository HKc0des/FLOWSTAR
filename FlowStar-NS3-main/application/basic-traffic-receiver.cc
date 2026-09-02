#include "basic-traffic-receiver.h"
#include "ns3/log.h"
#include "ns3/simulator.h"
#include "ns3/network-module.h"
#include "../model/basic-flow-id-tag.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("BasicTrafficReceiver");

NS_OBJECT_ENSURE_REGISTERED (BasicTrafficReceiver);

TypeId
BasicTrafficReceiver::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::BasicTrafficReceiver")
    .SetParent<Application> ()
    .SetGroupName("Applications")
    .AddConstructor<BasicTrafficReceiver> ()
  ;
  return tid;
}

BasicTrafficReceiver::BasicTrafficReceiver ()
  : m_port (0),
    m_socket (0)
{
}

BasicTrafficReceiver::~BasicTrafficReceiver ()
{
  m_socket = 0;
}

void
BasicTrafficReceiver::DoDispose (void)
{
  m_socket = 0;
  Application::DoDispose ();
}

void 
BasicTrafficReceiver::Setup (uint16_t port)
{
  m_port = port;
}

void
BasicTrafficReceiver::StartApplication (void)
{
  if (!m_socket)
    {
      m_socket = Socket::CreateSocket (GetNode (), TypeId::LookupByName ("ns3::UdpSocketFactory"));
      InetSocketAddress local = InetSocketAddress (Ipv4Address::GetAny (), m_port);
      m_socket->Bind (local);
    }
  m_socket->SetRecvCallback (MakeCallback (&BasicTrafficReceiver::HandleRead, this));
}

void
BasicTrafficReceiver::StopApplication (void)
{
  if (m_socket)
    {
      m_socket->SetRecvCallback (MakeNullCallback<void, Ptr<Socket> > ());
    }
}

void
BasicTrafficReceiver::HandleRead (Ptr<Socket> socket)
{
  Ptr<Packet> packet;
  Address from;
  while ((packet = socket->RecvFrom (from)))
    {
      BasicFlowIdTag tag;
      if (packet->PeekPacketTag (tag))
        {
          uint32_t flowId = tag.GetFlowId ();
          if (m_flowStats.find(flowId) == m_flowStats.end()) {
            m_flowStats[flowId] = {0, 0, Simulator::Now(), Simulator::Now()};
          }
          m_flowStats[flowId].bytesReceived += packet->GetSize ();
          m_flowStats[flowId].packetsReceived++;
          m_flowStats[flowId].lastPacketTime = Simulator::Now();
        }
    }
}

} // namespace ns3
