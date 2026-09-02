#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "../model/flowstar-endpoint-controller.h"
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

  Ptr<FlowStarEndpointController> ep = CreateObject<FlowStarEndpointController> ();
  FlowStarMid mid(1,2,10,20);
  uint64_t expectedRate = 1000000000; // 1 Gbps
  double threshold = 0.8; // Trigger if rate drops below 800 Mbps

  // Note: we can't easily advance simulation time manually without scheduling events.
  // Instead of a full NS3 test suite, we'll test the basic API.
  bool ok = true;
  
  // Test 1: Header serialization
  FlowStarCnpHeader hdrIn(mid, 500000000, 12345);
  Ptr<Packet> p = Create<Packet>(100);
  p->AddHeader(hdrIn);
  FlowStarCnpHeader hdrOut;
  p->RemoveHeader(hdrOut);
  
  ok = (hdrOut.GetMid() == mid && hdrOut.GetReceivingRateBps() == 500000000 && hdrOut.GetTimestamp() == 12345);
  PrintTestResult("Test 1 (CNP Header Serialization)", ok);

  Simulator::Destroy ();
  
  return ok ? 0 : 1;
}
