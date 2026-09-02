#include "cbfc-queue-disc.h"
#include "ns3/log.h"
#include "ns3/uinteger.h"
#include "ns3/drop-tail-queue.h"
#include <climits>

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("CbfcQueueDisc");

NS_OBJECT_ENSURE_REGISTERED (CbfcQueueDisc);

TypeId
CbfcQueueDisc::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::CbfcQueueDisc")
    .SetParent<QueueDisc> ()
    .SetGroupName ("FlowStar")
    .AddConstructor<CbfcQueueDisc> ()
    .AddAttribute ("NumQueues",
                   "Number of physical per-flow queue slots",
                   UintegerValue (4),
                   MakeUintegerAccessor (&CbfcQueueDisc::m_numQueues),
                   MakeUintegerChecker<uint32_t> (1, 256))
    .AddAttribute ("QueueSize",
                   "Maximum number of packets per internal queue",
                   UintegerValue (1000),
                   MakeUintegerAccessor (&CbfcQueueDisc::m_queueSize),
                   MakeUintegerChecker<uint32_t> (1))
  ;
  return tid;
}

CbfcQueueDisc::CbfcQueueDisc ()
  : QueueDisc (QueueDiscSizePolicy::MULTIPLE_QUEUES),
    m_numQueues (4),
    m_queueSize (1000),
    m_rrPtr (0),
    m_hasCreditCallback (false),
    m_hasEnqueueCallback (false)
{
}

CbfcQueueDisc::~CbfcQueueDisc ()
{
}

void
CbfcQueueDisc::SetNumQueues (uint32_t n)
{
  m_numQueues = n;
}

uint32_t
CbfcQueueDisc::GetNumQueues (void) const
{
  return m_numQueues;
}

void
CbfcQueueDisc::SetQueueSize (uint32_t packets)
{
  m_queueSize = packets;
}

void
CbfcQueueDisc::SetCreditUpdateCallback (Callback<void, FlowStarMid, uint32_t> cb)
{
  m_creditCallback = cb;
  m_hasCreditCallback = true;
}

void
CbfcQueueDisc::SetEnqueueCallback (Callback<void, FlowStarMid, uint32_t> cb)
{
  m_enqueueCallback = cb;
  m_hasEnqueueCallback = true;
}

bool
CbfcQueueDisc::CheckConfig (void)
{
  // CheckConfig is called before InitializeParams in ns-3.47.
  // Create internal queues here if not yet created.
  if (GetNInternalQueues () == 0)
    {
      for (uint32_t i = 0; i < m_numQueues; ++i)
        {
          std::ostringstream oss;
          oss << m_queueSize << "p";
          Ptr<DropTailQueue<QueueDiscItem>> q =
            CreateObject<DropTailQueue<QueueDiscItem>> ();
          q->SetAttribute ("MaxSize", QueueSizeValue (QueueSize (oss.str ())));
          AddInternalQueue (q);

          QueueEntry entry;
          entry.occupancy = 0;
          m_queueTable.push_back (entry);
        }
    }

  if (GetNInternalQueues () != m_numQueues)
    {
      NS_LOG_ERROR ("CbfcQueueDisc: expected " << m_numQueues
                    << " internal queues, got " << GetNInternalQueues ());
      return false;
    }
  return true;
}

void
CbfcQueueDisc::InitializeParams (void)
{
  // Queue creation is handled in CheckConfig (called first by ns-3.47).
}

uint32_t
CbfcQueueDisc::AssignQueue (FlowStarMid mid)
{
  // Already assigned?
  auto it = m_messageTable.find (mid);
  if (it != m_messageTable.end ())
    {
      return it->second;
    }

  // Step 1: find a completely free (unoccupied) queue
  for (uint32_t i = 0; i < m_numQueues; ++i)
    {
      if (m_queueTable[i].activeMids.empty ())
        {
          m_messageTable[mid] = i;
          m_queueTable[i].activeMids.insert (mid);
          NS_LOG_INFO ("DQA: MID(" << mid.srcFlowId << "->" << mid.dstFlowId
                       << ") → dedicated queue " << i);
          return i;
        }
    }

  // Step 2: all queues occupied — choose least-loaded (original DQA fallback)
  // NOTE: This is the ORIGINAL algorithm. Congestion-aware assignment
  // belongs to Phase 5 Improvement #1.
  uint32_t leastIdx = 0;
  uint32_t leastOccupancy = m_queueTable[0].occupancy;
  for (uint32_t i = 1; i < m_numQueues; ++i)
    {
      if (m_queueTable[i].occupancy < leastOccupancy)
        {
          leastOccupancy = m_queueTable[i].occupancy;
          leastIdx = i;
        }
    }

  m_messageTable[mid] = leastIdx;
  m_queueTable[leastIdx].activeMids.insert (mid);

  NS_LOG_INFO ("DQA: MID(" << mid.srcFlowId << "->" << mid.dstFlowId
               << ") → shared queue " << leastIdx
               << " (activeMids=" << m_queueTable[leastIdx].activeMids.size ()
               << ", occupancy=" << leastOccupancy << ")");

  return leastIdx;
}

bool
CbfcQueueDisc::DoEnqueue (Ptr<QueueDiscItem> item)
{
  // Extract FlowStarMidTag
  FlowStarMidTag midTag;
  FlowStarMid mid;
  uint32_t queueIdx = 0;
  bool hasMid = item->GetPacket ()->PeekPacketTag (midTag);

  if (hasMid)
    {
      mid = midTag.GetMid ();
      queueIdx = AssignQueue (mid);
    }
  else
    {
      // Control packets (credit updates etc.) go to queue 0
      queueIdx = 0;
      mid = FlowStarMid(0,0,0,0);
    }

  bool ok = GetInternalQueue (queueIdx)->Enqueue (item);
  if (!ok)
    {
      return false;
    }

  m_queueTable[queueIdx].occupancy++;
  NS_LOG_DEBUG ("Enqueued packet for MID " << mid.srcFlowId << "->" << mid.dstFlowId
                << " to queue " << queueIdx << " (occupancy=" << m_queueTable[queueIdx].occupancy << ")");

  if (m_hasEnqueueCallback)
    {
      m_enqueueCallback (mid, m_queueTable[queueIdx].occupancy);
    }

  return true;
}

Ptr<QueueDiscItem>
CbfcQueueDisc::DoDequeue (void)
{
  // Round-Robin across non-empty queues
  uint32_t start = m_rrPtr;
  do
    {
      uint32_t idx = m_rrPtr;
      m_rrPtr = (m_rrPtr + 1) % m_numQueues;

      if (GetInternalQueue (idx)->GetNPackets () > 0)
        {
          Ptr<QueueDiscItem> item = GetInternalQueue (idx)->Dequeue ();
          if (item)
            {
              NS_ASSERT (m_queueTable[idx].occupancy > 0);
              m_queueTable[idx].occupancy--;

              // Fire credit callback for the dequeued MID
              if (m_hasCreditCallback)
                {
                  FlowStarMidTag midTag;
                  if (item->GetPacket ()->PeekPacketTag (midTag))
                    {
                      m_creditCallback (midTag.GetMid (),
                                        item->GetPacket ()->GetSize ());
                    }
                }
              return item;
            }
        }
    }
  while (m_rrPtr != start);

  return nullptr;
}

Ptr<const QueueDiscItem>
CbfcQueueDisc::DoPeek (void)
{
  uint32_t start = m_rrPtr;
  do
    {
      uint32_t idx = m_rrPtr;
      m_rrPtr = (m_rrPtr + 1) % m_numQueues;
      if (GetInternalQueue (idx)->GetNPackets () > 0)
        {
          return GetInternalQueue (idx)->Peek ();
        }
    }
  while (m_rrPtr != start);

  return nullptr;
}

// ---- Query methods ----

uint32_t
CbfcQueueDisc::GetQueueIndex (FlowStarMid mid) const
{
  auto it = m_messageTable.find (mid);
  if (it == m_messageTable.end ()) return UINT32_MAX;
  return it->second;
}

bool
CbfcQueueDisc::IsDedicatedQueue (uint32_t queueIdx) const
{
  if (queueIdx >= m_queueTable.size ()) return false;
  return m_queueTable[queueIdx].isDedicated ();
}

std::set<FlowStarMid>
CbfcQueueDisc::GetActiveMids (uint32_t queueIdx) const
{
  if (queueIdx >= m_queueTable.size ()) return {};
  return m_queueTable[queueIdx].activeMids;
}

uint32_t
CbfcQueueDisc::GetQueueOccupancy (uint32_t queueIdx) const
{
  if (queueIdx >= m_queueTable.size ()) return 0;
  return m_queueTable[queueIdx].occupancy;
}

} // namespace ns3
