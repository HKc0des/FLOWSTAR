#include "flowstar-switch-agent.h"
#include "ns3/log.h"
#include "ns3/simulator.h"
#include "ns3/network-module.h"
#include "../model/flowstar-control-message.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("FlowStarSwitchAgent");

NS_OBJECT_ENSURE_REGISTERED (FlowStarSwitchAgent);

TypeId
FlowStarSwitchAgent::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::FlowStarSwitchAgent")
    .SetParent<Application> ()
    .SetGroupName ("Applications")
    .AddConstructor<FlowStarSwitchAgent> ()
    .AddAttribute ("BecnThreshold",
                   "Queue occupancy threshold for generating BECN (packets)",
                   UintegerValue (10),
                   MakeUintegerAccessor (&FlowStarSwitchAgent::m_becnThreshold),
                   MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("BecnInterval",
                   "Minimum time between BECNs for the same flow",
                   TimeValue (MicroSeconds (50)),
                   MakeTimeAccessor (&FlowStarSwitchAgent::m_becnInterval),
                   MakeTimeChecker ())
  ;
  return tid;
}

FlowStarSwitchAgent::FlowStarSwitchAgent ()
  : m_qdisc (0),
    m_switchId (0)
{
}

FlowStarSwitchAgent::~FlowStarSwitchAgent ()
{
}

void
FlowStarSwitchAgent::DoDispose (void)
{
  m_qdisc = 0;
  Application::DoDispose ();
}

void
FlowStarSwitchAgent::SetQueueDisc (Ptr<CbfcQueueDisc> qdisc)
{
  m_qdisc = qdisc;
}

void
FlowStarSwitchAgent::RegisterSenderBecn (FlowStarMid mid, Address senderBecnAddr)
{
  m_senderBecnAddrs[mid] = senderBecnAddr;
}

void
FlowStarSwitchAgent::SetSwitchId (uint32_t id)
{
  m_switchId = id;
}

void
FlowStarSwitchAgent::StartApplication (void)
{
  if (m_qdisc)
    {
      m_qdisc->SetEnqueueCallback (MakeCallback (&FlowStarSwitchAgent::OnEnqueue, this));
    }
}

void
FlowStarSwitchAgent::StopApplication (void)
{
  if (m_qdisc)
    {
      m_qdisc->SetEnqueueCallback (MakeNullCallback<void, FlowStarMid, uint32_t> ());
    }
}

void
FlowStarSwitchAgent::OnEnqueue (FlowStarMid mid, uint32_t occupancy)
{
  if (occupancy <= m_becnThreshold)
    return;

  // We have crossed the threshold. Check rate limit.
  Time now = Simulator::Now ();
  auto itTime = m_lastBecnTimes.find (mid);
  if (itTime != m_lastBecnTimes.end ())
    {
      if ((now - itTime->second) < m_becnInterval)
        {
          return; // Too soon for another BECN
        }
    }

  // Generate BECN
  auto itAddr = m_senderBecnAddrs.find (mid);
  if (itAddr != m_senderBecnAddrs.end ())
    {
      NS_LOG_INFO ("Queue occupancy " << occupancy << " > threshold " << m_becnThreshold
                   << " for MID " << mid.srcFlowId << "->" << mid.dstFlowId
                   << " at Switch " << m_switchId << ". Sending BECN.");
      m_lastBecnTimes[mid] = now;
      SendBecn (mid, occupancy, itAddr->second);
    }
}

void
FlowStarSwitchAgent::SendBecn (FlowStarMid mid, uint32_t occupancy, Address destAddr)
{
  Ptr<Socket> sock = Socket::CreateSocket (GetNode (), TypeId::LookupByName ("ns3::UdpSocketFactory"));
  sock->Connect (destAddr);

  Ptr<Packet> ctrlPkt = Create<Packet> (0);
  FlowStarBecnHeader hdr (mid, occupancy, m_switchId);
  ctrlPkt->AddHeader (hdr);

  sock->Send (ctrlPkt);
  sock->Close ();
}

} // namespace ns3
