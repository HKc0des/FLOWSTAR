cmake_minimum_required(VERSION 3.10)

# ============================================================
# Phase 1 core sources
# ============================================================
set(FLOWSTAR_PHASE1_SOURCES
    model/topology-builder.cc
    model/basic-flow-id-tag.cc
    application/basic-traffic-sender.cc
    application/basic-traffic-receiver.cc
    utils/metrics-collector.cc
    utils/csv-writer.cc
)

# ============================================================
# Phase 2 baseline flow/congestion control sources
# ============================================================
set(FLOWSTAR_PHASE2_SOURCES
    model/shared-queue-flow-control.cc
    model/baseline-control-message.cc
    model/baseline-congestion-detector.cc
    model/baseline-rate-controller.cc
    application/baseline-traffic-sender.cc
    application/baseline-traffic-receiver.cc
)

# ============================================================
# Phase 3 per-flow CBFC sources
# ============================================================
set(FLOWSTAR_PHASE3_SOURCES
    model/flowstar-mid.cc
    model/cbfc-credit-manager.cc
    model/cbfc-queue-disc.cc
    application/cbfc-traffic-sender.cc
    application/cbfc-traffic-receiver.cc
)

# ============================================================
# Phase 4 original FlowStar sources
# ============================================================
set(FLOWSTAR_PHASE4_SOURCES
    model/flowstar-control-message.cc
    model/flowstar-endpoint-controller.cc
    model/flowstar-rate-controller.cc
    application/flowstar-switch-agent.cc
    application/flowstar-sender.cc
    application/flowstar-receiver.cc
)

# ============================================================
# Phase 5 Improved FlowStar sources
# ============================================================
set(FLOWSTAR_PHASE5_SOURCES
    model/improved-cbfc-queue-disc.cc
    model/flowstar-path-manager.cc
    model/improved-flowstar-rate-controller.cc
)

# Combined sets
set(FLOWSTAR_P1P2_SOURCES ${FLOWSTAR_PHASE1_SOURCES} ${FLOWSTAR_PHASE2_SOURCES})
set(FLOWSTAR_ALL_SOURCES  ${FLOWSTAR_PHASE1_SOURCES} ${FLOWSTAR_PHASE2_SOURCES} ${FLOWSTAR_PHASE3_SOURCES} ${FLOWSTAR_PHASE4_SOURCES} ${FLOWSTAR_PHASE5_SOURCES})

# ============================================================
# Phase 1 examples
# ============================================================
build_exec(EXECNAME phase1-connectivity     EXECNAME_PREFIX scratch_flowstar_
    SOURCE_FILES examples/phase1-connectivity.cc     ${FLOWSTAR_PHASE1_SOURCES}
    LIBRARIES_TO_LINK ${ns3-libs} EXECUTABLE_DIRECTORY_PATH ${CMAKE_CURRENT_BINARY_DIR}/)

build_exec(EXECNAME phase1-multiple-flows   EXECNAME_PREFIX scratch_flowstar_
    SOURCE_FILES examples/phase1-multiple-flows.cc   ${FLOWSTAR_PHASE1_SOURCES}
    LIBRARIES_TO_LINK ${ns3-libs} EXECUTABLE_DIRECTORY_PATH ${CMAKE_CURRENT_BINARY_DIR}/)

build_exec(EXECNAME phase1-bottleneck       EXECNAME_PREFIX scratch_flowstar_
    SOURCE_FILES examples/phase1-bottleneck.cc       ${FLOWSTAR_PHASE1_SOURCES}
    LIBRARIES_TO_LINK ${ns3-libs} EXECUTABLE_DIRECTORY_PATH ${CMAKE_CURRENT_BINARY_DIR}/)

# ============================================================
# Phase 2 examples
# ============================================================
build_exec(EXECNAME phase2-no-control       EXECNAME_PREFIX scratch_flowstar_
    SOURCE_FILES examples/phase2-no-control.cc       ${FLOWSTAR_P1P2_SOURCES}
    LIBRARIES_TO_LINK ${ns3-libs} EXECUTABLE_DIRECTORY_PATH ${CMAKE_CURRENT_BINARY_DIR}/)

build_exec(EXECNAME phase2-flow-control-only EXECNAME_PREFIX scratch_flowstar_
    SOURCE_FILES examples/phase2-flow-control-only.cc ${FLOWSTAR_P1P2_SOURCES}
    LIBRARIES_TO_LINK ${ns3-libs} EXECUTABLE_DIRECTORY_PATH ${CMAKE_CURRENT_BINARY_DIR}/)

build_exec(EXECNAME phase2-baseline-cc      EXECNAME_PREFIX scratch_flowstar_
    SOURCE_FILES examples/phase2-baseline-cc.cc      ${FLOWSTAR_P1P2_SOURCES}
    LIBRARIES_TO_LINK ${ns3-libs} EXECUTABLE_DIRECTORY_PATH ${CMAKE_CURRENT_BINARY_DIR}/)

# ============================================================
# Phase 3 examples
# ============================================================
build_exec(EXECNAME phase3-perflow-basic    EXECNAME_PREFIX scratch_flowstar_
    SOURCE_FILES examples/phase3-perflow-basic.cc    ${FLOWSTAR_ALL_SOURCES}
    LIBRARIES_TO_LINK ${ns3-libs} EXECUTABLE_DIRECTORY_PATH ${CMAKE_CURRENT_BINARY_DIR}/)

build_exec(EXECNAME phase3-victim-flow      EXECNAME_PREFIX scratch_flowstar_
    SOURCE_FILES examples/phase3-victim-flow.cc      ${FLOWSTAR_ALL_SOURCES}
    LIBRARIES_TO_LINK ${ns3-libs} EXECUTABLE_DIRECTORY_PATH ${CMAKE_CURRENT_BINARY_DIR}/)

build_exec(EXECNAME phase3-cbfc-comparison  EXECNAME_PREFIX scratch_flowstar_
    SOURCE_FILES examples/phase3-cbfc-comparison.cc  ${FLOWSTAR_ALL_SOURCES}
    LIBRARIES_TO_LINK ${ns3-libs} EXECUTABLE_DIRECTORY_PATH ${CMAKE_CURRENT_BINARY_DIR}/)

# ============================================================
# Phase 1 tests
# ============================================================
build_exec(EXECNAME flow-id-test            EXECNAME_PREFIX scratch_flowstar_
    SOURCE_FILES tests/flow-id-test.cc               ${FLOWSTAR_PHASE1_SOURCES}
    LIBRARIES_TO_LINK ${ns3-libs} EXECUTABLE_DIRECTORY_PATH ${CMAKE_CURRENT_BINARY_DIR}/)

build_exec(EXECNAME flow-completion-test    EXECNAME_PREFIX scratch_flowstar_
    SOURCE_FILES tests/flow-completion-test.cc       ${FLOWSTAR_PHASE1_SOURCES}
    LIBRARIES_TO_LINK ${ns3-libs} EXECUTABLE_DIRECTORY_PATH ${CMAKE_CURRENT_BINARY_DIR}/)

build_exec(EXECNAME throughput-test         EXECNAME_PREFIX scratch_flowstar_
    SOURCE_FILES tests/throughput-test.cc            ${FLOWSTAR_PHASE1_SOURCES}
    LIBRARIES_TO_LINK ${ns3-libs} EXECUTABLE_DIRECTORY_PATH ${CMAKE_CURRENT_BINARY_DIR}/)

# ============================================================
# Phase 2 tests
# ============================================================
build_exec(EXECNAME shared-queue-flow-control-test EXECNAME_PREFIX scratch_flowstar_
    SOURCE_FILES tests/shared-queue-flow-control-test.cc ${FLOWSTAR_P1P2_SOURCES}
    LIBRARIES_TO_LINK ${ns3-libs} EXECUTABLE_DIRECTORY_PATH ${CMAKE_CURRENT_BINARY_DIR}/)

build_exec(EXECNAME baseline-rate-controller-test  EXECNAME_PREFIX scratch_flowstar_
    SOURCE_FILES tests/baseline-rate-controller-test.cc  ${FLOWSTAR_P1P2_SOURCES}
    LIBRARIES_TO_LINK ${ns3-libs} EXECUTABLE_DIRECTORY_PATH ${CMAKE_CURRENT_BINARY_DIR}/)

build_exec(EXECNAME baseline-congestion-detector-test EXECNAME_PREFIX scratch_flowstar_
    SOURCE_FILES tests/baseline-congestion-detector-test.cc ${FLOWSTAR_P1P2_SOURCES}
    LIBRARIES_TO_LINK ${ns3-libs} EXECUTABLE_DIRECTORY_PATH ${CMAKE_CURRENT_BINARY_DIR}/)

# ============================================================
# Phase 3 tests
# ============================================================
build_exec(EXECNAME cbfc-queue-assignment-test EXECNAME_PREFIX scratch_flowstar_
    SOURCE_FILES tests/cbfc-queue-assignment-test.cc ${FLOWSTAR_ALL_SOURCES}
    LIBRARIES_TO_LINK ${ns3-libs} EXECUTABLE_DIRECTORY_PATH ${CMAKE_CURRENT_BINARY_DIR}/)

build_exec(EXECNAME cbfc-credit-test           EXECNAME_PREFIX scratch_flowstar_
    SOURCE_FILES tests/cbfc-credit-test.cc           ${FLOWSTAR_ALL_SOURCES}
    LIBRARIES_TO_LINK ${ns3-libs} EXECUTABLE_DIRECTORY_PATH ${CMAKE_CURRENT_BINARY_DIR}/)

# ============================================================
# Phase 4 tests
# ============================================================
build_exec(EXECNAME flowstar-cnp-test               EXECNAME_PREFIX scratch_flowstar_
    SOURCE_FILES tests/flowstar-cnp-test.cc         ${FLOWSTAR_ALL_SOURCES}
    LIBRARIES_TO_LINK ${ns3-libs} EXECUTABLE_DIRECTORY_PATH ${CMAKE_CURRENT_BINARY_DIR}/)

build_exec(EXECNAME flowstar-rate-controller-test   EXECNAME_PREFIX scratch_flowstar_
    SOURCE_FILES tests/flowstar-rate-controller-test.cc ${FLOWSTAR_ALL_SOURCES}
    LIBRARIES_TO_LINK ${ns3-libs} EXECUTABLE_DIRECTORY_PATH ${CMAKE_CURRENT_BINARY_DIR}/)

build_exec(EXECNAME flowstar-becn-test              EXECNAME_PREFIX scratch_flowstar_
    SOURCE_FILES tests/flowstar-becn-test.cc        ${FLOWSTAR_ALL_SOURCES}
    LIBRARIES_TO_LINK ${ns3-libs} EXECUTABLE_DIRECTORY_PATH ${CMAKE_CURRENT_BINARY_DIR}/)

# ============================================================
# Phase 4 examples
# ============================================================
build_exec(EXECNAME phase4-bottleneck               EXECNAME_PREFIX scratch_flowstar_
    SOURCE_FILES examples/phase4-bottleneck.cc      ${FLOWSTAR_ALL_SOURCES}
    LIBRARIES_TO_LINK ${ns3-libs} EXECUTABLE_DIRECTORY_PATH ${CMAKE_CURRENT_BINARY_DIR}/)

build_exec(EXECNAME phase4-comparison               EXECNAME_PREFIX scratch_flowstar_
    SOURCE_FILES examples/phase4-comparison.cc      ${FLOWSTAR_ALL_SOURCES}
    LIBRARIES_TO_LINK ${ns3-libs} EXECUTABLE_DIRECTORY_PATH ${CMAKE_CURRENT_BINARY_DIR}/)

build_exec(EXECNAME phase4-victim-flow              EXECNAME_PREFIX scratch_flowstar_
    SOURCE_FILES examples/phase4-victim-flow.cc     ${FLOWSTAR_ALL_SOURCES}
    LIBRARIES_TO_LINK ${ns3-libs} EXECUTABLE_DIRECTORY_PATH ${CMAKE_CURRENT_BINARY_DIR}/)

build_exec(EXECNAME phase4-incast                   EXECNAME_PREFIX scratch_flowstar_
    SOURCE_FILES examples/phase4-incast.cc          ${FLOWSTAR_ALL_SOURCES}
    LIBRARIES_TO_LINK ${ns3-libs} EXECUTABLE_DIRECTORY_PATH ${CMAKE_CURRENT_BINARY_DIR}/)

build_exec(EXECNAME phase4-mixed-messages           EXECNAME_PREFIX scratch_flowstar_
    SOURCE_FILES examples/phase4-mixed-messages.cc  ${FLOWSTAR_ALL_SOURCES}
    LIBRARIES_TO_LINK ${ns3-libs} EXECUTABLE_DIRECTORY_PATH ${CMAKE_CURRENT_BINARY_DIR}/)

# ============================================================
# Phase 5 tests
# ============================================================
build_exec(EXECNAME improved-cbfc-queue-assignment-test EXECNAME_PREFIX scratch_flowstar_
    SOURCE_FILES tests/improved-cbfc-queue-assignment-test.cc ${FLOWSTAR_ALL_SOURCES}
    LIBRARIES_TO_LINK ${ns3-libs} EXECUTABLE_DIRECTORY_PATH ${CMAKE_CURRENT_BINARY_DIR}/)

build_exec(EXECNAME improved-hybrid-state-test EXECNAME_PREFIX scratch_flowstar_
    SOURCE_FILES tests/improved-hybrid-state-test.cc ${FLOWSTAR_ALL_SOURCES}
    LIBRARIES_TO_LINK ${ns3-libs} EXECUTABLE_DIRECTORY_PATH ${CMAKE_CURRENT_BINARY_DIR}/)

build_exec(EXECNAME adaptive-routing-test EXECNAME_PREFIX scratch_flowstar_
    SOURCE_FILES tests/adaptive-routing-test.cc ${FLOWSTAR_ALL_SOURCES}
    LIBRARIES_TO_LINK ${ns3-libs} EXECUTABLE_DIRECTORY_PATH ${CMAKE_CURRENT_BINARY_DIR}/)

build_exec(EXECNAME improved-rate-controller-test EXECNAME_PREFIX scratch_flowstar_
    SOURCE_FILES tests/improved-rate-controller-test.cc ${FLOWSTAR_ALL_SOURCES}
    LIBRARIES_TO_LINK ${ns3-libs} EXECUTABLE_DIRECTORY_PATH ${CMAKE_CURRENT_BINARY_DIR}/)

# ============================================================
# Phase 5 examples
# ============================================================
build_exec(EXECNAME phase5-evaluation EXECNAME_PREFIX scratch_flowstar_
    SOURCE_FILES examples/phase5-evaluation.cc ${FLOWSTAR_ALL_SOURCES}
    LIBRARIES_TO_LINK ${ns3-libs} EXECUTABLE_DIRECTORY_PATH ${CMAKE_CURRENT_BINARY_DIR}/)
