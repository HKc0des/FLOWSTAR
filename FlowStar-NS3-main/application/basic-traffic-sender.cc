#include "basic-traffic-sender.h"
#include "ns3/log.h"
#include "ns3/simulator.h"
#include "ns3/network-module.h"
#include "../model/basic-flow-id-tag.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("BasicTrafficSender");

NS_OBJECT_ENSURE_REGISTERED (BasicTrafficSender);

TypeId
BasicTrafficSender::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::BasicTrafficSender")
    .SetParent<Application> ()
    .SetGroupName("Applications")
    .AddConstructor<BasicTrafficSender> ()
  ;
  return tid;
}

BasicTrafficSender::BasicTrafficSender ()
  : m_socket (0),
    m_peer (),
    m_packetSize (1000),
    m_numPackets (0),
    m_dataRate ("1Gbps"),
    m_flowId (0),
    m_packetsSent (0),
    m_seqNum (0),
    m_sendEvent (),
    m_running (false)
{
}

BasicTrafficSender::~BasicTrafficSender ()
{
  m_socket = 0;
}

void
BasicTrafficSender::DoDispose (void)
{
  m_socket = 0;
  Application::DoDispose ();
}

void 
BasicTrafficSender::Setup (uint32_t flowId, Address address, uint32_t packetSize, uint32_t numPackets, DataRate dataRate)
{
  m_flowId = flowId;
  m_peer = address;
  m_packetSize = packetSize;
  m_numPackets = numPackets;
  m_dataRate = dataRate;
}

void 
BasicTrafficSender::SetupMessage (uint32_t flowId, Address address, uint32_t packetSize, uint64_t messageSize, DataRate dataRate)
{
  m_flowId = flowId;
  m_peer = address;
  m_packetSize = packetSize;
  m_numPackets = (messageSize + packetSize - 1) / packetSize; // ceiling division
  m_dataRate = dataRate;
}

void
BasicTrafficSender::StartApplication (void)
{
  m_running = true;
  m_packetsSent = 0;
  m_seqNum = 0;

  if (!m_socket)
    {
      m_socket = Socket::CreateSocket (GetNode (), TypeId::LookupByName ("ns3::UdpSocketFactory"));
      m_socket->Bind ();
      m_socket->Connect (m_peer);
    }

  SendPacket ();
}

void
BasicTrafficSender::StopApplication (void)
{
  m_running = false;
  if (!m_sendEvent.IsExpired ())
    {
      Simulator::Cancel (m_sendEvent);
    }
  if (m_socket)
    {
      m_socket->Close ();
    }
}

void
BasicTrafficSender::SendPacket (void)
{
  if (!m_running)
    return;

  Ptr<Packet> packet = Create<Packet> (m_packetSize);
  
  BasicFlowIdTag tag (m_flowId, m_seqNum, Simulator::Now().GetNanoSeconds());
  packet->AddPacketTag (tag);
  
  m_socket->Send (packet);
  
  m_packetsSent++;
  m_seqNum++;

  if (m_packetsSent < m_numPackets)
    {
      ScheduleNextTx ();
    }
}

void
BasicTrafficSender::ScheduleNextTx (void)
{
  if (m_running)
    {
      Time tNext = Seconds (m_packetSize * 8 / m_dataRate.GetBitRate ());
      m_sendEvent = Simulator::Schedule (tNext, &BasicTrafficSender::SendPacket, this);
    }
}

} // namespace ns3
