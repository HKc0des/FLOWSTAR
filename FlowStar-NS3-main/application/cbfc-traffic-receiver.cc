#include "cbfc-traffic-receiver.h"
#include "ns3/log.h"
#include "ns3/simulator.h"
#include "ns3/network-module.h"
#include "../model/flowstar-mid.h"
#include "../model/basic-flow-id-tag.h"
#include "../utils/metrics-collector.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("CbfcTrafficReceiver");

NS_OBJECT_ENSURE_REGISTERED (CbfcTrafficReceiver);

TypeId
CbfcTrafficReceiver::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::CbfcTrafficReceiver")
    .SetParent<Application> ()
    .SetGroupName ("Applications")
    .AddConstructor<CbfcTrafficReceiver> ()
  ;
  return tid;
}

CbfcTrafficReceiver::CbfcTrafficReceiver ()
  : m_dataPort (0),
    m_dataSocket (0)
{
}

CbfcTrafficReceiver::~CbfcTrafficReceiver ()
{
  m_dataSocket = 0;
}

void
CbfcTrafficReceiver::DoDispose (void)
{
  m_dataSocket = 0;
  Application::DoDispose ();
}

void
CbfcTrafficReceiver::Setup (uint16_t dataPort)
{
  m_dataPort = dataPort;
}

void
CbfcTrafficReceiver::RegisterSenderCredit (FlowStarMid mid, Address senderCreditAddr)
{
  m_senderCreditAddrs[mid] = senderCreditAddr;
}

void
CbfcTrafficReceiver::RegisterExpectedBytes (FlowStarMid mid, uint64_t expectedBytes)
{
  if (m_flowStats.find (mid) == m_flowStats.end ())
    {
      m_flowStats[mid] = {expectedBytes, 0, 0, Simulator::Now (), Simulator::Now (), false};
    }
  else
    {
      m_flowStats[mid].expectedBytes = expectedBytes;
    }
}

const std::map<FlowStarMid, CbfcTrafficReceiver::FlowStats>&
CbfcTrafficReceiver::GetFlowStats (void) const
{
  return m_flowStats;
}

void
CbfcTrafficReceiver::StartApplication (void)
{
  if (!m_dataSocket)
    {
      m_dataSocket = Socket::CreateSocket (GetNode (), TypeId::LookupByName ("ns3::UdpSocketFactory"));
      InetSocketAddress local = InetSocketAddress (Ipv4Address::GetAny (), m_dataPort);
      m_dataSocket->Bind (local);
    }
  m_dataSocket->SetRecvCallback (MakeCallback (&CbfcTrafficReceiver::HandleRead, this));
}

void
CbfcTrafficReceiver::StopApplication (void)
{
  if (m_dataSocket)
    m_dataSocket->SetRecvCallback (MakeNullCallback<void, Ptr<Socket>> ());
}

void
CbfcTrafficReceiver::HandleRead (Ptr<Socket> socket)
{
  Ptr<Packet> pkt;
  Address from;
  while ((pkt = socket->RecvFrom (from)))
    {
      FlowStarMidTag midTag;
      if (!pkt->PeekPacketTag (midTag))
        continue;

      FlowStarMid mid = midTag.GetMid ();
      uint32_t    sz  = pkt->GetSize ();

      // Update per-flow stats
      if (m_flowStats.find (mid) == m_flowStats.end ())
        {
          m_flowStats[mid] = {0, 0, 0, Simulator::Now (), Simulator::Now (), false};
        }
      auto& s = m_flowStats[mid];
      s.bytesReceived   += sz;
      s.packetsReceived++;
      s.lastPacketTime   = Simulator::Now ();

      if (s.expectedBytes > 0 && s.bytesReceived >= s.expectedBytes && !s.completed)
        {
          s.completed = true;
          MetricsCollector::GetInstance().FlowComplete(mid.srcFlowId, s.bytesReceived);
        }

      NS_LOG_INFO ("Received " << sz << " bytes from MID("
                   << mid.srcFlowId << "→" << mid.dstFlowId
                   << ") total=" << s.bytesReceived);

      // Send credit update to sender
      auto it = m_senderCreditAddrs.find (mid);
      if (it != m_senderCreditAddrs.end ())
        {
          SendCreditUpdate (mid, sz, it->second);
        }
    }
}

void
CbfcTrafficReceiver::SendCreditUpdate (FlowStarMid mid, uint32_t bytesFreed,
                                       Address senderCreditAddr)
{
  Ptr<Socket> sock = Socket::CreateSocket (GetNode (), TypeId::LookupByName ("ns3::UdpSocketFactory"));
  sock->Connect (senderCreditAddr);

  Ptr<Packet> ctrlPkt = Create<Packet> (0);
  CreditUpdateHeader hdr (mid, bytesFreed);
  ctrlPkt->AddHeader (hdr);

  sock->Send (ctrlPkt);
  sock->Close ();
}

} // namespace ns3
