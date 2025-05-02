import subprocess
import re

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

def create_table(data, metric):
    """Create a comparison table for a specific metric"""
    # Print header
    print(f"\n{metric}:")
    print(f"{'Scheduler':<12}", end="")
    for workload in workloads.values():
        print(f"{workload:<12}", end="")
    print()
    print("-" * (12 + 12 * len(workloads)))
    
    # Print data
    for scheduler in schemes.values():
        print(f"{scheduler:<12}", end="")
        for workload in workloads.values():
            for item in data:
                if item['Scheduler'] == scheduler and item['Workload'] == workload:
                    value = item[metric]
                    print(f"{value:<12}", end="")
                    break
            else:
                print(f"{'N/A':<12}", end="")
        print()

def calculate_rankings(data, metric):
    """Calculate rankings for each scheduler per workload"""
    rankings = {}
    
    for workload in workloads.values():
        workload_data = [item for item in data if item['Workload'] == workload]
        
        # Sort by metric value
        sorted_data = sorted(workload_data, key=lambda x: x[metric])
        
        # Assign ranks
        for rank, item in enumerate(sorted_data, 1):
            scheduler = item['Scheduler']
            if scheduler not in rankings:
                rankings[scheduler] = {}
            rankings[scheduler][workload] = rank
    
    return rankings

def main():
    # Collect data
    results = []
    
    print("Running simulations...\n")
    
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
    
    # Print comparison tables
    print("\n=== COMPARISON RESULTS ===")
    
    create_table(results, 'Energy Delay Product')
    create_table(results, 'Avg Turnaround Time (ms)')
    create_table(results, 'Total Energy (mJ)')
    create_table(results, 'Total Idle Time (ms)')
    
    # Calculate and print rankings
    print("\n=== RANKINGS (1 = best, 4 = worst) ===")
    
    for metric in ['Energy Delay Product', 'Avg Turnaround Time (ms)', 'Total Energy (mJ)']:
        rankings = calculate_rankings(results, metric)
        
        print(f"\nRankings for {metric}:")
        print(f"{'Scheduler':<12}", end="")
        for workload in workloads.values():
            print(f"{workload:<12}", end="")
        print()
        print("-" * (12 + 12 * len(workloads)))
        
        for scheduler in schemes.values():
            print(f"{scheduler:<12}", end="")
            for workload in workloads.values():
                if scheduler in rankings and workload in rankings[scheduler]:
                    rank = rankings[scheduler][workload]
                    print(f"{rank:<12}", end="")
                else:
                    print(f"{'N/A':<12}", end="")
            print()
    
    # Calculate overall performance score
    print("\n=== OVERALL PERFORMANCE SCORE ===")
    print("(Average rank across all metrics and workloads, lower is better)")
    
    overall_score = {}
    for scheduler in schemes.values():
        total_rank = 0
        count = 0
        for metric in ['Energy Delay Product', 'Avg Turnaround Time (ms)', 'Total Energy (mJ)']:
            rankings = calculate_rankings(results, metric)
            for workload in workloads.values():
                if scheduler in rankings and workload in rankings[scheduler]:
                    total_rank += rankings[scheduler][workload]
                    count += 1
        if count > 0:
            overall_score[scheduler] = total_rank / count
    
    # Sort and print overall scores
    for scheduler, score in sorted(overall_score.items(), key=lambda x: x[1]):
        print(f"{scheduler}: {score:.2f}")
    
    # Save results to CSV file
    with open('scheduler_comparison_results.csv', 'w') as f:
        # Write header
        f.write("Scheduler,Workload,Total Time (ms),Avg Turnaround Time (ms),Total Energy (mJ),Total Idle Time (ms),Energy Delay Product\n")
        
        # Write data
        for result in results:
            f.write(f"{result['Scheduler']},{result['Workload']},{result['Total Time (ms)']},{result['Avg Turnaround Time (ms)']},{result['Total Energy (mJ)']},{result['Total Idle Time (ms)']},{result['Energy Delay Product']}\n")
    
    print("\nResults saved to scheduler_comparison_results.csv")

if __name__ == "__main__":
    main()