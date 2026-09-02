#include "baseline-control-message.h"
#include "ns3/log.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("BaselineControlMessage");

// ===================== EcnTag =====================

NS_OBJECT_ENSURE_REGISTERED (EcnTag);

TypeId
EcnTag::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::EcnTag")
    .SetParent<Tag> ()
    .SetGroupName("FlowStar")
    .AddConstructor<EcnTag> ()
  ;
  return tid;
}

TypeId
EcnTag::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}

uint32_t
EcnTag::GetSerializedSize (void) const
{
  return 1; // 1 byte for the boolean
}

void
EcnTag::Serialize (TagBuffer i) const
{
  i.WriteU8 (m_congestionExperienced ? 1 : 0);
}

void
EcnTag::Deserialize (TagBuffer i)
{
  m_congestionExperienced = (i.ReadU8 () == 1);
}

void
EcnTag::Print (std::ostream &os) const
{
  os << "ECN=" << (m_congestionExperienced ? "CE" : "NotCE");
}

EcnTag::EcnTag ()
  : m_congestionExperienced (false)
{
}

EcnTag::EcnTag (bool congestionExperienced)
  : m_congestionExperienced (congestionExperienced)
{
}

void
EcnTag::SetCongestionExperienced (bool ce)
{
  m_congestionExperienced = ce;
}

bool
EcnTag::GetCongestionExperienced (void) const
{
  return m_congestionExperienced;
}

// ===================== CnpHeader =====================

NS_OBJECT_ENSURE_REGISTERED (CnpHeader);

TypeId
CnpHeader::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::CnpHeader")
    .SetParent<Header> ()
    .SetGroupName("FlowStar")
    .AddConstructor<CnpHeader> ()
  ;
  return tid;
}

TypeId
CnpHeader::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}

uint32_t
CnpHeader::GetSerializedSize (void) const
{
  return 4; // 4 bytes for flowId
}

void
CnpHeader::Serialize (Buffer::Iterator start) const
{
  start.WriteHtonU32 (m_flowId);
}

uint32_t
CnpHeader::Deserialize (Buffer::Iterator start)
{
  m_flowId = start.ReadNtohU32 ();
  return GetSerializedSize ();
}

void
CnpHeader::Print (std::ostream &os) const
{
  os << "CNP flowId=" << m_flowId;
}

CnpHeader::CnpHeader ()
  : m_flowId (0)
{
}

CnpHeader::CnpHeader (uint32_t flowId)
  : m_flowId (flowId)
{
}

void
CnpHeader::SetFlowId (uint32_t flowId)
{
  m_flowId = flowId;
}

uint32_t
CnpHeader::GetFlowId (void) const
{
  return m_flowId;
}

} // namespace ns3
