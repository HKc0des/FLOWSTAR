#include "cbfc-credit-manager.h"
#include "ns3/log.h"
#include "ns3/assert.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("CbfcCreditManager");

NS_OBJECT_ENSURE_REGISTERED (CbfcCreditManager);

TypeId
CbfcCreditManager::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::CbfcCreditManager")
    .SetParent<Object> ()
    .SetGroupName ("FlowStar")
    .AddConstructor<CbfcCreditManager> ()
  ;
  return tid;
}

CbfcCreditManager::CbfcCreditManager ()
{
}

CbfcCreditManager::~CbfcCreditManager ()
{
}

void
CbfcCreditManager::InitializeFlow (FlowStarMid mid, uint32_t initialCredit, uint32_t fccl)
{
  NS_ASSERT_MSG (initialCredit <= fccl,
                 "Initial credit must not exceed FCCL ceiling");

  PerFlowCredit state;
  state.fctbs      = initialCredit;
  state.fccl       = fccl;
  state.bufferFree = fccl;     // initially all buffer is free
  state.abr        = 0;

  m_flows[mid] = state;

  NS_LOG_INFO ("InitializeFlow MID=(" << mid.srcNodeId << "," << mid.dstNodeId
               << "," << mid.srcFlowId << "," << mid.dstFlowId
               << ") FCTBS=" << initialCredit << " FCCL=" << fccl);
}

bool
CbfcCreditManager::CanTransmit (FlowStarMid mid, uint32_t packetSize) const
{
  auto it = m_flows.find (mid);
  if (it == m_flows.end ())
    {
      NS_LOG_WARN ("CanTransmit: unknown MID, allowing (no credit tracking)");
      return true;
    }
  return it->second.fctbs >= packetSize;
}

void
CbfcCreditManager::ConsumeCredit (FlowStarMid mid, uint32_t packetSize)
{
  auto it = m_flows.find (mid);
  NS_ASSERT_MSG (it != m_flows.end (), "ConsumeCredit: unknown MID");
  NS_ASSERT_MSG (it->second.fctbs >= packetSize,
                 "ConsumeCredit: insufficient FCTBS (" << it->second.fctbs
                 << " < " << packetSize << ")");

  it->second.fctbs -= packetSize;
  it->second.abr   += packetSize;

  NS_LOG_INFO ("ConsumeCredit MID=(" << mid.srcFlowId << "," << mid.dstFlowId
               << ") size=" << packetSize
               << " FCTBS=" << it->second.fctbs
               << " ABR=" << it->second.abr);
}

void
CbfcCreditManager::ReplenishCredit (FlowStarMid mid, uint32_t bytes)
{
  auto it = m_flows.find (mid);
  if (it == m_flows.end ())
    {
      NS_LOG_WARN ("ReplenishCredit: unknown MID, ignoring");
      return;
    }

  uint32_t newFctbs = it->second.fctbs + bytes;

  // FCTBS must never exceed FCCL
  if (newFctbs > it->second.fccl)
    {
      newFctbs = it->second.fccl;
    }

  NS_LOG_INFO ("ReplenishCredit MID=(" << mid.srcFlowId << "," << mid.dstFlowId
               << ") +" << bytes
               << " FCTBS: " << it->second.fctbs << " -> " << newFctbs
               << " (FCCL=" << it->second.fccl << ")");

  it->second.fctbs = newFctbs;
}

void
CbfcCreditManager::UpdateBufferState (FlowStarMid mid, uint32_t bufferFree)
{
  auto it = m_flows.find (mid);
  if (it == m_flows.end ())
    {
      NS_LOG_WARN ("UpdateBufferState: unknown MID, ignoring");
      return;
    }
  it->second.bufferFree = bufferFree;
  NS_LOG_INFO ("UpdateBufferState MID=(" << mid.srcFlowId << "," << mid.dstFlowId
               << ") bufferFree=" << bufferFree);
}

uint32_t
CbfcCreditManager::GetAvailableCredit (FlowStarMid mid) const
{
  auto it = m_flows.find (mid);
  if (it == m_flows.end ()) return 0;
  return it->second.fctbs;
}

uint32_t
CbfcCreditManager::GetCreditCeiling (FlowStarMid mid) const
{
  auto it = m_flows.find (mid);
  if (it == m_flows.end ()) return 0;
  return it->second.fccl;
}

uint32_t
CbfcCreditManager::GetBufferFree (FlowStarMid mid) const
{
  auto it = m_flows.find (mid);
  if (it == m_flows.end ()) return 0;
  return it->second.bufferFree;
}

uint64_t
CbfcCreditManager::GetABR (FlowStarMid mid) const
{
  auto it = m_flows.find (mid);
  if (it == m_flows.end ()) return 0;
  return it->second.abr;
}

bool
CbfcCreditManager::IsFlowKnown (FlowStarMid mid) const
{
  return m_flows.find (mid) != m_flows.end ();
}

} // namespace ns3
