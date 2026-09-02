/**
 * Test: SharedQueueFlowControl PAUSE/RESUME hysteresis.
 *
 * Verifies:
 * - CanTransmit returns true when queue is below pause threshold
 * - CanTransmit returns false when queue exceeds pause threshold
 * - CanTransmit remains false until queue drops below resume threshold (hysteresis)
 * - CanTransmit returns true once queue drops below resume threshold
 */
#include "ns3/core-module.h"
#include "../model/shared-queue-flow-control.h"

#include <cassert>
#include <iostream>

using namespace ns3;

int main (int argc, char *argv[])
{
  Ptr<SharedQueueFlowControl> fc = CreateObject<SharedQueueFlowControl> ();
  fc->SetThresholds (800, 500); // Pause at 800, Resume at 500

  int passed = 0;
  int total = 0;

  // Test 1: Initially OPEN
  total++;
  fc->UpdateQueueOccupancy (0);
  if (fc->CanTransmit (1, 1000))
    {
      std::cout << "Test 1 (Initial OPEN): PASS" << std::endl;
      passed++;
    }
  else
    {
      std::cout << "Test 1 (Initial OPEN): FAIL" << std::endl;
    }

  // Test 2: Below threshold — still OPEN
  total++;
  fc->UpdateQueueOccupancy (799);
  if (fc->CanTransmit (1, 1000))
    {
      std::cout << "Test 2 (Below Pause): PASS" << std::endl;
      passed++;
    }
  else
    {
      std::cout << "Test 2 (Below Pause): FAIL" << std::endl;
    }

  // Test 3: At/above threshold — PAUSED
  total++;
  fc->UpdateQueueOccupancy (800);
  if (!fc->CanTransmit (1, 1000))
    {
      std::cout << "Test 3 (At Pause Threshold): PASS" << std::endl;
      passed++;
    }
  else
    {
      std::cout << "Test 3 (At Pause Threshold): FAIL" << std::endl;
    }

  // Test 4: Hysteresis — still PAUSED above resume threshold
  total++;
  fc->UpdateQueueOccupancy (600);
  if (!fc->CanTransmit (1, 1000))
    {
      std::cout << "Test 4 (Hysteresis - still PAUSED): PASS" << std::endl;
      passed++;
    }
  else
    {
      std::cout << "Test 4 (Hysteresis - still PAUSED): FAIL" << std::endl;
    }

  // Test 5: Below resume threshold — RESUMED
  total++;
  fc->UpdateQueueOccupancy (500);
  if (fc->CanTransmit (1, 1000))
    {
      std::cout << "Test 5 (Below Resume Threshold): PASS" << std::endl;
      passed++;
    }
  else
    {
      std::cout << "Test 5 (Below Resume Threshold): FAIL" << std::endl;
    }

  // Test 6: Shared behavior — flowId does not matter
  total++;
  fc->UpdateQueueOccupancy (900);
  bool allPaused = !fc->CanTransmit (1, 1000) &&
                   !fc->CanTransmit (2, 1000) &&
                   !fc->CanTransmit (3, 1000);
  if (allPaused)
    {
      std::cout << "Test 6 (Shared - All Flows PAUSED): PASS" << std::endl;
      passed++;
    }
  else
    {
      std::cout << "Test 6 (Shared - All Flows PAUSED): FAIL" << std::endl;
    }

  std::cout << std::endl;
  std::cout << "SharedQueueFlowControl Test: " << passed << "/" << total << " passed" << std::endl;

  return (passed == total) ? 0 : 1;
}
