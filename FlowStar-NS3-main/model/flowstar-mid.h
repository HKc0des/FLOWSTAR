#ifndef FLOWSTAR_MID_H
#define FLOWSTAR_MID_H

#include "ns3/tag.h"
#include "ns3/packet.h"
#include <stdint.h>

namespace ns3 {

/**
 * \brief Simulation-level representation of the FlowStar Message ID (MID).
 *
 * Maps conceptually to the paper's:
 *   srcNodeId  → simulation equivalent of source LID
 *   dstNodeId  → simulation equivalent of destination LID
 *   srcFlowId  → simulation equivalent of source QP
 *   dstFlowId  → simulation equivalent of destination QP
 *
 * NOTE: These fields are NOT actual InfiniBand LIDs or QP numbers.
 * They are simulation-level identifiers capturing the semantically equivalent
 * per-message identity described in the FlowStar paper.
 */
struct FlowStarMid
{
  uint32_t srcNodeId;  ///< simulation equivalent of source LID
  uint32_t dstNodeId;  ///< simulation equivalent of destination LID
  uint32_t srcFlowId;  ///< simulation equivalent of source QP
  uint32_t dstFlowId;  ///< simulation equivalent of destination QP

  FlowStarMid () : srcNodeId(0), dstNodeId(0), srcFlowId(0), dstFlowId(0) {}
  FlowStarMid (uint32_t src, uint32_t dst, uint32_t sFlow, uint32_t dFlow)
    : srcNodeId(src), dstNodeId(dst), srcFlowId(sFlow), dstFlowId(dFlow) {}

  bool operator== (const FlowStarMid& o) const {
    return srcNodeId == o.srcNodeId && dstNodeId == o.dstNodeId &&
           srcFlowId == o.srcFlowId && dstFlowId == o.dstFlowId;
  }

  bool operator!= (const FlowStarMid& o) const { return !(*this == o); }

  /// Total order for use in std::map/std::set
  bool operator< (const FlowStarMid& o) const {
    if (srcNodeId != o.srcNodeId) return srcNodeId < o.srcNodeId;
    if (dstNodeId != o.dstNodeId) return dstNodeId < o.dstNodeId;
    if (srcFlowId != o.srcFlowId) return srcFlowId < o.srcFlowId;
    return dstFlowId < o.dstFlowId;
  }
};

/**
 * \brief Packet tag carrying a FlowStarMid.
 *
 * Added by CbfcTrafficSender to every data packet so that
 * CbfcQueueDisc can identify the MID at the switch and perform
 * Dynamic Queue Assignment.
 */
class FlowStarMidTag : public Tag
{
public:
  static TypeId GetTypeId (void);
  virtual TypeId GetInstanceTypeId (void) const;
  virtual uint32_t GetSerializedSize (void) const;
  virtual void Serialize (TagBuffer i) const;
  virtual void Deserialize (TagBuffer i);
  virtual void Print (std::ostream &os) const;

  FlowStarMidTag ();
  explicit FlowStarMidTag (const FlowStarMid& mid);

  void SetMid (const FlowStarMid& mid);
  FlowStarMid GetMid (void) const;

private:
  FlowStarMid m_mid;
};

/**
 * \brief Header for Credit Update control packets.
 *
 * Sent from the receiver (Phase 3: receiver-originated credits, analogous
 * to buffer-freed events at the switch) back to the sender.
 * The sender uses this to replenish FCTBS up to FCCL.
 */
class CreditUpdateHeader : public Header
{
public:
  static TypeId GetTypeId (void);
  virtual TypeId GetInstanceTypeId (void) const;
  virtual uint32_t GetSerializedSize (void) const;
  virtual void Serialize (Buffer::Iterator start) const;
  virtual uint32_t Deserialize (Buffer::Iterator start);
  virtual void Print (std::ostream &os) const;

  CreditUpdateHeader ();
  CreditUpdateHeader (FlowStarMid mid, uint32_t bytesFreed);

  void SetMid (const FlowStarMid& mid);
  FlowStarMid GetMid (void) const;

  void SetBytesFreed (uint32_t bytes);
  uint32_t GetBytesFreed (void) const;

private:
  FlowStarMid m_mid;
  uint32_t    m_bytesFreed;
};

} // namespace ns3

#endif /* FLOWSTAR_MID_H */
