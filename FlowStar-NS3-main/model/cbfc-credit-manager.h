#ifndef CBFC_CREDIT_MANAGER_H
#define CBFC_CREDIT_MANAGER_H

#include "flowstar-mid.h"
#include "ns3/object.h"
#include <map>

namespace ns3 {

/**
 * \brief Per-flow credit state for CBFC, following the FlowStar paper.
 *
 * Per-flow variables maintained:
 *
 *   FCTBS (Flow Credit Token Bucket Size)
 *     Current available credit in bytes. Decreases on each send.
 *     Increases when a CreditUpdate is received (bounded by FCCL).
 *
 *   FCCL (Flow Credit Ceiling Level)
 *     Maximum allowed credit for this flow, set by switch based on
 *     buffer allocation. FCTBS may never exceed FCCL.
 *
 *   BufferFree
 *     Cached estimate of free buffer space at the switch for this flow.
 *     Updated when the switch reports buffer state.
 *
 *   ABR (Allowed Byte Rate)
 *     Byte accounting variable tracking the allowed transmission budget.
 *     Updated alongside FCTBS to maintain rate accounting.
 *
 * Operations:
 *   InitializeFlow   - set up initial FCTBS and FCCL
 *   CanTransmit      - returns true iff FCTBS >= packetSize
 *   ConsumeCredit    - called on each send; decreases FCTBS and ABR
 *   ReplenishCredit  - called when credit update arrives; FCTBS += bytes, capped at FCCL
 *   UpdateBufferState- update cached BufferFree
 */
class CbfcCreditManager : public Object
{
public:
  static TypeId GetTypeId (void);

  CbfcCreditManager ();
  virtual ~CbfcCreditManager ();

  // ---- Per-flow lifecycle ----
  void InitializeFlow (FlowStarMid mid, uint32_t initialCredit, uint32_t fccl);

  // ---- Sender-side credit interface ----
  bool     CanTransmit (FlowStarMid mid, uint32_t packetSize) const;
  void     ConsumeCredit (FlowStarMid mid, uint32_t packetSize);
  void     ReplenishCredit (FlowStarMid mid, uint32_t bytes);

  // ---- Switch-side state (cached at sender) ----
  void     UpdateBufferState (FlowStarMid mid, uint32_t bufferFree);

  // ---- Accessors ----
  uint32_t GetAvailableCredit (FlowStarMid mid) const;   // returns FCTBS
  uint32_t GetCreditCeiling   (FlowStarMid mid) const;   // returns FCCL
  uint32_t GetBufferFree      (FlowStarMid mid) const;   // returns BufferFree
  uint64_t GetABR             (FlowStarMid mid) const;   // returns ABR

  bool     IsFlowKnown (FlowStarMid mid) const;

private:
  struct PerFlowCredit {
    uint32_t fctbs;       ///< Current available credit (bytes)
    uint32_t fccl;        ///< Maximum allowed credit ceiling (bytes)
    uint32_t bufferFree;  ///< Cached switch buffer free state (bytes)
    uint64_t abr;         ///< Allowed Byte Rate (cumulative bytes allowed to send)
  };

  std::map<FlowStarMid, PerFlowCredit> m_flows;
};

} // namespace ns3

#endif /* CBFC_CREDIT_MANAGER_H */
