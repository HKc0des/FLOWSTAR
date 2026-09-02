#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/traffic-control-module.h"
#include "../model/improved-cbfc-queue-disc.h"
#include <iostream>

using namespace ns3;

void PrintTestResult(const std::string& name, bool pass) {
    if (pass) {
        std::cout << name << ": PASS\n";
    } else {
        std::cout << name << ": FAIL\n";
    }
}

static Ptr<QueueDiscItem> MakeItem (FlowStarMid mid)
{
  Ptr<Packet> pkt = Create<Packet> (1000);
  FlowStarMidTag tag (mid);
  pkt->AddPacketTag (tag);
  Ipv4Header hdr;
  return Create<Ipv4QueueDiscItem> (pkt, Address (), 0x0800, hdr);
}

int main(int argc, char *argv[])
{
  Time::SetResolution (Time::NS);

  int passed = 0;
  
  // Create the queue disc
  Ptr<ImprovedCbfcQueueDisc> qdisc = CreateObject<ImprovedCbfcQueueDisc>();
  qdisc->SetNumQueues(2);
  qdisc->Initialize();
  
  // Test 1: Empty queues get assigned first (Flow A -> Q0, Flow B -> Q1)
  FlowStarMid midA(1, 2, 10, 20);
  FlowStarMid midB(3, 4, 30, 40);
  
  qdisc->Enqueue(MakeItem(midA));
  qdisc->Enqueue(MakeItem(midB));
  
  bool ok1 = (qdisc->GetQueueIndex(midA) == 0 && qdisc->GetQueueIndex(midB) == 1);
  PrintTestResult("Test 1 (Initial Empty Queues)", ok1);
  if (ok1) passed++;
  
  // Test 2: Exhaustion assignment.
  // Q0 has 1 packet (occupancy = 1), Q1 has 1 packet (occupancy = 1).
  // We'll dequeue from Q0 to make its occupancy 0.
  // Then the next flow C should be assigned to Q0 because its score will be lower.
  qdisc->Dequeue(); // Should dequeue from Q0 (round-robin)
  
  // Advance time so trend updates properly
  Simulator::Schedule(MicroSeconds(10), [](){});
  Simulator::Run();
  
  FlowStarMid midC(5, 6, 50, 60);
  qdisc->Enqueue(MakeItem(midC));
  
  // Q0 had occupancy 0 (trend dropping), Q1 has occupancy 1 (trend static).
  // Q0 score should be lower. So Flow C goes to Q0.
  bool ok2 = (qdisc->GetQueueIndex(midC) == 0);
  PrintTestResult("Test 2 (Score-based Assignment)", ok2);
  if (ok2) passed++;
  
  std::cout << "\nImprovedCbfcQueueAssignment Test: " << passed << "/2 passed\n";

  Simulator::Destroy ();
  
  return (passed == 2) ? 0 : 1;
}
