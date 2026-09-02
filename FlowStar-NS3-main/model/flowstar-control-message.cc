#include "flowstar-control-message.h"

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (FlowStarCnpHeader);

TypeId
FlowStarCnpHeader::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::FlowStarCnpHeader")
    .SetParent<Header> ()
    .SetGroupName ("FlowStar")
    .AddConstructor<FlowStarCnpHeader> ()
  ;
  return tid;
}

TypeId
FlowStarCnpHeader::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}

FlowStarCnpHeader::FlowStarCnpHeader ()
  : m_mid (0,0,0,0), m_receivingRateBps (0), m_timestamp (0)
{
}

FlowStarCnpHeader::FlowStarCnpHeader (FlowStarMid mid, uint64_t rateBps, uint64_t timestamp)
  : m_mid (mid), m_receivingRateBps (rateBps), m_timestamp (timestamp)
{
}

FlowStarCnpHeader::~FlowStarCnpHeader ()
{
}

void
FlowStarCnpHeader::Print (std::ostream &os) const
{
  os << "FlowStarCnp (MID=" << m_mid.srcFlowId << "->" << m_mid.dstFlowId
     << " Rate=" << m_receivingRateBps << " bps"
     << " TS=" << m_timestamp << ")";
}

uint32_t
FlowStarCnpHeader::GetSerializedSize (void) const
{
  return 4 * 4 + 8 + 8; // mid(4x32bit) + rate(64bit) + ts(64bit)
}

void
FlowStarCnpHeader::Serialize (Buffer::Iterator start) const
{
  start.WriteHtonU32 (m_mid.srcNodeId);
  start.WriteHtonU32 (m_mid.dstNodeId);
  start.WriteHtonU32 (m_mid.srcFlowId);
  start.WriteHtonU32 (m_mid.dstFlowId);
  start.WriteHtonU64 (m_receivingRateBps);
  start.WriteHtonU64 (m_timestamp);
}

uint32_t
FlowStarCnpHeader::Deserialize (Buffer::Iterator start)
{
  m_mid.srcNodeId = start.ReadNtohU32 ();
  m_mid.dstNodeId = start.ReadNtohU32 ();
  m_mid.srcFlowId = start.ReadNtohU32 ();
  m_mid.dstFlowId = start.ReadNtohU32 ();
  m_receivingRateBps = start.ReadNtohU64 ();
  m_timestamp = start.ReadNtohU64 ();
  return GetSerializedSize ();
}

// ----------------------------------------------------------------------------
// FlowStarBecnHeader
// ----------------------------------------------------------------------------

NS_OBJECT_ENSURE_REGISTERED (FlowStarBecnHeader);

TypeId
FlowStarBecnHeader::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::FlowStarBecnHeader")
    .SetParent<Header> ()
    .SetGroupName ("FlowStar")
    .AddConstructor<FlowStarBecnHeader> ()
  ;
  return tid;
}

TypeId
FlowStarBecnHeader::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}

FlowStarBecnHeader::FlowStarBecnHeader ()
  : m_mid (0,0,0,0), m_queueOccupancy (0), m_switchId (0)
{
}

FlowStarBecnHeader::FlowStarBecnHeader (FlowStarMid mid, uint32_t qOccupancy, uint32_t sId)
  : m_mid (mid), m_queueOccupancy (qOccupancy), m_switchId (sId)
{
}

FlowStarBecnHeader::~FlowStarBecnHeader ()
{
}

void
FlowStarBecnHeader::Print (std::ostream &os) const
{
  os << "FlowStarBecn (MID=" << m_mid.srcFlowId << "->" << m_mid.dstFlowId
     << " QOcc=" << m_queueOccupancy
     << " Switch=" << m_switchId << ")";
}

uint32_t
FlowStarBecnHeader::GetSerializedSize (void) const
{
  return 4 * 4 + 4 + 4; // mid(4x32bit) + queue(32bit) + switchId(32bit)
}

void
FlowStarBecnHeader::Serialize (Buffer::Iterator start) const
{
  start.WriteHtonU32 (m_mid.srcNodeId);
  start.WriteHtonU32 (m_mid.dstNodeId);
  start.WriteHtonU32 (m_mid.srcFlowId);
  start.WriteHtonU32 (m_mid.dstFlowId);
  start.WriteHtonU32 (m_queueOccupancy);
  start.WriteHtonU32 (m_switchId);
}

uint32_t
FlowStarBecnHeader::Deserialize (Buffer::Iterator start)
{
  m_mid.srcNodeId = start.ReadNtohU32 ();
  m_mid.dstNodeId = start.ReadNtohU32 ();
  m_mid.srcFlowId = start.ReadNtohU32 ();
  m_mid.dstFlowId = start.ReadNtohU32 ();
  m_queueOccupancy = start.ReadNtohU32 ();
  m_switchId = start.ReadNtohU32 ();
  return GetSerializedSize ();
}

} // namespace ns3
