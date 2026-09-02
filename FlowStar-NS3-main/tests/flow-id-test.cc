#include "ns3/core-module.h"
#include "../model/basic-flow-id-tag.h"
#include "ns3/packet.h"
#include <cassert>
#include <iostream>

using namespace ns3;

int main (int argc, char *argv[])
{
  Ptr<Packet> p = Create<Packet> (100);
  BasicFlowIdTag tag (42, 1001, 555555);
  p->AddPacketTag (tag);

  BasicFlowIdTag tag2;
  bool found = p->PeekPacketTag (tag2);

  assert(found == true);
  assert(tag2.GetFlowId() == 42);
  assert(tag2.GetSequenceNumber() == 1001);
  assert(tag2.GetGenerationTimestamp() == 555555);

  std::cout << "BasicFlowIdTag Serialization/Deserialization Test: PASS\n";
  return 0;
}
