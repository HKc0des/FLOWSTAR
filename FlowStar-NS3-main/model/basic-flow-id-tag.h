#ifndef BASIC_FLOW_ID_TAG_H
#define BASIC_FLOW_ID_TAG_H

#include "ns3/tag.h"
#include "ns3/packet.h"

namespace ns3 {

/**
 * \brief A Packet tag used to identify the flow to which a packet belongs.
 *
 * It contains the FlowId, SequenceNumber, and a GenerationTimestamp.
 * This is deliberately kept simple for Phase 1.
 */
class BasicFlowIdTag : public Tag {
public:
  static TypeId GetTypeId (void);
  virtual TypeId GetInstanceTypeId (void) const;
  virtual uint32_t GetSerializedSize (void) const;
  virtual void Serialize (TagBuffer i) const;
  virtual void Deserialize (TagBuffer i);
  virtual void Print (std::ostream &os) const;

  BasicFlowIdTag ();
  BasicFlowIdTag (uint32_t flowId, uint32_t seqNum, uint64_t generationTime);

  void SetFlowId (uint32_t flowId);
  uint32_t GetFlowId (void) const;

  void SetSequenceNumber (uint32_t seqNum);
  uint32_t GetSequenceNumber (void) const;

  void SetGenerationTimestamp (uint64_t timestamp);
  uint64_t GetGenerationTimestamp (void) const;

private:
  uint32_t m_flowId;
  uint32_t m_seqNum;
  uint64_t m_timestamp; // nanoseconds since simulation start
};

} // namespace ns3

#endif /* BASIC_FLOW_ID_TAG_H */
