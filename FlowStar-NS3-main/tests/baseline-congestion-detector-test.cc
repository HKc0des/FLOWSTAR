/**
 * Test: BaselineCongestionDetector ECN marking.
 *
 * Verifies:
 * - No ECN mark when below threshold
 * - ECN mark applied when at/above threshold
 * - IsCongested() reflects current state
 */
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "../model/baseline-congestion-detector.h"
#include "../model/baseline-control-message.h"

#include <iostream>

using namespace ns3;

int main (int argc, char *argv[])
{
  int passed = 0;
  int total = 0;

  Ptr<BaselineCongestionDetector> detector = CreateObject<BaselineCongestionDetector> ();
  detector->SetCongestionThreshold (500);

  // Test 1: No congestion below threshold
  total++;
  {
    Ptr<Packet> p = Create<Packet> (1000);
    detector->OnEnqueue (499, p);
    EcnTag ecnTag;
    if (!p->PeekPacketTag (ecnTag))
      {
        std::cout << "Test 1 (No ECN Below Threshold): PASS" << std::endl;
        passed++;
      }
    else
      {
        std::cout << "Test 1 (No ECN Below Threshold): FAIL" << std::endl;
      }
  }

  // Test 2: IsCongested false below threshold
  total++;
  if (!detector->IsCongested ())
    {
      std::cout << "Test 2 (Not Congested Below Threshold): PASS" << std::endl;
      passed++;
    }
  else
    {
      std::cout << "Test 2 (Not Congested Below Threshold): FAIL" << std::endl;
    }

  // Test 3: ECN marked at threshold
  total++;
  {
    Ptr<Packet> p = Create<Packet> (1000);
    detector->OnEnqueue (500, p);
    EcnTag ecnTag;
    if (p->PeekPacketTag (ecnTag) && ecnTag.GetCongestionExperienced ())
      {
        std::cout << "Test 3 (ECN At Threshold): PASS" << std::endl;
        passed++;
      }
    else
      {
        std::cout << "Test 3 (ECN At Threshold): FAIL" << std::endl;
      }
  }

  // Test 4: IsCongested true at threshold
  total++;
  if (detector->IsCongested ())
    {
      std::cout << "Test 4 (Congested At Threshold): PASS" << std::endl;
      passed++;
    }
  else
    {
      std::cout << "Test 4 (Congested At Threshold): FAIL" << std::endl;
    }

  // Test 5: ECN marked above threshold
  total++;
  {
    Ptr<Packet> p = Create<Packet> (1000);
    detector->OnEnqueue (1000, p);
    EcnTag ecnTag;
    if (p->PeekPacketTag (ecnTag) && ecnTag.GetCongestionExperienced ())
      {
        std::cout << "Test 5 (ECN Above Threshold): PASS" << std::endl;
        passed++;
      }
    else
      {
        std::cout << "Test 5 (ECN Above Threshold): FAIL" << std::endl;
      }
  }

  // Test 6: No ECN after dropping below threshold again
  total++;
  {
    Ptr<Packet> p = Create<Packet> (1000);
    detector->OnEnqueue (100, p);
    EcnTag ecnTag;
    if (!p->PeekPacketTag (ecnTag))
      {
        std::cout << "Test 6 (No ECN After Drop Below): PASS" << std::endl;
        passed++;
      }
    else
      {
        std::cout << "Test 6 (No ECN After Drop Below): FAIL" << std::endl;
      }
  }

  std::cout << std::endl;
  std::cout << "BaselineCongestionDetector Test: " << passed << "/" << total << " passed" << std::endl;

  return (passed == total) ? 0 : 1;
}
