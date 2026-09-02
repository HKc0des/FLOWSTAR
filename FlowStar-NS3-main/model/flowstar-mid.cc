#include "flowstar-mid.h"
#include "ns3/log.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("FlowStarMid");

// ===================== FlowStarMidTag =====================

NS_OBJECT_ENSURE_REGISTERED (FlowStarMidTag);

TypeId
FlowStarMidTag::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::FlowStarMidTag")
    .SetParent<Tag> ()
    .SetGroupName ("FlowStar")
    .AddConstructor<FlowStarMidTag> ()
  ;
  return tid;
}

TypeId FlowStarMidTag::GetInstanceTypeId (void) const { return GetTypeId (); }

uint32_t FlowStarMidTag::GetSerializedSize (void) const
{
  return 4 * sizeof (uint32_t); // srcNodeId, dstNodeId, srcFlowId, dstFlowId
}

void FlowStarMidTag::Serialize (TagBuffer i) const
{
  i.WriteU32 (m_mid.srcNodeId);
  i.WriteU32 (m_mid.dstNodeId);
  i.WriteU32 (m_mid.srcFlowId);
  i.WriteU32 (m_mid.dstFlowId);
}

void FlowStarMidTag::Deserialize (TagBuffer i)
{
  m_mid.srcNodeId = i.ReadU32 ();
  m_mid.dstNodeId = i.ReadU32 ();
  m_mid.srcFlowId = i.ReadU32 ();
  m_mid.dstFlowId = i.ReadU32 ();
}

void FlowStarMidTag::Print (std::ostream &os) const
{
  os << "MID=(" << m_mid.srcNodeId << "," << m_mid.dstNodeId
     << "," << m_mid.srcFlowId << "," << m_mid.dstFlowId << ")";
}

FlowStarMidTag::FlowStarMidTag () {}
FlowStarMidTag::FlowStarMidTag (const FlowStarMid& mid) : m_mid (mid) {}

void FlowStarMidTag::SetMid (const FlowStarMid& mid) { m_mid = mid; }
FlowStarMid FlowStarMidTag::GetMid (void) const { return m_mid; }

// ===================== CreditUpdateHeader =====================

NS_OBJECT_ENSURE_REGISTERED (CreditUpdateHeader);

TypeId
CreditUpdateHeader::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::CreditUpdateHeader")
    .SetParent<Header> ()
    .SetGroupName ("FlowStar")
    .AddConstructor<CreditUpdateHeader> ()
  ;
  return tid;
}

TypeId CreditUpdateHeader::GetInstanceTypeId (void) const { return GetTypeId (); }

uint32_t CreditUpdateHeader::GetSerializedSize (void) const
{
  return 4 * sizeof (uint32_t) + sizeof (uint32_t); // 4 MID fields + bytesFreed
}

void CreditUpdateHeader::Serialize (Buffer::Iterator start) const
{
  start.WriteHtonU32 (m_mid.srcNodeId);
  start.WriteHtonU32 (m_mid.dstNodeId);
  start.WriteHtonU32 (m_mid.srcFlowId);
  start.WriteHtonU32 (m_mid.dstFlowId);
  start.WriteHtonU32 (m_bytesFreed);
}

uint32_t CreditUpdateHeader::Deserialize (Buffer::Iterator start)
{
  m_mid.srcNodeId = start.ReadNtohU32 ();
  m_mid.dstNodeId = start.ReadNtohU32 ();
  m_mid.srcFlowId = start.ReadNtohU32 ();
  m_mid.dstFlowId = start.ReadNtohU32 ();
  m_bytesFreed    = start.ReadNtohU32 ();
  return GetSerializedSize ();
}

void CreditUpdateHeader::Print (std::ostream &os) const
{
  os << "CreditUpdate MID=(" << m_mid.srcNodeId << "," << m_mid.dstNodeId
     << "," << m_mid.srcFlowId << "," << m_mid.dstFlowId
     << ") bytesFreed=" << m_bytesFreed;
}

CreditUpdateHeader::CreditUpdateHeader () : m_bytesFreed (0) {}
CreditUpdateHeader::CreditUpdateHeader (FlowStarMid mid, uint32_t bytesFreed)
  : m_mid (mid), m_bytesFreed (bytesFreed) {}

void CreditUpdateHeader::SetMid (const FlowStarMid& mid) { m_mid = mid; }
FlowStarMid CreditUpdateHeader::GetMid (void) const { return m_mid; }
void CreditUpdateHeader::SetBytesFreed (uint32_t bytes) { m_bytesFreed = bytes; }
uint32_t CreditUpdateHeader::GetBytesFreed (void) const { return m_bytesFreed; }

} // namespace ns3
