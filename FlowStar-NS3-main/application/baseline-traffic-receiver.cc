#include "baseline-traffic-receiver.h"
#include "ns3/log.h"
#include "ns3/simulator.h"
#include "ns3/network-module.h"
#include "../model/basic-flow-id-tag.h"
#include "../model/baseline-control-message.h"
#include "../utils/metrics-collector.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("BaselineTrafficReceiver");

NS_OBJECT_ENSURE_REGISTERED (BaselineTrafficReceiver);

TypeId
BaselineTrafficReceiver::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::BaselineTrafficReceiver")
    .SetParent<Application> ()
    .SetGroupName("Applications")
    .AddConstructor<BaselineTrafficReceiver> ()
  ;
  return tid;
}

BaselineTrafficReceiver::BaselineTrafficReceiver ()
  : m_dataPort (0),
    m_cnpPort (5000),
    m_dataSocket (0),
    m_ccEnabled (true)
{
}

BaselineTrafficReceiver::~BaselineTrafficReceiver ()
{
  m_dataSocket = 0;
}

void
BaselineTrafficReceiver::DoDispose (void)
{
  m_dataSocket = 0;
  Application::DoDispose ();
}

void
BaselineTrafficReceiver::Setup (uint16_t dataPort, uint16_t cnpPort)
{
  m_dataPort = dataPort;
  m_cnpPort = cnpPort;
}

void
BaselineTrafficReceiver::SetCcEnabled (bool enabled)
{
  m_ccEnabled = enabled;
}

void
BaselineTrafficReceiver::RegisterSender (uint32_t flowId, Address senderAddress)
{
  m_senderAddresses[flowId] = senderAddress;
}

void
BaselineTrafficReceiver::RegisterExpectedBytes (uint32_t flowId, uint64_t expectedBytes)
{
  if (m_flowStats.find(flowId) == m_flowStats.end())
    {
      m_flowStats[flowId] = {expectedBytes, 0, 0, Simulator::Now(), Simulator::Now(), 0, false};
    }
  else
    {
      m_flowStats[flowId].expectedBytes = expectedBytes;
    }
}

void
BaselineTrafficReceiver::StartApplication (void)
{
  if (!m_dataSocket)
    {
      m_dataSocket = Socket::CreateSocket (GetNode (), TypeId::LookupByName ("ns3::UdpSocketFactory"));
      InetSocketAddress local = InetSocketAddress (Ipv4Address::GetAny (), m_dataPort);
      m_dataSocket->Bind (local);
    }
  m_dataSocket->SetRecvCallback (MakeCallback (&BaselineTrafficReceiver::HandleRead, this));
}

void
BaselineTrafficReceiver::StopApplication (void)
{
  if (m_dataSocket)
    {
      m_dataSocket->SetRecvCallback (MakeNullCallback<void, Ptr<Socket> > ());
    }
}

void
BaselineTrafficReceiver::HandleRead (Ptr<Socket> socket)
{
  Ptr<Packet> packet;
  Address from;
  while ((packet = socket->RecvFrom (from)))
    {
      BasicFlowIdTag flowTag;
      if (packet->PeekPacketTag (flowTag))
        {
          uint32_t flowId = flowTag.GetFlowId ();

          // Update flow stats
          if (m_flowStats.find(flowId) == m_flowStats.end())
            {
              m_flowStats[flowId] = {0, 0, 0, Simulator::Now(), Simulator::Now(), 0, false};
            }
          auto& s = m_flowStats[flowId];
          s.bytesReceived += packet->GetSize ();
          s.packetsReceived++;
          s.lastPacketTime = Simulator::Now();

          if (s.expectedBytes > 0 && s.bytesReceived >= s.expectedBytes && !s.completed)
            {
              s.completed = true;
              MetricsCollector::GetInstance().FlowComplete(flowId, s.bytesReceived);
            }

          // Check for ECN mark and generate CNP if CC is enabled
          if (m_ccEnabled)
            {
              EcnTag ecnTag;
              if (packet->PeekPacketTag (ecnTag) && ecnTag.GetCongestionExperienced ())
                {
                  m_flowStats[flowId].ecnMarkedPackets++;
                  NS_LOG_INFO ("ECN mark detected for flow " << flowId << ", sending CNP");

                  // Send CNP to the sender
                  auto it = m_senderAddresses.find (flowId);
                  if (it != m_senderAddresses.end ())
                    {
                      SendCnp (flowId, it->second);
                    }
                }
            }
        }
    }
}

void
BaselineTrafficReceiver::SendCnp (uint32_t flowId, Address senderAddress)
{
  Ptr<Socket> cnpSocket = Socket::CreateSocket (GetNode (), TypeId::LookupByName ("ns3::UdpSocketFactory"));
  cnpSocket->Connect (senderAddress);

  Ptr<Packet> cnpPacket = Create<Packet> (0);
  CnpHeader cnpHeader (flowId);
  cnpPacket->AddHeader (cnpHeader);

  cnpSocket->Send (cnpPacket);
  cnpSocket->Close ();
}

} // namespace ns3
