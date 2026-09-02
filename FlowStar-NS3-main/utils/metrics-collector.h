#ifndef METRICS_COLLECTOR_H
#define METRICS_COLLECTOR_H

#include "ns3/core-module.h"
#include "ns3/packet.h"
#include "ns3/nstime.h"
#include "ns3/event-id.h"
#include "ns3/data-rate.h"
#include "ns3/queue-item.h"
#include <stdint.h>

namespace ns3 {

class MetricsCollector {
public:
  static MetricsCollector& GetInstance ();

  void Reset ();

  void TxCallback (std::string context, Ptr<const Packet> p);
  void RxCallback (std::string context, Ptr<const Packet> p);
  void DropCallback (std::string context, Ptr<const Packet> p);
  void EnqueueCallback (std::string context, Ptr<const Packet> p);
  void DequeueCallback (std::string context, Ptr<const Packet> p);

  void InstallTxRxDropHooks ();
  void InstallQueueHooks ();
  void StartQueueSampling (Time interval, std::string filepath, uint32_t mode, uint32_t workload, uint32_t seed);

  // FCT tracking
  void FlowStart (uint32_t flowId, uint64_t expectedBytes);
  void FlowComplete (uint32_t flowId, uint64_t rxPayloadBytes);
  void FlowProgress (uint32_t flowId, uint64_t rxPayloadBytes);

  // Rate tracking
  void StartRateTracking (std::string filepath, uint32_t mode, uint32_t workload, uint32_t seed);
  void RateChange (uint32_t flowId, DataRate newRate);

  // Expose these for CSV writing
  struct FlowRecord {
    uint32_t flowId;
    Time startTime;
    Time completionTime;
    uint64_t expectedBytes;
    uint64_t rxPayloadBytes;
  };
  const std::map<uint32_t, FlowRecord>& GetFlowRecords () const { return m_flowRecords; }

  uint64_t GetTotalTxPackets () const { return m_txPackets; }
  uint64_t GetTotalRxPackets () const { return m_rxPackets; }
  uint64_t GetTotalDroppedPackets () const { return m_dropPackets; }
  
  uint32_t GetCurrentQueueOccupancy () const { return m_currentQueueOccupancy; }
  uint32_t GetMaxQueueOccupancy () const { return m_maxQueueOccupancy; }

private:
  MetricsCollector ();
  ~MetricsCollector ();

  uint64_t m_txPackets;
  uint64_t m_rxPackets;
  uint64_t m_dropPackets;

  uint32_t m_currentQueueOccupancy;
  uint32_t m_maxQueueOccupancy;
  std::map<uint32_t, FlowRecord> m_flowRecords;

  // Queue sampling
  void SampleQueue ();
  EventId m_queueSampleEvent;
  Time m_queueSampleInterval;
  std::string m_queueSampleFile;
  uint32_t m_queueSampleMode;
  uint32_t m_queueSampleWorkload;
  uint32_t m_queueSampleSeed;

  // Rate tracking
  std::string m_rateSampleFile;
  uint32_t m_rateSampleMode;
  uint32_t m_rateSampleWorkload;
  uint32_t m_rateSampleSeed;
  bool m_rateTrackingEnabled;
};

} // namespace ns3

#endif /* METRICS_COLLECTOR_H */
