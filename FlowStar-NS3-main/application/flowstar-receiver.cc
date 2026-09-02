#include "flowstar-receiver.h"
#include "ns3/log.h"
#include "ns3/simulator.h"
#include "ns3/network-module.h"
#include "../model/flowstar-mid.h"
#include "../model/flowstar-control-message.h"
#include "../utils/metrics-collector.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("FlowStarReceiver");

NS_OBJECT_ENSURE_REGISTERED (FlowStarReceiver);

TypeId
FlowStarReceiver::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::FlowStarReceiver")
    .SetParent<Application> ()
    .SetGroupName ("Applications")
    .AddConstructor<FlowStarReceiver> ()
  ;
  return tid;
}

FlowStarReceiver::FlowStarReceiver ()
  : m_dataPort (0),
    m_dataSocket (0),
    m_cnpThresholdFraction (0.8) // Default: trigger CNP if rate drops below 80%
{
  m_endpointController = CreateObject<FlowStarEndpointController> ();
}

FlowStarReceiver::~FlowStarReceiver ()
{
  m_dataSocket = 0;
}

void
FlowStarReceiver::DoDispose (void)
{
  m_dataSocket = 0;
  m_endpointController = 0;
  Application::DoDispose ();
}

void
FlowStarReceiver::Setup (uint16_t dataPort)
{
  m_dataPort = dataPort;
}

void
FlowStarReceiver::SetCnpThreshold (double fraction)
{
  m_cnpThresholdFraction = fraction;
}

void
FlowStarReceiver::RegisterSenderCredit (FlowStarMid mid, Address senderCreditAddr)
{
  m_senderCreditAddrs[mid] = senderCreditAddr;
}

void
FlowStarReceiver::RegisterExpectedBytes (FlowStarMid mid, uint64_t expectedBytes)
{
  if (m_flowStats.find (mid) == m_flowStats.end ())
    {
      m_flowStats[mid] = {expectedBytes, 0, 0, Simulator::Now (), Simulator::Now (), 0, false};
    }
  else
    {
      m_flowStats[mid].expectedBytes = expectedBytes;
    }
  MetricsCollector::GetInstance().FlowStart(mid.srcFlowId, expectedBytes);
}

void
FlowStarReceiver::RegisterSenderCnp (FlowStarMid mid, Address senderCnpAddr, uint64_t expectedRateBps)
{
  m_senderCnpConfigs[mid] = {senderCnpAddr, expectedRateBps};
}

const std::map<FlowStarMid, FlowStarReceiver::FlowStats>&
FlowStarReceiver::GetFlowStats (void) const
{
  return m_flowStats;
}

void
FlowStarReceiver::StartApplication (void)
{
  if (!m_dataSocket)
    {
      m_dataSocket = Socket::CreateSocket (GetNode (), TypeId::LookupByName ("ns3::UdpSocketFactory"));
      InetSocketAddress local = InetSocketAddress (Ipv4Address::GetAny (), m_dataPort);
      m_dataSocket->Bind (local);
    }
  m_dataSocket->SetRecvCallback (MakeCallback (&FlowStarReceiver::HandleRead, this));
}

void
FlowStarReceiver::StopApplication (void)
{
  if (m_dataSocket)
    m_dataSocket->SetRecvCallback (MakeNullCallback<void, Ptr<Socket>> ());
}

void
FlowStarReceiver::HandleRead (Ptr<Socket> socket)
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

      // 1. Update stats
      if (m_flowStats.find (mid) == m_flowStats.end ())
        {
          m_flowStats[mid] = {0, 0, 0, Simulator::Now (), Simulator::Now (), 0, false};
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
      else
        {
          MetricsCollector::GetInstance().FlowProgress(mid.srcFlowId, s.bytesReceived);
        }

      // 2. Endpoint measurement
      m_endpointController->OnPacketReceived (mid, sz);

      // 3. Send Phase 3 Credit Update
      auto itCredit = m_senderCreditAddrs.find (mid);
      if (itCredit != m_senderCreditAddrs.end ())
        {
          SendCreditUpdate (mid, sz, itCredit->second);
        }

      // 4. Check for Endpoint Congestion (CNP)
      auto itCnp = m_senderCnpConfigs.find (mid);
      if (itCnp != m_senderCnpConfigs.end ())
        {
          if (m_endpointController->ShouldGenerateCnp (mid, itCnp->second.expectedRateBps, m_cnpThresholdFraction))
            {
              uint64_t measuredRate = m_endpointController->GetReceivingRateBps (mid);
              SendCnp (mid, measuredRate, itCnp->second.addr);
              m_endpointController->RecordCnpSent (mid);
              s.cnpsSent++;
            }
        }
    }
}

void
FlowStarReceiver::SendCreditUpdate (FlowStarMid mid, uint32_t bytesFreed, Address dest)
{
  Ptr<Socket> sock = Socket::CreateSocket (GetNode (), TypeId::LookupByName ("ns3::UdpSocketFactory"));
  sock->Connect (dest);
  Ptr<Packet> ctrlPkt = Create<Packet> (0);
  CreditUpdateHeader hdr (mid, bytesFreed);
  ctrlPkt->AddHeader (hdr);
  sock->Send (ctrlPkt);
  sock->Close ();
}

void
FlowStarReceiver::SendCnp (FlowStarMid mid, uint64_t rateBps, Address dest)
{
  NS_LOG_INFO ("Generating CNP for MID " << mid.srcFlowId << "->" << mid.dstFlowId
               << " measuredRate=" << rateBps << " bps");

  Ptr<Socket> sock = Socket::CreateSocket (GetNode (), TypeId::LookupByName ("ns3::UdpSocketFactory"));
  sock->Connect (dest);
  Ptr<Packet> ctrlPkt = Create<Packet> (0);
  FlowStarCnpHeader hdr (mid, rateBps, Simulator::Now ().GetNanoSeconds ());
  ctrlPkt->AddHeader (hdr);
  sock->Send (ctrlPkt);
  sock->Close ();
}

} // namespace ns3
