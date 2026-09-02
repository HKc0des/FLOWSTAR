#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "../model/flowstar-control-message.h"
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

  bool ok = true;
  FlowStarMid mid(1,2,10,20);
  
  // Test 1: BECN Header serialization
  FlowStarBecnHeader hdrIn(mid, 42, 99);
  Ptr<Packet> p = Create<Packet>(100);
  p->AddHeader(hdrIn);
  FlowStarBecnHeader hdrOut;
  p->RemoveHeader(hdrOut);
  
  ok = (hdrOut.GetMid() == mid && hdrOut.GetQueueOccupancy() == 42 && hdrOut.GetSwitchId() == 99);
  PrintTestResult("Test 1 (BECN Header Serialization)", ok);

  Simulator::Destroy ();
  
  return ok ? 0 : 1;
}
