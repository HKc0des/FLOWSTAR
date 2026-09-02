#include "basic-flow-id-tag.h"
#include "ns3/core-module.h"

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (BasicFlowIdTag);

TypeId
BasicFlowIdTag::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::BasicFlowIdTag")
    .SetParent<Tag> ()
    .SetGroupName("Network")
    .AddConstructor<BasicFlowIdTag> ()
  ;
  return tid;
}

TypeId
BasicFlowIdTag::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}

uint32_t
BasicFlowIdTag::GetSerializedSize (void) const
{
  return 4 + 4 + 8; // flowId (4), seqNum (4), timestamp (8)
}

void
BasicFlowIdTag::Serialize (TagBuffer i) const
{
  i.WriteU32 (m_flowId);
  i.WriteU32 (m_seqNum);
  i.WriteU64 (m_timestamp);
}

void
BasicFlowIdTag::Deserialize (TagBuffer i)
{
  m_flowId = i.ReadU32 ();
  m_seqNum = i.ReadU32 ();
  m_timestamp = i.ReadU64 ();
}

void
BasicFlowIdTag::Print (std::ostream &os) const
{
  os << "FlowId=" << m_flowId 
     << " SeqNum=" << m_seqNum 
     << " Timestamp=" << m_timestamp;
}

BasicFlowIdTag::BasicFlowIdTag ()
  : m_flowId (0),
    m_seqNum (0),
    m_timestamp (0)
{
}

BasicFlowIdTag::BasicFlowIdTag (uint32_t flowId, uint32_t seqNum, uint64_t generationTime)
  : m_flowId (flowId),
    m_seqNum (seqNum),
    m_timestamp (generationTime)
{
}

void
BasicFlowIdTag::SetFlowId (uint32_t flowId)
{
  m_flowId = flowId;
}

uint32_t
BasicFlowIdTag::GetFlowId (void) const
{
  return m_flowId;
}

void
BasicFlowIdTag::SetSequenceNumber (uint32_t seqNum)
{
  m_seqNum = seqNum;
}

uint32_t
BasicFlowIdTag::GetSequenceNumber (void) const
{
  return m_seqNum;
}

void
BasicFlowIdTag::SetGenerationTimestamp (uint64_t timestamp)
{
  m_timestamp = timestamp;
}

uint64_t
BasicFlowIdTag::GetGenerationTimestamp (void) const
{
  return m_timestamp;
}

} // namespace ns3
