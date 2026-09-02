/**
 * Tests 5–10: CbfcCreditManager credit semantics.
 *
 * Test 5:  InitializeFlow sets FCTBS = initialCredit and FCCL = fccl.
 * Test 6:  ConsumeCredit decreases FCTBS by exactly packetSize.
 * Test 7:  CanTransmit returns false when FCTBS < packetSize.
 * Test 8:  ReplenishCredit cannot raise FCTBS above FCCL.
 * Test 9:  Two flows have independent FCTBS/FCCL state.
 * Test 10: Multiple send/replenish cycles leave FCTBS consistent.
 */
#include "ns3/core-module.h"
#include "../model/cbfc-credit-manager.h"

using namespace ns3;

int main (int argc, char *argv[])
{
  int passed = 0, total = 0;

  Ptr<CbfcCreditManager> cm = CreateObject<CbfcCreditManager> ();

  FlowStarMid midA (1, 4, 1, 1);
  FlowStarMid midB (2, 4, 2, 1);

  const uint32_t initialCredit = 16000; // 16 KB
  const uint32_t fccl          = 32000; // 32 KB ceiling
  const uint32_t pktSize       = 1000;

  // Test 5: InitializeFlow
  total++;
  cm->InitializeFlow (midA, initialCredit, fccl);
  if (cm->GetAvailableCredit (midA) == initialCredit &&
      cm->GetCreditCeiling (midA) == fccl)
    {
      std::cout << "Test 5 (InitializeFlow FCTBS=" << initialCredit
                << " FCCL=" << fccl << "): PASS\n";
      passed++;
    }
  else
    std::cout << "Test 5 (InitializeFlow): FAIL FCTBS="
              << cm->GetAvailableCredit (midA) << " FCCL="
              << cm->GetCreditCeiling (midA) << "\n";

  // Test 6: ConsumeCredit decreases FCTBS by exactly pktSize
  total++;
  cm->ConsumeCredit (midA, pktSize);
  uint32_t expected6 = initialCredit - pktSize;
  if (cm->GetAvailableCredit (midA) == expected6)
    {
      std::cout << "Test 6 (ConsumeCredit FCTBS=" << expected6 << "): PASS\n";
      passed++;
    }
  else
    std::cout << "Test 6 (ConsumeCredit): FAIL got="
              << cm->GetAvailableCredit (midA) << " expected=" << expected6 << "\n";

  // Test 7: CanTransmit returns false when FCTBS < pktSize
  total++;
  // Drain all credits
  while (cm->CanTransmit (midA, pktSize))
    cm->ConsumeCredit (midA, pktSize);

  if (!cm->CanTransmit (midA, pktSize))
    {
      std::cout << "Test 7 (CanTransmit blocked at FCTBS="
                << cm->GetAvailableCredit (midA) << "): PASS\n";
      passed++;
    }
  else
    std::cout << "Test 7 (CanTransmit should be blocked): FAIL\n";

  // Test 8: ReplenishCredit cannot exceed FCCL
  total++;
  // Replenish more than FCCL
  cm->ReplenishCredit (midA, fccl + 99999);
  if (cm->GetAvailableCredit (midA) == fccl)
    {
      std::cout << "Test 8 (ReplenishCredit capped at FCCL=" << fccl << "): PASS\n";
      passed++;
    }
  else
    std::cout << "Test 8 (FCCL ceiling): FAIL FCTBS="
              << cm->GetAvailableCredit (midA) << " expected=" << fccl << "\n";

  // Test 9: Two flows have independent credit state
  total++;
  cm->InitializeFlow (midB, initialCredit, fccl);
  cm->ConsumeCredit (midB, pktSize * 5); // drain 5 packets from B

  bool aUntouched = (cm->GetAvailableCredit (midA) == fccl); // A was replenished to ceiling in Test 8
  bool bDrained   = (cm->GetAvailableCredit (midB) == initialCredit - pktSize * 5);

  if (aUntouched && bDrained)
    {
      std::cout << "Test 9 (Independent credit: A=" << cm->GetAvailableCredit (midA)
                << " B=" << cm->GetAvailableCredit (midB) << "): PASS\n";
      passed++;
    }
  else
    std::cout << "Test 9 (Independent credit): FAIL A=" << cm->GetAvailableCredit (midA)
              << " B=" << cm->GetAvailableCredit (midB) << "\n";

  // Test 10: Multiple send/replenish cycles are consistent
  total++;
  // Reset A to a known mid-credit state
  cm->ReplenishCredit (midA, 0); // no-op to ensure state is stable
  uint32_t startFctbs = cm->GetAvailableCredit (midA);

  // send 3 packets, replenish 3 packets
  for (int i = 0; i < 3; ++i)
    {
      if (cm->CanTransmit (midA, pktSize))
        cm->ConsumeCredit (midA, pktSize);
    }
  cm->ReplenishCredit (midA, pktSize * 3);

  uint32_t afterFctbs = cm->GetAvailableCredit (midA);
  // Should end at startFctbs (send 3, replenish 3), capped at FCCL
  uint32_t expectedAfter = std::min (startFctbs, fccl);

  if (afterFctbs == expectedAfter)
    {
      std::cout << "Test 10 (3x send/replenish cycle, FCTBS=" << afterFctbs << "): PASS\n";
      passed++;
    }
  else
    std::cout << "Test 10 (Cycle consistency): FAIL FCTBS=" << afterFctbs
              << " expected=" << expectedAfter << "\n";

  std::cout << "\nCbfcCredit Test: " << passed << "/" << total << " passed\n";
  return (passed == total) ? 0 : 1;
}
