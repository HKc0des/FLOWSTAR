#!/bin/bash

# run-evaluations.sh
# Automates the Phase 6 canonical evaluation matrix

# Total seeds to run (can be overridden by argument)
SEEDS=${1:-10}

MODES=(0 1 2 3 4 5 6 7 8 9 10)
WORKLOADS=(1 2 3 4 5)

echo "Starting Evaluation Matrix. Seeds: $SEEDS"

cd ../../.. || exit 1

# Ensure ns-3 is built
./ns3 build

for seed in $(seq 1 $SEEDS); do
    outDir="results/raw/seed-$seed/"
    mkdir -p $outDir
    
    echo "=========================================================="
    echo "Running Seed $seed"
    echo "=========================================================="

    for mode in "${MODES[@]}"; do
        for workload in "${WORKLOADS[@]}"; do
            echo "Running Mode $mode, Workload $workload, Seed $seed..."
            
            ./cmake-cache/scratch/flowstar/ns3.47-phase5-evaluation-default --mode=$mode --workload=$workload --seed=$seed --outDir=$outDir > /dev/null
        done
    done
done

echo "All simulations completed successfully."
echo "Raw CSVs are available in results/raw/"
