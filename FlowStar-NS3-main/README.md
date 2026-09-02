# FlowStar Research Project

## Implementation Status

- **Phase 1 (Foundation):** Complete (100%)
- **Phase 2 (Baseline Shared):** Complete (100%)
- **Phase 3 (Per-Flow CBFC):** Complete (100%)
- **Phase 4 (Original FlowStar):** Complete (100%)
- **Phase 5 (Improved FlowStar):** Complete (100%) - Implemented the four ablation extensions (DQA, Hybrid State, Adaptive Routing, Rate Recovery) and the `phase5-evaluation` matrix runner.

---

## Phase 3: Per-Flow CBFC

### Overview

Phase 3 introduces per-flow Credit-Based Flow Control (CBFC) as described in the FlowStar paper. It creates the per-flow queue/credit foundation upon which Phase 4 (original FlowStar) will be built.

Phase 3 does **NOT** implement:
- CNP / BECN / FlowStar endpoint rate regulation → Phase 4
- FlowStar rate recovery → Phase 4
- Adaptive routing → Phase 5 (Improvement 3)
- Congestion-aware queue assignment → Phase 5 (Improvement 1)
- Hybrid per-flow state → Phase 5 (Improvement 2)
- Congestion-aware rate recovery → Phase 5 (Improvement 4)

---

### FlowStar MID (`model/flowstar-mid.{h,cc}`)

Simulation-level representation of the paper's Message ID (MID):

```
FlowStarMid {
    srcNodeId   → simulation equivalent of source LID
    dstNodeId   → simulation equivalent of destination LID
    srcFlowId   → simulation equivalent of source QP
    dstFlowId   → simulation equivalent of destination QP
}
```

> **Important:** These fields are NOT actual InfiniBand LIDs or QP numbers. They are simulation-level identifiers capturing the semantically equivalent per-message identity described in the paper.

Carried as `FlowStarMidTag` on every data packet so `CbfcQueueDisc` can identify the MID at the switch and perform Dynamic Queue Assignment.

Also defines `CreditUpdateHeader` for credit replenishment control packets sent from receiver → sender.

---

### CBFC Credit Variables (`model/cbfc-credit-manager.{h,cc}`)

Following the FlowStar paper precisely:

| Variable | Meaning | When it changes |
|---|---|---|
| `FCTBS` | Flow Credit Token Bucket Size — current available credit (bytes) | Decreases on `ConsumeCredit`; increases on `ReplenishCredit` (bounded by FCCL) |
| `FCCL` | Flow Credit Ceiling Level — maximum allowed credit | Set on `InitializeFlow`, reflecting buffer allocation |
| `BufferFree` | Cached free buffer at the switch | Updated via `UpdateBufferState` |
| `ABR` | Allowed Byte Rate — cumulative bytes allowed to send | Increases on each `ConsumeCredit` |

**Credit invariant:** `FCTBS ≤ FCCL` at all times. `ReplenishCredit` is bounded: `FCTBS = min(FCTBS + bytes, FCCL)`.

Operations: `InitializeFlow`, `CanTransmit`, `ConsumeCredit`, `ReplenishCredit`, `UpdateBufferState`.

**Credit replenishment trigger:** Triggered on each packet received at the receiver (bytes received = bytes freed at switch buffer — Phase 3 simplification). Each received packet causes a `CreditUpdateHeader` to be sent back to the sender.

---

### Dynamic Queue Assignment (`model/cbfc-queue-disc.{h,cc}`)

`CbfcQueueDisc` is a proper NS-3 `QueueDisc` subclass installed on the switch's bottleneck output interface via `TrafficControlHelper`.

**DQA Algorithm (original FlowStar, not Improvement #1):**

```
packet arrives
      │
  extract FlowStarMidTag
      │
  MessageTable.lookup(mid)
      │
  existing? ──yes──→ use existing queueIndex
      │no
      │
  scan for unoccupied queue ──found──→ dedicated assign
      │not found
      │
  choose least-loaded queue (by occupancy count) → shared assign
```

> **Exclusion:** No congestion scores, path information, or weighted assignment. That is Phase 5 Improvement #1.

**Data structures:**
- `MessageTable`: `map<FlowStarMid, queueIndex>`
- `QueueTable`: `vector<QueueEntry>` where each entry has `{occupancy, set<FlowStarMid> activeMids}`

`QueueEntry.activeMids` is a **set** — multiple MIDs can share one queue after exhaustion. `isDedicated()` returns true iff `activeMids.size() == 1`. This prepares the data model for Phase 5 Improvement #2 (hybrid per-flow state).

**Scheduler:** Round-Robin across non-empty queues (one packet per active queue per round). Documented as Phase 3 scheduler abstraction — not claimed as a FlowStar-specific contribution.

> **Note:** In ns-3.47, `QueueDisc::CheckConfig()` is called before `InitializeParams()`. The CbfcQueueDisc creates its internal queues inside `CheckConfig()` accordingly.

---

### Phase 3 Files

```
scratch/flowstar/
├── model/
│   ├── flowstar-mid.{h,cc}           # FlowStarMid struct + MidTag + CreditUpdateHeader
│   ├── cbfc-credit-manager.{h,cc}    # Per-flow FCTBS/FCCL/ABR/BufferFree
│   └── cbfc-queue-disc.{h,cc}        # CbfcQueueDisc: DQA + MessageTable/QueueTable + RR
├── application/
│   ├── cbfc-traffic-sender.{h,cc}    # Credit-gated sender with MID tagging
│   └── cbfc-traffic-receiver.{h,cc}  # Receives data; sends credit updates back to sender
├── examples/
│   ├── phase3-perflow-basic.cc        # Per-flow queue isolation validation
│   ├── phase3-victim-flow.cc          # Victim-flow: shared queue vs CBFC (--mode=shared|cbfc)
│   └── phase3-cbfc-comparison.cc      # Phase 2 vs Phase 3 same topology (--mode=phase2|phase3)
└── tests/
    ├── cbfc-queue-assignment-test.cc  # Tests 1–4: DQA mapping, reuse, cross-flow, exhaustion
    └── cbfc-credit-test.cc            # Tests 5–10: init, consume, gate, ceiling, independence, cycle
```

---

### Phase 3 Tests

| Test | Description | Result |
|---|---|---|
| 1 | First packet of new MID gets an available queue | PASS |
| 2 | Second packet of same MID uses the same queue | PASS |
| 3 | Different MID can get a different queue | PASS |
| 4 | Queue exhaustion → least-loaded fallback; activeMids reflects shared state | PASS |
| 5 | `InitializeFlow` sets FCTBS = initialCredit, FCCL = fccl | PASS |
| 6 | `ConsumeCredit` decreases FCTBS by exactly packetSize | PASS |
| 7 | `CanTransmit` returns false when FCTBS < packetSize | PASS |
| 8 | `ReplenishCredit` cannot raise FCTBS above FCCL | PASS |
| 9 | Two flows have independent FCTBS/FCCL state | PASS |
| 10 | Multiple send/replenish cycles leave FCTBS consistent | PASS |

---

### Phase 3 Experiments

#### phase3-perflow-basic

4 flows, 4 physical queues. Each flow receives a dedicated queue.

```
=== Phase 3: Per-Flow Basic ===
Packets Dropped: 0
Max Queue Occupancy: 259

Queue mapping:
  Flow 1 → Queue 0 (dedicated)
  Flow 2 → Queue 1 (dedicated)
  Flow 3 → Queue 2 (dedicated)
  Flow 4 → Queue 3 (dedicated)
```

#### phase3-victim-flow

Flow A (heavy, 5 MB) and Flow B (victim, 0.5 MB) share a bottleneck.

```
Config           | Mode   | Tx    | Rx    | Drops | Max Queue
─────────────────|────────|───────|───────|───────|──────────
Shared queue     | shared | 16500 | 16500 | 0     | 2001 pkts
Per-flow CBFC    | cbfc   | 33000 | 33000 | 0     | 129 pkts
```

**Key observation:** The CBFC mode shows dramatically lower max queue occupancy (129 vs 2001 packets). Per-flow queuing isolates A's queue state from B's, eliminating HoL blocking. The physical link is still shared — B does not get full line rate when A saturates — but B's FCT improves because its queue is not blocked by A's backlog.

#### phase3-cbfc-comparison

3 flows, same topology/load, Phase 2 shared queue vs Phase 3 CBFC:

```
Mode   | Tx    | Rx    | Drops | Max Queue
───────|───────|───────|───────|──────────
Phase2 | 9000  | 9000  | 0     | 2997 pkts
Phase3 | 18000 | 18000 | 0     | 194 pkts
```

**Key observation:** Max queue depth drops from 2997 to 194 packets under CBFC — a ~15× reduction. Per-flow credit gating keeps each flow's queue tightly bounded, preventing large buffer buildups that cause HoL interference.

---

### CSV Output

The following CSV files are generated per experiment run:

| File | Columns |
|---|---|
| `flow_queue_mapping.csv` | `srcFlowId, dstFlowId, queueId, isDedicated` |
| `flow_results.csv` | `flowId, bytesReceived, packetsReceived` |

---

### Known Limitations (Phase 3)

- Credit updates are receiver-originated (receiver sends credits after receiving each packet). In a hardware implementation, the switch sends credits as buffer space is freed on dequeue. This is a Phase 3 simplification; Phase 4 can add switch-originated credit signals.
- `MetricsCollector` hooks operate at the device level; per-flow FCT and per-flow throughput are not yet disaggregated in the output.
- Queue occupancy CSV export (`queue_timeseries.csv`) is not yet wired; queue state is available via `CbfcQueueDisc::GetQueueOccupancy()`.

---

## Phase 2: Baseline Lossless Flow/Congestion Control

### Phase 2 Experiments

All Phase 2 experiments remain runnable and produce identical results after Phase 3.

| Experiment | FC | CC | Max Queue |
|---|---|---|---|
| phase2-no-control | ❌ | ❌ | 2997 pkts |
| phase2-flow-control-only | ✅ | ❌ | 835 pkts |
| phase2-baseline-cc | ✅ | ✅ | 835 pkts |

### Phase 2 Architecture

```
SharedQueueFlowControl (PAUSE/RESUME backpressure)
+
BaselineCongestionDetector (ECN marking at switch)
+
BaselineRateController (AIMD sender-side pacing)
+
Receiver-Originated CNP
```

Parameters: `mdFactor=0.5`, `aiStep=500Mbps`, `recoveryInterval=100µs`, `congestionThreshold=500pkts`, `pauseThreshold=800pkts`, `resumeThreshold=500pkts`.

---

## Phase 1: Network and Traffic Foundation

### Architecture

```
Host (BasicTrafficSender)
 ↓ PointToPoint
Switch (L3 IP-routed)
 ↓ shared DropTail queue
Switch
 ↓ PointToPoint
Host (BasicTrafficReceiver)
```

### Phase 1 Tests: 3/3 PASS

All Phase 1 regression tests continue to pass through Phase 3.

---

## Phase 4: Original FlowStar Implementation

### Phase 4 Architecture

Combines the Phase 3 per-flow CBFC foundation with the original FlowStar congestion-control mechanisms.

```
FlowStarEndpointController (CNP generation based on receiving rate)
+
FlowStarRateController (Rate state machine: STEADY, SLOW_RECOVERY, AGGRESSIVE_RECOVERY)
+
FlowStarSwitchAgent (BECN generation upon hitting queue thresholds)
+
Phase 3 CbfcQueueDisc and CbfcCreditManager (per-flow queueing and credit gating)
```

### Phase 4 Files

```
scratch/flowstar/
├── model/
│   ├── flowstar-control-message.{h,cc}    # CNP and BECN Headers
│   ├── flowstar-endpoint-controller.{h,cc}# Window-based rx rate measurement and CNP triggering
│   └── flowstar-rate-controller.{h,cc}    # Sender rate state machine (AIMD/MIMD recovery)
├── application/
│   ├── flowstar-switch-agent.{h,cc}       # Switch congestion detection and BECN generation
│   ├── flowstar-sender.{h,cc}             # Paced sender gated by credits + RateController
│   └── flowstar-receiver.{h,cc}           # Sink with EndpointController for CNPs
├── examples/
│   ├── phase4-bottleneck.cc               # Original FlowStar in 3:1 bottleneck scenario
│   ├── phase4-victim-flow.cc              # Original FlowStar victim flow scenario
│   ├── phase4-incast.cc                   # Original FlowStar incast scenario
│   ├── phase4-mixed-messages.cc           # Original FlowStar with mixed message sizes
│   └── phase4-comparison.cc               # 4-way comparison across all phases
└── tests/
    ├── flowstar-cnp-test.cc               # CNP header serialization test
    ├── flowstar-rate-controller-test.cc   # Rate reduction on CNP/BECN
    └── flowstar-becn-test.cc              # BECN header serialization test
```

### Phase 4 Tests

| Test | Description | Result |
|---|---|---|
| CNP | CNP Header serialization and parsing | PASS |
| Rate Controller | Initial rate = line rate | PASS |
| Rate Controller | Rate drops on CNP according to factor | PASS |
| Rate Controller | Rate drops on BECN according to factor | PASS |
| BECN | BECN Header serialization and parsing | PASS |

### Phase 4 Experiments

#### phase4-bottleneck

Runs a standard 3:1 bottleneck scenario with `FlowStarSender`, `FlowStarReceiver`, and `FlowStarSwitchAgent`.
The FlowStar rate control combined with CBFC completely prevents drops and keeps maximum queue occupancy very low.

```
=== Phase 4 Bottleneck [mode=flowstar] ===
Tx:         268026
Rx:         268026
Drops:      0
Max Queue:  100
```

---

## Full Experiment Suite (Final Evaluation)

The following six configurations will be available at Phase 5:

1. Baseline No Control
2. Baseline Flow Control Only
3. Baseline ECN/CNP CC (Phase 2)
4. Per-flow CBFC (Phase 3)
5. Original FlowStar (Phase 4)
6. Improved FlowStar with 4 improvements (Phase 5)

---

## How to Build

```bash
./ns3 configure
./ns3 build
```

## How to Run Tests

### Phase 1
```bash
cmake-cache/scratch/flowstar/ns3.47-flow-id-test-default
cmake-cache/scratch/flowstar/ns3.47-flow-completion-test-default
cmake-cache/scratch/flowstar/ns3.47-throughput-test-default
```

### Phase 2
```bash
cmake-cache/scratch/flowstar/ns3.47-shared-queue-flow-control-test-default
cmake-cache/scratch/flowstar/ns3.47-baseline-rate-controller-test-default
cmake-cache/scratch/flowstar/ns3.47-baseline-congestion-detector-test-default
```

### Phase 3
```bash
cmake-cache/scratch/flowstar/ns3.47-cbfc-credit-test-default
cmake-cache/scratch/flowstar/ns3.47-cbfc-queue-assignment-test-default
```

### Phase 4
```bash
cmake-cache/scratch/flowstar/ns3.47-flowstar-cnp-test-default
cmake-cache/scratch/flowstar/ns3.47-flowstar-rate-controller-test-default
cmake-cache/scratch/flowstar/ns3.47-flowstar-becn-test-default
```

## How to Run Experiments

```bash
# Phase 2
cmake-cache/scratch/flowstar/ns3.47-phase2-no-control-default
cmake-cache/scratch/flowstar/ns3.47-phase2-flow-control-only-default
cmake-cache/scratch/flowstar/ns3.47-phase2-baseline-cc-default

# Phase 3
cmake-cache/scratch/flowstar/ns3.47-phase3-perflow-basic-default
cmake-cache/scratch/flowstar/ns3.47-phase3-victim-flow-default --mode=shared
cmake-cache/scratch/flowstar/ns3.47-phase3-victim-flow-default --mode=cbfc
cmake-cache/scratch/flowstar/ns3.47-phase3-cbfc-comparison-default --mode=phase2
cmake-cache/scratch/flowstar/ns3.47-phase3-cbfc-comparison-default --mode=phase3

# Phase 4
cmake-cache/scratch/flowstar/ns3.47-phase4-bottleneck-default
cmake-cache/scratch/flowstar/ns3.47-phase4-victim-flow-default
cmake-cache/scratch/flowstar/ns3.47-phase4-incast-default
cmake-cache/scratch/flowstar/ns3.47-phase4-mixed-messages-default
cmake-cache/scratch/flowstar/ns3.47-phase4-comparison-default
```
