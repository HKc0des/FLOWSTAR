#include "flowstar-sender.h"
#include "ns3/log.h"
#include "ns3/simulator.h"
#include "ns3/network-module.h"
#include "../model/flowstar-control-message.h"
#include "../utils/metrics-collector.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("FlowStarSender");

NS_OBJECT_ENSURE_REGISTERED (FlowStarSender);

TypeId
FlowStarSender::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::FlowStarSender")
    .SetParent<Application> ()
    .SetGroupName ("Applications")
    .AddConstructor<FlowStarSender> ()
  ;
  return tid;
}

FlowStarSender::FlowStarSender ()
  : m_packetSize (1000),
    m_numPackets (0),
    m_packetsSent (0),
    m_creditPort (0),
    m_cnpPort (0),
    m_becnPort (0)
{
}

FlowStarSender::~FlowStarSender ()
{
}

void
FlowStarSender::DoDispose (void)
{
  m_dataSocket = 0;
  m_creditSocket = 0;
  m_cnpSocket = 0;
  m_becnSocket = 0;
  m_creditManager = 0;
  m_rateController = 0;
  Application::DoDispose ();
}

void
FlowStarSender::Setup (FlowStarMid mid, Address peer, uint32_t packetSize, uint32_t numPackets, DataRate lineRate)
{
  m_mid = mid;
  m_peer = peer;
  m_packetSize = packetSize;
  m_numPackets = numPackets;
  m_lineRate = lineRate;
}

void
FlowStarSender::RegisterCreditManager (Ptr<CbfcCreditManager> creditManager)
{
  m_creditManager = creditManager;
}

void
FlowStarSender::RegisterRateController (Ptr<FlowStarRateController> rateController)
{
  m_rateController = rateController;
}

void
FlowStarSender::SetListenPorts (uint16_t creditPort, uint16_t cnpPort, uint16_t becnPort)
{
  m_creditPort = creditPort;
  m_cnpPort = cnpPort;
  m_becnPort = becnPort;
}

void
FlowStarSender::StartApplication (void)
{
  if (m_rateController)
    m_rateController->InitializeFlow (m_mid, m_lineRate);

  // Setup sockets
  m_dataSocket = Socket::CreateSocket (GetNode (), TypeId::LookupByName ("ns3::UdpSocketFactory"));
  m_dataSocket->Connect (m_peer);

  m_creditSocket = Socket::CreateSocket (GetNode (), TypeId::LookupByName ("ns3::UdpSocketFactory"));
  m_creditSocket->Bind (InetSocketAddress (Ipv4Address::GetAny (), m_creditPort));
  m_creditSocket->SetRecvCallback (MakeCallback (&FlowStarSender::HandleCredit, this));

  m_cnpSocket = Socket::CreateSocket (GetNode (), TypeId::LookupByName ("ns3::UdpSocketFactory"));
  m_cnpSocket->Bind (InetSocketAddress (Ipv4Address::GetAny (), m_cnpPort));
  m_cnpSocket->SetRecvCallback (MakeCallback (&FlowStarSender::HandleCnp, this));

  m_becnSocket = Socket::CreateSocket (GetNode (), TypeId::LookupByName ("ns3::UdpSocketFactory"));
  m_becnSocket->Bind (InetSocketAddress (Ipv4Address::GetAny (), m_becnPort));
  m_becnSocket->SetRecvCallback (MakeCallback (&FlowStarSender::HandleBecn, this));

  uint64_t expectedBytes = static_cast<uint64_t>(m_numPackets) * m_packetSize;
  MetricsCollector::GetInstance().FlowStart(m_mid.srcFlowId, expectedBytes);

  ScheduleNextTx ();
}

void
FlowStarSender::StopApplication (void)
{
  if (!m_sendEvent.IsExpired ())
    {
      Simulator::Cancel (m_sendEvent);
    }

  if (m_dataSocket) m_dataSocket->Close ();
  if (m_creditSocket) m_creditSocket->Close ();
  if (m_cnpSocket) m_cnpSocket->Close ();
  if (m_becnSocket) m_becnSocket->Close ();
}

void
FlowStarSender::ScheduleNextTx (void)
{
  if (m_numPackets > 0 && m_packetsSent >= m_numPackets)
    return;

  // Credit Gate
  if (m_creditManager && !m_creditManager->CanTransmit (m_mid, m_packetSize))
    {
      NS_LOG_DEBUG ("Credit exhausted for MID " << m_mid.srcFlowId << "->" << m_mid.dstFlowId << " Waiting...");
      return; // Will be re-triggered by HandleCredit
    }

  // Rate Gate (Pacing)
  DataRate currentRate = m_lineRate;
  if (m_rateController)
    {
      currentRate = m_rateController->GetCurrentRate (m_mid);
    }
  
  Time txTime = currentRate.CalculateBytesTxTime (m_packetSize);
  
  // We can just schedule the packet send at current Time + txTime, 
  // or right away if we are not busy. Since we schedule sequentially:
  m_sendEvent = Simulator::Schedule (txTime, &FlowStarSender::SendPacket, this);
}

void
FlowStarSender::SendPacket (void)
{
  if (m_numPackets > 0 && m_packetsSent >= m_numPackets)
    return;

  // Re-check credits just in case
  if (m_creditManager && !m_creditManager->CanTransmit (m_mid, m_packetSize))
    {
      return; // Wait for credit
    }

  Ptr<Packet> pkt = Create<Packet> (m_packetSize);
  FlowStarMidTag tag (m_mid);
  pkt->AddPacketTag (tag);

  m_dataSocket->Send (pkt);

  if (m_creditManager)
    m_creditManager->ConsumeCredit (m_mid, m_packetSize);

  m_packetsSent++;

  ScheduleNextTx ();
}

void
FlowStarSender::HandleCredit (Ptr<Socket> socket)
{
  Ptr<Packet> pkt;
  Address from;
  while ((pkt = socket->RecvFrom (from)))
    {
      CreditUpdateHeader hdr;
      pkt->RemoveHeader (hdr);
      
      if (hdr.GetMid () == m_mid && m_creditManager)
        {
          m_creditManager->ReplenishCredit (m_mid, hdr.GetBytesFreed ());
          
          // If we were blocked on credits, this might unblock us
          if (m_sendEvent.IsExpired ())
            {
              ScheduleNextTx ();
            }
        }
    }
}

void
FlowStarSender::HandleCnp (Ptr<Socket> socket)
{
  Ptr<Packet> pkt;
  Address from;
  while ((pkt = socket->RecvFrom (from)))
    {
      FlowStarCnpHeader hdr;
      pkt->RemoveHeader (hdr);
      
      if (hdr.GetMid () == m_mid && m_rateController)
        {
          m_rateController->OnCnp (m_mid, hdr.GetReceivingRateBps ());
        }
    }
}

void
FlowStarSender::HandleBecn (Ptr<Socket> socket)
{
  Ptr<Packet> pkt;
  Address from;
  while ((pkt = socket->RecvFrom (from)))
    {
      FlowStarBecnHeader becnHdr;
      if (pkt->PeekHeader (becnHdr))
        {
          NS_LOG_INFO ("FlowStarSender Received BECN for mid " << becnHdr.GetMid().srcFlowId << "->" << becnHdr.GetMid().dstFlowId);
          m_rateController->OnBecn (becnHdr.GetMid(), becnHdr.GetQueueOccupancy());
          m_rateController->RecordBecnPath (becnHdr.GetMid(), becnHdr.GetSwitchId());
        }
    }
}

} // namespace ns3
