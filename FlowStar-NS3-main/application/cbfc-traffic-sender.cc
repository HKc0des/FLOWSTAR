#include "cbfc-traffic-sender.h"
#include "ns3/log.h"
#include "ns3/simulator.h"
#include "ns3/network-module.h"
#include "../model/basic-flow-id-tag.h"
#include "../model/flowstar-mid.h"
#include "../utils/metrics-collector.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("CbfcTrafficSender");

NS_OBJECT_ENSURE_REGISTERED (CbfcTrafficSender);

TypeId
CbfcTrafficSender::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::CbfcTrafficSender")
    .SetParent<Application> ()
    .SetGroupName ("Applications")
    .AddConstructor<CbfcTrafficSender> ()
  ;
  return tid;
}

CbfcTrafficSender::CbfcTrafficSender ()
  : m_dataSocket (0),
    m_creditSocket (0),
    m_packetSize (1000),
    m_numPackets (0),
    m_packetsSent (0),
    m_seqNum (0),
    m_bytesSent (0),
    m_running (false),
    m_creditListenPort (6000),
    m_dataRate ("10Gbps"),
    m_creditManager (0)
{
}

CbfcTrafficSender::~CbfcTrafficSender ()
{
  m_dataSocket   = 0;
  m_creditSocket = 0;
}

void
CbfcTrafficSender::DoDispose (void)
{
  m_dataSocket   = 0;
  m_creditSocket = 0;
  Application::DoDispose ();
}

void
CbfcTrafficSender::Setup (FlowStarMid mid,
                          Address destDataAddress,
                          Address destCreditAddress,
                          uint32_t packetSize,
                          uint64_t messageSize,
                          DataRate dataRate)
{
  m_mid        = mid;
  m_destData   = destDataAddress;
  m_destCredit = destCreditAddress;
  m_packetSize = packetSize;
  m_numPackets = (messageSize + packetSize - 1) / packetSize;
  m_dataRate   = dataRate;
}

void
CbfcTrafficSender::SetCreditManager (Ptr<CbfcCreditManager> cm)
{
  m_creditManager = cm;
}

void
CbfcTrafficSender::SetCreditListenPort (uint16_t port)
{
  m_creditListenPort = port;
}

uint32_t CbfcTrafficSender::GetPacketsSent (void) const { return m_packetsSent; }
uint64_t CbfcTrafficSender::GetBytesSent   (void) const { return m_bytesSent;   }
Time     CbfcTrafficSender::GetStartTime   (void) const { return m_actualStart; }
Time     CbfcTrafficSender::GetEndTime     (void) const { return m_actualEnd;   }

void
CbfcTrafficSender::StartApplication (void)
{
  m_running     = true;
  m_packetsSent = 0;
  m_seqNum      = 0;
  m_bytesSent   = 0;

  // Data socket
  if (!m_dataSocket)
    {
      m_dataSocket = Socket::CreateSocket (GetNode (), TypeId::LookupByName ("ns3::UdpSocketFactory"));
      m_dataSocket->Bind ();
      m_dataSocket->Connect (m_destData);
    }

  // Credit listening socket
  if (!m_creditSocket)
    {
      m_creditSocket = Socket::CreateSocket (GetNode (), TypeId::LookupByName ("ns3::UdpSocketFactory"));
      InetSocketAddress local = InetSocketAddress (Ipv4Address::GetAny (), m_creditListenPort);
      m_creditSocket->Bind (local);
      m_creditSocket->SetRecvCallback (MakeCallback (&CbfcTrafficSender::HandleCreditUpdate, this));
    }

  m_actualStart = Simulator::Now ();
  
  uint64_t expectedBytes = static_cast<uint64_t>(m_numPackets) * m_packetSize;
  MetricsCollector::GetInstance().FlowStart(m_mid.srcFlowId, expectedBytes);

  SendPacket ();
}

void
CbfcTrafficSender::StopApplication (void)
{
  m_running = false;
  if (!m_sendEvent.IsExpired ())
    Simulator::Cancel (m_sendEvent);
  if (!m_retryEvent.IsExpired ())
    Simulator::Cancel (m_retryEvent);
  if (m_dataSocket)
    m_dataSocket->Close ();
  if (m_creditSocket)
    {
      m_creditSocket->SetRecvCallback (MakeNullCallback<void, Ptr<Socket>> ());
      m_creditSocket->Close ();
    }
}

void
CbfcTrafficSender::SendPacket (void)
{
  if (!m_running)
    return;

  // Check per-flow credit gate (FCTBS)
  if (m_creditManager && !m_creditManager->CanTransmit (m_mid, m_packetSize))
    {
      NS_LOG_INFO ("Flow (" << m_mid.srcFlowId << "→" << m_mid.dstFlowId
                   << ") blocked: insufficient FCTBS="
                   << m_creditManager->GetAvailableCredit (m_mid));
      m_retryEvent = Simulator::Schedule (MicroSeconds (10),
                                          &CbfcTrafficSender::RetryAfterBlock, this);
      return;
    }

  // Build packet with FlowStarMidTag + BasicFlowIdTag
  Ptr<Packet> packet = Create<Packet> (m_packetSize);

  FlowStarMidTag midTag (m_mid);
  packet->AddPacketTag (midTag);

  BasicFlowIdTag flowTag (m_mid.srcFlowId, m_seqNum, Simulator::Now ().GetNanoSeconds ());
  packet->AddPacketTag (flowTag);

  m_dataSocket->Send (packet);

  // Consume credit
  if (m_creditManager)
    m_creditManager->ConsumeCredit (m_mid, m_packetSize);

  m_packetsSent++;
  m_seqNum++;
  m_bytesSent += m_packetSize;

  if (m_packetsSent < m_numPackets)
    {
      ScheduleNextTx ();
    }
  else
    {
      m_actualEnd = Simulator::Now ();
      NS_LOG_INFO ("Flow (" << m_mid.srcFlowId << "→" << m_mid.dstFlowId
                   << ") complete: " << m_packetsSent << " packets, "
                   << m_bytesSent << " bytes");
    }
}

void
CbfcTrafficSender::ScheduleNextTx (void)
{
  if (m_running)
    {
      Time tNext = Seconds (static_cast<double> (m_packetSize * 8) /
                            m_dataRate.GetBitRate ());
      m_sendEvent = Simulator::Schedule (tNext, &CbfcTrafficSender::SendPacket, this);
    }
}

void
CbfcTrafficSender::RetryAfterBlock (void)
{
  if (m_running && m_packetsSent < m_numPackets)
    SendPacket ();
}

void
CbfcTrafficSender::HandleCreditUpdate (Ptr<Socket> socket)
{
  Ptr<Packet> pkt;
  Address from;
  while ((pkt = socket->RecvFrom (from)))
    {
      CreditUpdateHeader hdr;
      if (pkt->GetSize () >= hdr.GetSerializedSize ())
        {
          pkt->RemoveHeader (hdr);
          if (m_creditManager)
            {
              NS_LOG_INFO ("Received CreditUpdate for MID(" << hdr.GetMid ().srcFlowId
                           << "→" << hdr.GetMid ().dstFlowId
                           << ") bytes=" << hdr.GetBytesFreed ());
              m_creditManager->ReplenishCredit (hdr.GetMid (), hdr.GetBytesFreed ());
            }
        }
    }
}

} // namespace ns3
