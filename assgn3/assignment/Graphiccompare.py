import subprocess
import re
import pandas as pd
import matplotlib.pyplot as plt
import seaborn as sns

# Define scheduling schemes and workloads
schemes = {
    0: "Clustering",
    1: "Switcher",
    2: "Global",
    3: "Bonus"
}

workloads = {
    1: "IO bound",
    2: "CPU bound",
    3: "Mixed",
    4: "Sporadic"
}

def parse_output(output):
    """Parse the simulator output and extract metrics"""
    metrics = {}
    
    # Extract total time
    time_match = re.search(r'\[System\] Total Time\s+:\s+(\d+)', output)
    if time_match:
        metrics['total_time'] = int(time_match.group(1))
    
    # Extract average turnaround time
    tat_match = re.search(r'\[System\] Avg Turn Around Time\s+:\s+(\d+)', output)
    if tat_match:
        metrics['avg_turnaround_time'] = int(tat_match.group(1))
    
    # Extract energy delay product
    edp_match = re.search(r'\[System\] Energy Delay Product\s+:\s+(\d+)', output)
    if edp_match:
        metrics['energy_delay_product'] = int(edp_match.group(1))
    
    # Extract total energy consumed (sum of all cores)
    energy_total = 0
    energy_matches = re.findall(r'\[Core \d+\] Total energy consumed\s+:\s+(\d+)', output)
    for energy in energy_matches:
        energy_total += int(energy)
    metrics['total_energy'] = energy_total
    
    # Extract total idle time (sum of all cores)
    idle_total = 0
    idle_matches = re.findall(r'\[Core \d+\] Total time powered on and idle\s+:\s+(\d+)', output)
    for idle in idle_matches:
        idle_total += int(idle)
    metrics['total_idle_time'] = idle_total
    
    return metrics

def run_simulation(scheme, workload):
    """Run a single simulation and return metrics"""
    cmd = f"./sim {scheme} < testcases/testcases/workload{workload}.in"
    
    try:
        result = subprocess.run(cmd, shell=True, capture_output=True, text=True)
        if result.returncode != 0:
            print(f"Error running scheme {scheme} with workload {workload}")
            return None
        return parse_output(result.stdout)
    except Exception as e:
        print(f"Exception running scheme {scheme} with workload {workload}: {e}")
        return None

def main():
    # Collect data
    results = []
    
    for scheme_id, scheme_name in schemes.items():
        for workload_id, workload_name in workloads.items():
            print(f"Running {scheme_name} with {workload_name} workload...")
            metrics = run_simulation(scheme_id, workload_id)
            
            if metrics:
                result = {
                    'Scheduler': scheme_name,
                    'Workload': workload_name,
                    'Total Time (ms)': metrics['total_time'],
                    'Avg Turnaround Time (ms)': metrics['avg_turnaround_time'],
                    'Total Energy (mJ)': metrics['total_energy'],
                    'Total Idle Time (ms)': metrics['total_idle_time'],
                    'Energy Delay Product': metrics['energy_delay_product']
                }
                results.append(result)
    
    # Create DataFrame
    df = pd.DataFrame(results)
    
    # Create pivot tables for better comparison
    print("\n=== COMPARISON RESULTS ===\n")
    
    # Energy Delay Product comparison
    edp_pivot = df.pivot(index='Scheduler', columns='Workload', values='Energy Delay Product')
    print("Energy Delay Product (lower is better):")
    print(edp_pivot)
    print("\n")
    
    # Average Turnaround Time comparison
    tat_pivot = df.pivot(index='Scheduler', columns='Workload', values='Avg Turnaround Time (ms)')
    print("Average Turnaround Time (ms) (lower is better):")
    print(tat_pivot)
    print("\n")
    
    # Total Energy comparison
    energy_pivot = df.pivot(index='Scheduler', columns='Workload', values='Total Energy (mJ)')
    print("Total Energy Consumed (mJ) (lower is better):")
    print(energy_pivot)
    print("\n")
    
    # Create visualizations
    plt.style.use('seaborn')
    
    # EDP comparison
    fig, ax = plt.subplots(2, 2, figsize=(15, 12))
    
    # Bar plot for EDP
    edp_data = df.pivot(index='Workload', columns='Scheduler', values='Energy Delay Product')
    edp_data.plot(kind='bar', ax=ax[0, 0])
    ax[0, 0].set_title('Energy Delay Product by Workload and Scheduler')
    ax[0, 0].set_ylabel('Energy Delay Product')
    ax[0, 0].tick_params(axis='x', rotation=45)
    
    # Bar plot for Average Turnaround Time
    tat_data = df.pivot(index='Workload', columns='Scheduler', values='Avg Turnaround Time (ms)')
    tat_data.plot(kind='bar', ax=ax[0, 1])
    ax[0, 1].set_title('Average Turnaround Time by Workload and Scheduler')
    ax[0, 1].set_ylabel('Average Turnaround Time (ms)')
    ax[0, 1].tick_params(axis='x', rotation=45)
    
    # Bar plot for Total Energy
    energy_data = df.pivot(index='Workload', columns='Scheduler', values='Total Energy (mJ)')
    energy_data.plot(kind='bar', ax=ax[1, 0])
    ax[1, 0].set_title('Total Energy Consumed by Workload and Scheduler')
    ax[1, 0].set_ylabel('Total Energy (mJ)')
    ax[1, 0].tick_params(axis='x', rotation=45)
    
    # Stacked bar plot for efficiency (idle vs active time)
    # Calculate active time
    df['Active Time (ms)'] = df['Total Time (ms)'] * 8 - df['Total Idle Time (ms)']
    efficiency_data = df.pivot(index='Workload', columns='Scheduler', values=['Total Idle Time (ms)', 'Active Time (ms)'])
    
    # Create stacked bar plot
    idle_data = efficiency_data['Total Idle Time (ms)']
    active_data = efficiency_data['Active Time (ms)']
    
    x = range(len(workloads))
    width = 0.2
    
    for i, scheduler in enumerate(schemes.values()):
        ax[1, 1].bar([xi + i*width for xi in x], active_data[scheduler], width, label=f'{scheduler} (Active)')
        ax[1, 1].bar([xi + i*width for xi in x], idle_data[scheduler], width, 
                     bottom=active_data[scheduler], label=f'{scheduler} (Idle)', alpha=0.7)
    
    ax[1, 1].set_title('Core Utilization by Workload and Scheduler')
    ax[1, 1].set_ylabel('Time (ms)')
    ax[1, 1].set_xlabel('Workload')
    ax[1, 1].set_xticks([xi + width*1.5 for xi in x])
    ax[1, 1].set_xticklabels(workloads.values())
    ax[1, 1].legend(bbox_to_anchor=(1.05, 1), loc='upper left')
    ax[1, 1].tick_params(axis='x', rotation=45)
    
    plt.tight_layout()
    plt.savefig('scheduler_comparison.png', dpi=300, bbox_inches='tight')
    plt.close()
    
    # Create heatmap for EDP
    plt.figure(figsize=(10, 8))
    sns.heatmap(edp_pivot, annot=True, fmt='.0f', cmap='YlOrRd')
    plt.title('Energy Delay Product Heatmap')
    plt.tight_layout()
    plt.savefig('edp_heatmap.png', dpi=300, bbox_inches='tight')
    plt.close()
    
    # Save detailed results to CSV
    df.to_csv('scheduler_comparison_results.csv', index=False)
    
    # Calculate rankings for each metric and workload
    print("\n=== RANKINGS (1 = best, 4 = worst) ===\n")
    
    for metric in ['Energy Delay Product', 'Avg Turnaround Time (ms)', 'Total Energy (mJ)']:
        print(f"\nRankings for {metric}:")
        rankings = df.copy()
        for workload in workloads.values():
            workload_data = rankings[rankings['Workload'] == workload].copy()
            workload_data['Rank'] = workload_data[metric].rank(method='min')
            rankings.loc[rankings['Workload'] == workload, 'Rank'] = workload_data['Rank']
        
        ranking_pivot = rankings.pivot(index='Scheduler', columns='Workload', values='Rank')
        print(ranking_pivot)
        
    # Calculate overall performance score (average rank across all metrics and workloads)
    print("\n=== OVERALL PERFORMANCE SCORE ===")
    print("(Average rank across all metrics and workloads, lower is better)")
    
    overall_score = {}
    for scheduler in schemes.values():
        total_rank = 0
        count = 0
        for metric in ['Energy Delay Product', 'Avg Turnaround Time (ms)', 'Total Energy (mJ)']:
            for workload in workloads.values():
                workload_data = df[df['Workload'] == workload].copy()
                workload_data['Rank'] = workload_data[metric].rank(method='min')
                rank = workload_data[workload_data['Scheduler'] == scheduler]['Rank'].values[0]
                total_rank += rank
                count += 1
        overall_score[scheduler] = total_rank / count
    
    for scheduler, score in sorted(overall_score.items(), key=lambda x: x[1]):
        print(f"{scheduler}: {score:.2f}")

if __name__ == "__main__":
    main()