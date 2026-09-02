#ifndef CBFC_QUEUE_DISC_H
#define CBFC_QUEUE_DISC_H

#include "flowstar-mid.h"
#include "ns3/queue-disc.h"
#include "ns3/drop-tail-queue.h"
#include "ns3/callback.h"
#include <map>
#include <set>
#include <vector>

namespace ns3 {

/**
 * \brief Per-flow queue disc implementing Dynamic Queue Assignment and
 *        Round-Robin scheduling, as described in the FlowStar paper.
 *
 * Dynamic Queue Assignment (DQA) algorithm:
 *
 *   packet arrives
 *         │
 *     extract FlowStarMidTag
 *         │
 *     MessageTable.lookup(mid)
 *         │
 *     existing? ──yes──→ use existing queueIndex
 *         │no
 *         │
 *     scan QueueTable for unoccupied queue ──found──→ dedicated assign
 *         │not found
 *         │
 *     choose least-loaded queue (by occupancy) ──→ shared assign
 *
 * QueueEntry tracks activeMids (a set) so that multiple MIDs can share
 * a single physical queue after exhaustion without losing state.
 * isDedicated() returns true iff activeMids.size() == 1.
 *
 * Scheduler: Round-Robin across non-empty queues (one packet per queue
 * per round). Documented as Phase 3 scheduler abstraction.
 *
 * Credit callback: on each DoDequeue, fires CreditUpdateCallback with
 * (mid, bytesFreed) so the experiment can route credit updates back
 * to senders.
 */
class CbfcQueueDisc : public QueueDisc
{
public:
  static TypeId GetTypeId (void);

  CbfcQueueDisc ();
  ~CbfcQueueDisc () override;

  /// Set number of physical queues (must be called before Initialize)
  void SetNumQueues (uint32_t n);
  uint32_t GetNumQueues (void) const;

  /// Set queue size per flow-queue (packets)
  void SetQueueSize (uint32_t packets);

  /// Register a callback fired on each dequeue: (mid, bytesFreed)
  void SetCreditUpdateCallback (Callback<void, FlowStarMid, uint32_t> cb);

  /// Register a callback fired on each enqueue: (mid, newOccupancy)
  void SetEnqueueCallback (Callback<void, FlowStarMid, uint32_t> cb);

  // ---- Queue state queries ----
  /// Returns the queue index assigned to mid, or UINT32_MAX if unmapped
  uint32_t GetQueueIndex (FlowStarMid mid) const;

  /// Returns true if the queue at idx has exactly one assigned MID
  bool IsDedicatedQueue (uint32_t queueIdx) const;

  /// Returns the set of MIDs currently sharing queue idx
  std::set<FlowStarMid> GetActiveMids (uint32_t queueIdx) const;

  /// Returns current occupancy of queue idx (packets)
  uint32_t GetQueueOccupancy (uint32_t queueIdx) const;

private:
  bool DoEnqueue (Ptr<QueueDiscItem> item) override;
  Ptr<QueueDiscItem> DoDequeue (void) override;
  Ptr<const QueueDiscItem> DoPeek (void) override;
  bool CheckConfig (void) override;
  void InitializeParams (void) override;

protected:
  virtual uint32_t AssignQueue (FlowStarMid mid);

  struct QueueEntry {
    uint32_t              occupancy;
    std::set<FlowStarMid> activeMids;

    bool isDedicated () const { return activeMids.size () == 1; }
  };

  uint32_t m_numQueues;
  uint32_t m_queueSize;     ///< max packets per internal queue
  uint32_t m_rrPtr;         ///< round-robin pointer

  std::map<FlowStarMid, uint32_t>  m_messageTable;  ///< MID → queue index
  std::vector<QueueEntry>          m_queueTable;

private:
  Callback<void, FlowStarMid, uint32_t> m_creditCallback;
  bool m_hasCreditCallback;

  Callback<void, FlowStarMid, uint32_t> m_enqueueCallback;
  bool m_hasEnqueueCallback;
};

} // namespace ns3

#endif /* CBFC_QUEUE_DISC_H */
