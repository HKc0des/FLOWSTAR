#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "../model/flowstar-rate-controller.h"
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

  Ptr<FlowStarRateController> rc = CreateObject<FlowStarRateController> ();
  // Note: minRate is 100Mbps default.
  FlowStarMid mid(1,2,10,20);
  DataRate lineRate("10Gbps");
  
  rc->InitializeFlow(mid, lineRate);

  int passed = 0;
  
  // Test 1: Initial rate is lineRate
  bool ok1 = (rc->GetCurrentRate(mid) == lineRate);
  PrintTestResult("Test 1 (Initial Rate == Line Rate)", ok1);
  if (ok1) passed++;

  // Test 2: OnCnp changes rate to receivedRate * cnpDecreaseFactor (default 1.0)
  uint64_t measuredRate = 5000000000ULL; // 5 Gbps
  rc->OnCnp(mid, measuredRate);
  bool ok2 = (rc->GetCurrentRate(mid).GetBitRate() == measuredRate);
  PrintTestResult("Test 2 (Rate Drop on CNP)", ok2);
  if (ok2) passed++;

  // Test 3: OnBecn multiplies by 0.5 (default becnDecreaseFactor)
  rc->OnBecn(mid, 50);
  bool ok3 = (rc->GetCurrentRate(mid).GetBitRate() == measuredRate / 2);
  PrintTestResult("Test 3 (Multiplicative Decrease on BECN)", ok3);
  if (ok3) passed++;

  std::cout << "\nFlowStarRateController Test: " << passed << "/3 passed\n";

  Simulator::Destroy ();
  
  return (passed == 3) ? 0 : 1;
}
