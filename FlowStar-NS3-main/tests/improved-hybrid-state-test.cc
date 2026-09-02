#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/traffic-control-module.h"
#include "../model/improved-cbfc-queue-disc.h"
#include "../model/cbfc-credit-manager.h"
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
  
  // Create the queue disc with only 1 queue
  Ptr<ImprovedCbfcQueueDisc> qdisc = CreateObject<ImprovedCbfcQueueDisc>();
  qdisc->SetNumQueues(1);
  qdisc->SetQueueSize(2); // very small capacity
  qdisc->Initialize();
  
  Ptr<CbfcCreditManager> creditMgr = CreateObject<CbfcCreditManager>();
  
  FlowStarMid midA(1, 2, 10, 20);
  FlowStarMid midB(3, 4, 30, 40);
  
  creditMgr->InitializeFlow(midA, 10000, 10000);
  creditMgr->InitializeFlow(midB, 10000, 10000);
  
  // Both flows assigned to the same queue (Q0) since there's only 1 queue
  qdisc->Enqueue(MakeItem(midA));
  qdisc->Enqueue(MakeItem(midB));
  
  // Physical queue is full (size 2). Enqueueing a 3rd packet will drop!
  bool dropped = !qdisc->Enqueue(MakeItem(midA));
  
  // Logical credits remain independent!
  creditMgr->ConsumeCredit(midA, 1000);
  creditMgr->ConsumeCredit(midA, 1000);
  creditMgr->ConsumeCredit(midB, 1000);
  
  // Check decoupling
  bool ok1 = (creditMgr->GetAvailableCredit(midA) == 8000);
  bool ok2 = (creditMgr->GetAvailableCredit(midB) == 9000);
  bool ok3 = dropped; // Dropped because physical queue is exhausted
  
  PrintTestResult("Hybrid State - Logical independence Flow A", ok1);
  PrintTestResult("Hybrid State - Logical independence Flow B", ok2);
  PrintTestResult("Hybrid State - Physical queue bounds enforced", ok3);
  
  if (ok1 && ok2 && ok3) {
      passed++;
  }
  
  std::cout << "\nHybridState Test: " << passed << "/1 passed\n";
  Simulator::Destroy ();
  return (passed == 1) ? 0 : 1;
}
