#include "improved-cbfc-queue-disc.h"
#include "ns3/log.h"
#include "ns3/simulator.h"
#include "ns3/double.h"
#include <limits>
#include <algorithm>

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("ImprovedCbfcQueueDisc");
NS_OBJECT_ENSURE_REGISTERED (ImprovedCbfcQueueDisc);

TypeId
ImprovedCbfcQueueDisc::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::ImprovedCbfcQueueDisc")
    .SetParent<CbfcQueueDisc> ()
    .SetGroupName ("FlowStar")
    .AddConstructor<ImprovedCbfcQueueDisc> ()
    .AddAttribute ("OccupancyWeight",
                   "Weight for normalized occupancy in DQA scoring",
                   DoubleValue (1.0),
                   MakeDoubleAccessor (&ImprovedCbfcQueueDisc::m_occupancyWeight),
                   MakeDoubleChecker<double> ())
    .AddAttribute ("TrendWeight",
                   "Weight for normalized trend in DQA scoring",
                   DoubleValue (1.0),
                   MakeDoubleAccessor (&ImprovedCbfcQueueDisc::m_trendWeight),
                   MakeDoubleChecker<double> ());
  return tid;
}

ImprovedCbfcQueueDisc::ImprovedCbfcQueueDisc ()
  : m_occupancyWeight (1.0),
    m_trendWeight (1.0),
    m_lastTrendUpdate (Seconds(0))
{
}

ImprovedCbfcQueueDisc::~ImprovedCbfcQueueDisc ()
{
}

void
ImprovedCbfcQueueDisc::UpdateTrend (void)
{
  Time now = Simulator::Now ();
  if (m_prevOccupancy.empty ())
    {
      m_prevOccupancy.assign (GetNumQueues (), 0);
      m_trend.assign (GetNumQueues (), 0.0);
      m_lastTrendUpdate = now;
      return;
    }
    
  if (now > m_lastTrendUpdate)
    {
      double dt = (now - m_lastTrendUpdate).GetSeconds ();
      if (dt > 0.000001) // Update at most every 1us to avoid noise/division by zero
        {
          for (uint32_t i = 0; i < GetNumQueues (); ++i)
            {
              uint32_t currentOcc = m_queueTable[i].occupancy;
              double delta = (double)currentOcc - (double)m_prevOccupancy[i];
              m_trend[i] = delta / dt; // packets per second trend
              m_prevOccupancy[i] = currentOcc;
            }
          m_lastTrendUpdate = now;
        }
    }
}

uint32_t
ImprovedCbfcQueueDisc::AssignQueue (FlowStarMid mid)
{
  // First update trend
  UpdateTrend ();
  
  auto it = m_messageTable.find (mid);
  if (it != m_messageTable.end ())
    {
      return it->second;
    }

  // Find an empty queue (dedicated assignment)
  for (uint32_t i = 0; i < GetNumQueues (); ++i)
    {
      if (m_queueTable[i].activeMids.empty ())
        {
          m_messageTable[mid] = i;
          m_queueTable[i].activeMids.insert (mid);
          NS_LOG_INFO ("Assigning new MID " << mid.srcFlowId << "->" << mid.dstFlowId 
                       << " to empty queue " << i);
          return i;
        }
    }

  // If no empty queue, evaluate queues using congestion score
  double bestScore = std::numeric_limits<double>::max ();
  uint32_t bestQueue = 0;
  
  // Normalize metrics
  uint32_t maxCapacity = GetMaxSize ().GetValue ();
  if (maxCapacity == 0) maxCapacity = 1000;
  
  double maxTrend = 0.0;
  double minTrend = 0.0;
  for (uint32_t i = 0; i < GetNumQueues (); ++i)
    {
      maxTrend = std::max(maxTrend, m_trend[i]);
      minTrend = std::min(minTrend, m_trend[i]);
    }
  double trendRange = maxTrend - minTrend;
  if (trendRange < 1.0) trendRange = 1.0;

  for (uint32_t i = 0; i < GetNumQueues (); ++i)
    {
      double normalizedOccupancy = (double)m_queueTable[i].occupancy / maxCapacity;
      double normalizedTrend = (m_trend[i] - minTrend) / trendRange;
      
      double score = m_occupancyWeight * normalizedOccupancy + m_trendWeight * normalizedTrend;
      
      if (score < bestScore)
        {
          bestScore = score;
          bestQueue = i;
        }
    }

  m_messageTable[mid] = bestQueue;
  m_queueTable[bestQueue].activeMids.insert (mid);
  NS_LOG_INFO ("Assigning new MID " << mid.srcFlowId << "->" << mid.dstFlowId 
               << " to shared queue " << bestQueue << " with score " << bestScore);
               
  return bestQueue;
}

} // namespace ns3
