#include "metrics-collector.h"
#include "ns3/config.h"
#include "ns3/log.h"
#include "ns3/simulator.h"
#include "../model/basic-flow-id-tag.h"
#include <fstream>

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("MetricsCollector");

MetricsCollector& MetricsCollector::GetInstance ()
{
  static MetricsCollector instance;
  return instance;
}

MetricsCollector::MetricsCollector ()
{
  Reset ();
}

MetricsCollector::~MetricsCollector ()
{
}

void MetricsCollector::Reset ()
{
  m_txPackets = 0;
  m_rxPackets = 0;
  m_dropPackets = 0;
  m_currentQueueOccupancy = 0;
  m_maxQueueOccupancy = 0;
  m_flowRecords.clear();
  m_rateTrackingEnabled = false;
}

void MetricsCollector::TxCallback (std::string context, Ptr<const Packet> p)
{
  m_txPackets++;
}

void MetricsCollector::RxCallback (std::string context, Ptr<const Packet> p)
{
  m_rxPackets++;
}

void MetricsCollector::DropCallback (std::string context, Ptr<const Packet> p)
{
  m_dropPackets++;
}

void MetricsCollector::EnqueueCallback (std::string context, Ptr<const Packet> p)
{
  m_currentQueueOccupancy++;
  if (m_currentQueueOccupancy > m_maxQueueOccupancy)
    {
      m_maxQueueOccupancy = m_currentQueueOccupancy;
    }
}

void MetricsCollector::DequeueCallback (std::string context, Ptr<const Packet> p)
{
  if (m_currentQueueOccupancy > 0)
    {
      m_currentQueueOccupancy--;
    }
}

void MetricsCollector::FlowStart (uint32_t flowId, uint64_t expectedBytes)
{
  auto& record = m_flowRecords[flowId];
  record.flowId = flowId;
  record.startTime = Simulator::Now();
  record.expectedBytes = expectedBytes;
  record.rxPayloadBytes = 0;
}

void MetricsCollector::FlowComplete (uint32_t flowId, uint64_t rxPayloadBytes)
{
  if (m_flowRecords.find(flowId) != m_flowRecords.end()) {
    m_flowRecords[flowId].rxPayloadBytes = rxPayloadBytes;
    m_flowRecords[flowId].completionTime = Simulator::Now();
  }
}

void MetricsCollector::FlowProgress (uint32_t flowId, uint64_t rxPayloadBytes)
{
  if (m_flowRecords.find(flowId) != m_flowRecords.end()) {
    m_flowRecords[flowId].rxPayloadBytes = rxPayloadBytes;
  }
}

void MetricsCollector::InstallTxRxDropHooks ()
{
  Config::Connect ("/NodeList/*/DeviceList/*/$ns3::PointToPointNetDevice/TxQueue/Drop",
                   MakeCallback (&MetricsCollector::DropCallback, this));
  Config::Connect ("/NodeList/*/DeviceList/*/$ns3::PointToPointNetDevice/PhyTxEnd",
                   MakeCallback (&MetricsCollector::TxCallback, this));
  Config::Connect ("/NodeList/*/DeviceList/*/$ns3::PointToPointNetDevice/PhyRxEnd",
                   MakeCallback (&MetricsCollector::RxCallback, this));
}

void MetricsCollector::InstallQueueHooks ()
{
  Config::Connect ("/NodeList/*/DeviceList/*/$ns3::PointToPointNetDevice/TxQueue/Enqueue",
                   MakeCallback (&MetricsCollector::EnqueueCallback, this));
  Config::Connect ("/NodeList/*/DeviceList/*/$ns3::PointToPointNetDevice/TxQueue/Dequeue",
                   MakeCallback (&MetricsCollector::DequeueCallback, this));
}

void MetricsCollector::StartQueueSampling (Time interval, std::string filepath, uint32_t mode, uint32_t workload, uint32_t seed)
{
  m_queueSampleInterval = interval;
  m_queueSampleFile = filepath;
  m_queueSampleMode = mode;
  m_queueSampleWorkload = workload;
  m_queueSampleSeed = seed;

  std::ofstream csv(m_queueSampleFile, std::ios_base::app);
  if (csv.is_open()) {
    csv.seekp(0, std::ios::end);
    if (csv.tellp() == 0) {
      csv << "timestamp_ns,mode,workload,seed,occupancy\n";
    }
  }

  if (!m_queueSampleEvent.IsExpired()) {
    Simulator::Cancel(m_queueSampleEvent);
  }
  m_queueSampleEvent = Simulator::Schedule(m_queueSampleInterval, &MetricsCollector::SampleQueue, this);
}

void MetricsCollector::SampleQueue ()
{
  std::ofstream csv(m_queueSampleFile, std::ios_base::app);
  if (csv.is_open()) {
    csv << Simulator::Now().GetNanoSeconds() << ","
        << m_queueSampleMode << ","
        << m_queueSampleWorkload << ","
        << m_queueSampleSeed << ","
        << m_currentQueueOccupancy << "\n";
  }
  m_queueSampleEvent = Simulator::Schedule(m_queueSampleInterval, &MetricsCollector::SampleQueue, this);
}

void MetricsCollector::StartRateTracking (std::string filepath, uint32_t mode, uint32_t workload, uint32_t seed)
{
  m_rateSampleFile = filepath;
  m_rateSampleMode = mode;
  m_rateSampleWorkload = workload;
  m_rateSampleSeed = seed;
  m_rateTrackingEnabled = true;

  std::ofstream csv(m_rateSampleFile, std::ios_base::app);
  if (csv.is_open()) {
    csv.seekp(0, std::ios::end);
    if (csv.tellp() == 0) {
      csv << "timestamp_ns,mode,workload,seed,flow_id,rate_bps\n";
    }
  }
}

void MetricsCollector::RateChange (uint32_t flowId, DataRate newRate)
{
  if (!m_rateTrackingEnabled) return;

  std::ofstream csv(m_rateSampleFile, std::ios_base::app);
  if (csv.is_open()) {
    csv << Simulator::Now().GetNanoSeconds() << ","
        << m_rateSampleMode << ","
        << m_rateSampleWorkload << ","
        << m_rateSampleSeed << ","
        << flowId << ","
        << newRate.GetBitRate() << "\n";
  }
}

} // namespace ns3
