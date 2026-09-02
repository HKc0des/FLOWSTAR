#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "../model/flowstar-path-manager.h"
#include <iostream>

using namespace ns3;

void PrintTestResult(const std::string& name, bool pass) {
    if (pass) {
        std::cout << name << ": PASS\n";
    } else {
        std::cout << name << ": FAIL\n";
    }
}

int main(int argc, char *argv[])
{
  Time::SetResolution (Time::NS);
  int passed = 0;
  
  Ptr<FlowStarPathManager> pathMgr = CreateObject<FlowStarPathManager>();
  
  // Node 2 has two paths
  Ipv4Address pathA("10.1.1.2");
  Ipv4Address pathB("10.1.2.2");
  
  pathMgr->AddPath(2, pathA);
  pathMgr->AddPath(2, pathB);
  
  // Test 1: Initially, it should pick the first path (Path A) because both are 0.0
  Ipv4Address best1 = pathMgr->GetBestPath(2);
  bool ok1 = (best1 == pathA);
  PrintTestResult("Adaptive Routing - Initial Path", ok1);
  if (ok1) passed++;
  
  // Give Path A some congestion
  pathMgr->UpdatePathCongestion(pathA, 100.0);
  
  // Test 2: Now it should pick Path B
  Ipv4Address best2 = pathMgr->GetBestPath(2);
  bool ok2 = (best2 == pathB);
  PrintTestResult("Adaptive Routing - Avoid Congested Path", ok2);
  if (ok2) passed++;
  
  // Give Path B extreme congestion
  pathMgr->UpdatePathCongestion(pathB, 500.0);
  
  // Test 3: Now it should pick Path A again
  Ipv4Address best3 = pathMgr->GetBestPath(2);
  bool ok3 = (best3 == pathA);
  PrintTestResult("Adaptive Routing - Switch Back", ok3);
  if (ok3) passed++;
  
  // Test 4: Time decay
  Simulator::Schedule(Seconds(0.002), [](){}); // Wait > 1ms
  Simulator::Run();
  
  // Both decayed. Path A was 20.0 (0.8*0+0.2*100) -> decayed to 10.0
  // Path B was 100.0 (0.8*0+0.2*500) -> decayed to 50.0
  // Path A should still be better.
  Ipv4Address best4 = pathMgr->GetBestPath(2);
  bool ok4 = (best4 == pathA);
  PrintTestResult("Adaptive Routing - Time Decay", ok4);
  if (ok4) passed++;

  std::cout << "\nAdaptiveRouting Test: " << passed << "/4 passed\n";
  Simulator::Destroy ();
  return (passed == 4) ? 0 : 1;
}
