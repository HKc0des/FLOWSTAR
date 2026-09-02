/**
 * Tests 1–4: CbfcQueueDisc Dynamic Queue Assignment.
 *
 * Test 1: First packet of new MID gets an available queue.
 * Test 2: Second packet of same MID uses the same queue (no reassignment).
 * Test 3: Different MIDs can get different queues when queues are available.
 * Test 4: When all queues are occupied, new MID falls back to least-loaded queue;
 *          QueueTable.activeMids reflects multiple MIDs sharing one queue.
 */
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/traffic-control-module.h"
#include "../model/flowstar-mid.h"
#include "../model/cbfc-queue-disc.h"

using namespace ns3;

static Ptr<CbfcQueueDisc> BuildDisc (uint32_t numQueues)
{
  Ptr<CbfcQueueDisc> disc = CreateObject<CbfcQueueDisc> ();
  disc->SetNumQueues (numQueues);
  disc->SetQueueSize (100);
  disc->Initialize ();
  return disc;
}

static Ptr<QueueDiscItem> MakeItem (FlowStarMid mid)
{
  Ptr<Packet> pkt = Create<Packet> (1000);
  FlowStarMidTag tag (mid);
  pkt->AddPacketTag (tag);
  Ipv4Header hdr;
  return Create<Ipv4QueueDiscItem> (pkt, Address (), 0x0800, hdr);
}

int main (int argc, char *argv[])
{
  int passed = 0, total = 0;

  // 2 physical queues for easy exhaustion
  Ptr<CbfcQueueDisc> disc = BuildDisc (2);

  FlowStarMid midA (1, 4, 1, 1);
  FlowStarMid midB (2, 4, 2, 1);
  FlowStarMid midC (3, 4, 3, 1);

  // Test 1: first packet of new MID gets a queue
  total++;
  disc->Enqueue (MakeItem (midA));
  uint32_t qA = disc->GetQueueIndex (midA);
  if (qA != UINT32_MAX)
    {
      std::cout << "Test 1 (First packet → assigned queue " << qA << "): PASS\n";
      passed++;
    }
  else
    std::cout << "Test 1 (First packet → assigned queue): FAIL\n";

  // Test 2: second packet of same MID uses same queue
  total++;
  disc->Enqueue (MakeItem (midA));
  uint32_t qA2 = disc->GetQueueIndex (midA);
  if (qA2 == qA)
    {
      std::cout << "Test 2 (Second packet → same queue " << qA << "): PASS\n";
      passed++;
    }
  else
    std::cout << "Test 2 (Same-MID reuse): FAIL got q=" << qA2 << "\n";

  // Test 3: different MID gets a different queue (one free slot remains)
  total++;
  disc->Enqueue (MakeItem (midB));
  uint32_t qB = disc->GetQueueIndex (midB);
  if (qB != UINT32_MAX && qB != qA)
    {
      std::cout << "Test 3 (Different MID → different queue " << qB << "): PASS\n";
      passed++;
    }
  else
    std::cout << "Test 3 (Different MID → different queue): FAIL qB=" << qB << " qA=" << qA << "\n";

  // Test 4: queue exhaustion — midC must share with the least-loaded queue
  // At this point: Q0 and Q1 both occupied (by midA and midB respectively)
  total++;
  disc->Enqueue (MakeItem (midC));
  uint32_t qC = disc->GetQueueIndex (midC);
  bool     cShared    = !disc->IsDedicatedQueue (qC);
  auto     activeMids = disc->GetActiveMids (qC);
  bool     multiMid   = activeMids.size () >= 2;

  if (qC != UINT32_MAX && cShared && multiMid)
    {
      std::cout << "Test 4 (Exhaustion fallback → shared queue " << qC
                << ", activeMids=" << activeMids.size () << "): PASS\n";
      passed++;
    }
  else
    std::cout << "Test 4 (Exhaustion fallback): FAIL qC=" << qC
              << " shared=" << cShared << " multiMid=" << multiMid << "\n";

  std::cout << "\nCbfcQueueAssignment Test: " << passed << "/" << total << " passed\n";
  return (passed == total) ? 0 : 1;
}
