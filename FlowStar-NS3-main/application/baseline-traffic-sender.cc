#include "baseline-traffic-sender.h"
#include "ns3/log.h"
#include "ns3/simulator.h"
#include "ns3/network-module.h"
#include "../model/basic-flow-id-tag.h"
#include "../model/baseline-control-message.h"
#include "../utils/metrics-collector.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("BaselineTrafficSender");

NS_OBJECT_ENSURE_REGISTERED (BaselineTrafficSender);

TypeId
BaselineTrafficSender::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::BaselineTrafficSender")
    .SetParent<Application> ()
    .SetGroupName("Applications")
    .AddConstructor<BaselineTrafficSender> ()
  ;
  return tid;
}

BaselineTrafficSender::BaselineTrafficSender ()
  : m_socket (0),
    m_cnpSocket (0),
    m_peer (),
    m_packetSize (1000),
    m_numPackets (0),
    m_flowId (0),
    m_packetsSent (0),
    m_seqNum (0),
    m_sendEvent (),
    m_retryEvent (),
    m_running (false),
    m_cnpPort (5000),
    m_flowController (0),
    m_rateController (0)
{
}

BaselineTrafficSender::~BaselineTrafficSender ()
{
  m_socket = 0;
  m_cnpSocket = 0;
}

void
BaselineTrafficSender::DoDispose (void)
{
  m_socket = 0;
  m_cnpSocket = 0;
  Application::DoDispose ();
}

void
BaselineTrafficSender::Setup (uint32_t flowId, Address destAddress,
                              uint32_t packetSize, uint64_t messageSize,
                              DataRate initialRate)
{
  m_flowId = flowId;
  m_peer = destAddress;
  m_packetSize = packetSize;
  m_numPackets = (messageSize + packetSize - 1) / packetSize;

  if (m_rateController)
    {
      m_rateController->SetInitialRate (initialRate);
    }
}

void
BaselineTrafficSender::SetFlowController (Ptr<SharedQueueFlowControl> fc)
{
  m_flowController = fc;
}

void
BaselineTrafficSender::SetRateController (Ptr<BaselineRateController> rc)
{
  m_rateController = rc;
}

void
BaselineTrafficSender::SetCnpPort (uint16_t port)
{
  m_cnpPort = port;
}

void
BaselineTrafficSender::StartApplication (void)
{
  m_running = true;
  m_packetsSent = 0;
  m_seqNum = 0;

  // Data socket
  if (!m_socket)
    {
      m_socket = Socket::CreateSocket (GetNode (), TypeId::LookupByName ("ns3::UdpSocketFactory"));
      m_socket->Bind ();
      m_socket->Connect (m_peer);
    }

  // CNP listening socket
  if (!m_cnpSocket)
    {
      m_cnpSocket = Socket::CreateSocket (GetNode (), TypeId::LookupByName ("ns3::UdpSocketFactory"));
      InetSocketAddress local = InetSocketAddress (Ipv4Address::GetAny (), m_cnpPort);
      m_cnpSocket->Bind (local);
    }
  m_cnpSocket->SetRecvCallback (MakeCallback (&BaselineTrafficSender::HandleCnp, this));

  uint64_t expectedBytes = static_cast<uint64_t>(m_numPackets) * m_packetSize;
  MetricsCollector::GetInstance().FlowStart(m_flowId, expectedBytes);

  SendPacket ();
}

void
BaselineTrafficSender::StopApplication (void)
{
  m_running = false;
  if (!m_sendEvent.IsExpired ())
    {
      Simulator::Cancel (m_sendEvent);
    }
  if (!m_retryEvent.IsExpired ())
    {
      Simulator::Cancel (m_retryEvent);
    }
  if (m_socket)
    {
      m_socket->Close ();
    }
  if (m_cnpSocket)
    {
      m_cnpSocket->SetRecvCallback (MakeNullCallback<void, Ptr<Socket> > ());
      m_cnpSocket->Close ();
    }
}

void
BaselineTrafficSender::SendPacket (void)
{
  if (!m_running)
    return;

  // Check flow control gate
  if (m_flowController && !m_flowController->CanTransmit (m_flowId, m_packetSize))
    {
      NS_LOG_INFO ("Flow " << m_flowId << " PAUSED by flow control, will retry");
      // Schedule a retry after a short backoff
      m_retryEvent = Simulator::Schedule (MicroSeconds (10),
                                          &BaselineTrafficSender::RetryAfterPause, this);
      return;
    }

  Ptr<Packet> packet = Create<Packet> (m_packetSize);

  BasicFlowIdTag tag (m_flowId, m_seqNum, Simulator::Now().GetNanoSeconds());
  packet->AddPacketTag (tag);

  m_socket->Send (packet);

  m_packetsSent++;
  m_seqNum++;

  // Try recovery on every send
  if (m_rateController)
    {
      m_rateController->TryRecovery (m_flowId);
    }

  if (m_packetsSent < m_numPackets)
    {
      ScheduleNextTx ();
    }
}

void
BaselineTrafficSender::ScheduleNextTx (void)
{
  if (m_running)
    {
      DataRate currentRate = m_rateController
                             ? m_rateController->GetCurrentRate ()
                             : DataRate ("10Gbps");

      Time tNext = Seconds (
        static_cast<double>(m_packetSize * 8) / currentRate.GetBitRate ());
      m_sendEvent = Simulator::Schedule (tNext, &BaselineTrafficSender::SendPacket, this);
    }
}

void
BaselineTrafficSender::HandleCnp (Ptr<Socket> socket)
{
  Ptr<Packet> packet;
  Address from;
  while ((packet = socket->RecvFrom (from)))
    {
      CnpHeader cnpHeader;
      packet->RemoveHeader (cnpHeader);

      uint32_t flowId = cnpHeader.GetFlowId ();
      NS_LOG_INFO ("Received CNP for flow " << flowId);

      if (m_rateController)
        {
          m_rateController->OnCnpReceived (flowId);
        }
    }
}

void
BaselineTrafficSender::RetryAfterPause (void)
{
  if (m_running && m_packetsSent < m_numPackets)
    {
      SendPacket ();
    }
}

} // namespace ns3
