#!/usr/bin/env python3

import os
import glob
import pandas as pd
import matplotlib.pyplot as plt
import seaborn as sns
import argparse

# analyze-results.py
# Reads all CSVs from results/raw/seed-*/ and generates summary statistics and plots.

def parse_args():
    parser = argparse.ArgumentParser()
    parser.add_argument('--input_dir', default='results/raw', help='Directory containing seed-*/ folders')
    parser.add_argument('--output_summary', default='results/summary', help='Directory to save summary CSVs')
    parser.add_argument('--output_plots', default='results/plots', help='Directory to save plots')
    return parser.parse_args()

def main():
    args = parse_args()
    os.makedirs(args.output_summary, exist_ok=True)
    os.makedirs(args.output_plots, exist_ok=True)

    # Load the summary.csv data
    csv_files = glob.glob(os.path.join(args.input_dir, 'seed-*', 'summary.csv'))
    if not csv_files:
        print("No CSV files found in", args.input_dir)
        return
        
    df = pd.concat([pd.read_csv(f) for f in csv_files])
    
    # Mode mapping
    mode_names = {
        0: 'NoCtrl', 1: 'BaseCC', 2: 'CBFC', 3: 'FlowStar', 
        4: 'FS+I1', 5: 'FS+I2', 6: 'FS+I3', 7: 'FS+I4', 
        8: 'Improved', 9: 'FS+I12', 10: 'FS+I123'
    }
    df['mode_name'] = df['mode'].map(mode_names)
    
    sns.set_theme(style="whitegrid")
    
    # Plot 2: Throughput (sum of Tx)
    plt.figure(figsize=(12, 6))
    sns.barplot(data=df, x='mode_name', y='tx', hue='workload')
    plt.title("Aggregate Throughput (Tx) across Modes and Workloads")
    plt.xlabel("Configuration Mode")
    plt.ylabel("Total Packets Transmitted")
    plt.xticks(rotation=45)
    plt.tight_layout()
    plt.savefig(os.path.join(args.output_plots, 'throughput.png'))
    plt.close()

    # Plot 3: Queue Occupancy Over Time
    plt.figure(figsize=(12, 6))
    sns.barplot(data=df, x='mode_name', y='max_queue', hue='workload')
    plt.title("Maximum Queue Occupancy across Modes and Workloads")
    plt.xlabel("Configuration Mode")
    plt.ylabel("Max Queue Occupancy (Packets)")
    plt.xticks(rotation=45)
    plt.tight_layout()
    plt.savefig(os.path.join(args.output_plots, 'queue-occupancy.png'))
    plt.close()

    # Create dummy plots for the rest that need timeseries data we didn't collect
    plt.figure(figsize=(10, 6))
    plt.title("Graph 1: Flow Completion Time (FCT) vs Configuration - (Placeholder: Requires Per-flow timeseries)")
    plt.savefig(os.path.join(args.output_plots, 'fct.png'))
    plt.close()

    plt.figure(figsize=(10, 6))
    plt.title("Graph 4: Rate Recovery Over Time (I4) - (Placeholder: Requires Rate trace)")
    plt.savefig(os.path.join(args.output_plots, 'rate-response.png'))
    plt.close()

    plt.figure(figsize=(10, 6))
    plt.title("Graph 5: Per-Flow FCT (W3: Queue Exhaustion) - (Placeholder)")
    plt.savefig(os.path.join(args.output_plots, 'per-flow-fct.png'))
    plt.close()

    plt.figure(figsize=(10, 6))
    plt.title("Graph 6: Link Utilization (W4: Multipath) - (Placeholder)")
    plt.savefig(os.path.join(args.output_plots, 'link-utilization.png'))
    plt.close()
    
    # Save a summary
    summary_df = df.groupby(['mode_name', 'workload']).mean(numeric_only=True).reset_index()
    summary_df.to_csv(os.path.join(args.output_summary, 'manifest.csv'), index=False)

    print(f"Summary saved to {args.output_summary}/")
    print(f"Plots saved to {args.output_plots}/")

if __name__ == '__main__':
    main()
