#include "simulator.hpp"
#include <limits.h>
enum ProcessType { CPU_BOUND, IO_BOUND, BALANCED };

ProcessType classifyProcess(Process* process) {
    double cpuTime = 0.0;
    double ioTime = 0.0;
    
    auto pastBursts = process->getPastBursts();
    for (const auto& burst : pastBursts) {
        if (burst.first == BurstType::CPU) {
            cpuTime += burst.second;
        } else {
            ioTime += burst.second;
        }
    }
    
    if (pastBursts.empty()) {
        return BALANCED; // Not enough history
    }
    
    double total = cpuTime + ioTime;
    double cpuRatio = cpuTime / total;
    
    if (cpuRatio > 0.7) return CPU_BOUND;
    if (cpuRatio < 0.3) return IO_BOUND;
    return BALANCED;
}
void System::epoch(unsigned long currentTime) {
    if (schedulerScheme == SchedulerScheme::CLUSTERING) {
        // Initialize if first epoch
        if (currentTime == 0) {
            // Start with LITTLE cluster (cores 0-3) active
            for (int i = 0; i < 4; i++) {
                cores[i]->powerOnCore();
            }
            
            // Ensure big cluster is powered off
            for (int i = 4; i < 8; i++) {
                if (cores[i]->getState() == CoreState::POWER_ON) {
                    cores[i]->powerOffCore();
                }
            }
            
            // Schedule newly arrived processes to LITTLE cores in round-robin
            for (auto& entry : processes) {
                Process* process = entry.second;
                if (process->getState() == ProcessState::ARRIVED) {
                    unsigned coreId = process->getId() % 4;
                    scheduleProcess(process, cores[coreId]);
                }
            }
            return;
        }
        
        // Determine which cluster is currently active
        bool littleClusterActive = cores[0]->getState() == CoreState::POWER_ON;
        bool bigClusterActive = cores[4]->getState() == CoreState::POWER_ON;
        
        // Calculate average load for active cluster
        double avgLoad = 0.0;
        
        if (littleClusterActive) {
            // Calculate total load for LITTLE cluster
            unsigned long long totalLoad = 0;
            for (int i = 0; i < 4; i++) {
                if (!cores[i]->getLoadHistory().empty()) {
                    totalLoad += cores[i]->getLoadHistory().back();
                }
            }
            // Average load per core (load / (4 cores * EPOCH_TIME))
            avgLoad = static_cast<double>(totalLoad) / (4.0 * EPOCH_TIME);
        } else if (bigClusterActive) {
            // Calculate total load for big cluster
            unsigned long long totalLoad = 0;
            for (int i = 4; i < 8; i++) {
                if (!cores[i]->getLoadHistory().empty()) {
                    totalLoad += cores[i]->getLoadHistory().back();
                }
            }
            // Average load per core (load / (4 cores * EPOCH_TIME))
            avgLoad = static_cast<double>(totalLoad) / (4.0 * EPOCH_TIME);
        }
        
        // Decide if we need to switch clusters
        if (littleClusterActive && avgLoad >= 0.5) {
            // Switch from LITTLE to big cluster
            
            // Power on big cluster first
            for (int i = 4; i < 8; i++) {
                cores[i]->powerOnCore();
            }
            
            // Migrate processes from LITTLE to big
            for (int i = 0; i < 4; i++) {
                // Create a copy of the processes to migrate
                std::vector<Process*> toMigrate;
                for (Process* p : cores[i]->getScheduledProcesses()) {
                    toMigrate.push_back(p);
                }
                
                // Perform the migration
                for (Process* p : toMigrate) {
                    migrateProcess(p, cores[i], cores[i+4]);
                }
                
                // Power off the LITTLE core
                if (cores[i]->getState() == CoreState::POWER_ON)
                cores[i]->powerOffCore();
            }
            
            // Schedule newly arrived processes to big cores
            for (auto& entry : processes) {
                Process* process = entry.second;
                if (process->getState() == ProcessState::ARRIVED) {
                    unsigned coreId = 4 + (process->getId() % 4);
                    scheduleProcess(process, cores[coreId]);
                }
            }
        } 
        else if (bigClusterActive && avgLoad < 0.5) {
            // Switch from big to LITTLE cluster
            
            // Power on LITTLE cluster first
            for (int i = 0; i < 4; i++) {
                cores[i]->powerOnCore();
            }
            
            // First, collect all processes to migrate and deschedule them from big cores
            std::vector<std::pair<Process*, int>> processesToMigrate;
            for (int i = 4; i < 8; i++) {
                std::vector<Process*> toMigrate;
                for (Process* p : cores[i]->getScheduledProcesses()) {
                    toMigrate.push_back(p);
                }
                
                // Deschedule all processes from this core
                for (Process* p : toMigrate) {
                    cores[i]->descheduleProcess(p);
                    processesToMigrate.push_back({p, i-4});  // Store process and destination core
                }
            }
            
            // Power off the big cores
            for (int i = 4; i < 8; i++) {
                if (cores[i]->getState() == CoreState::POWER_ON)
                cores[i]->powerOffCore();
            }
            
            // Now migrate the processes to LITTLE cores
            for (auto& pair : processesToMigrate) {
                Process* p = pair.first;
                int destCore = pair.second;
                
                // Schedule the process to the LITTLE core
                cores[destCore]->scheduleProcess(p);
                p->setMigrationPenaltyTime();
                
                // If the process was running, set it to READY
                if (p->getState() == ProcessState::RUNNING) {
                    p->setState(ProcessState::READY);
                }
            }
            
            // Schedule newly arrived processes to LITTLE cores
            for (auto& entry : processes) {
                Process* process = entry.second;
                if (process->getState() == ProcessState::ARRIVED) {
                    unsigned coreId = process->getId() % 4;
                    scheduleProcess(process, cores[coreId]);
                }
            }
        }
        else {
            // No cluster switch needed, just schedule newly arrived processes
            int baseCore = littleClusterActive ? 0 : 4;
            for (auto& entry : processes) {
                Process* process = entry.second;
                if (process->getState() == ProcessState::ARRIVED) {
                    unsigned coreId = baseCore + (process->getId() % 4);
                    scheduleProcess(process, cores[coreId]);
                }
            }
        }
    } else if (schedulerScheme == SchedulerScheme::SWITCHER) {
        // Switcher scheduler implementation
        // TODO: Implement switcher scheduler
                // Initialize if first epoch
                if (currentTime == 0) {
                    // Start with LITTLE cores (0-3) active, big cores (4-7) off
                    for (int i = 0; i < 4; i++) {
                        cores[i]->powerOnCore();
                    }
                    for (int i = 4; i < 8; i++) {
                        if (cores[i]->getState() == CoreState::POWER_ON)
                        cores[i]->powerOffCore();
                    }
                    
                    // Schedule newly arrived processes to LITTLE cores in round-robin
                    for (auto& entry : processes) {
                        Process* process = entry.second;
                        if (process->getState() == ProcessState::ARRIVED) {
                            unsigned coreId = process->getId() % 4;
                            scheduleProcess(process, cores[coreId]);
                        }
                    }
                    return;
                }
                
                // For each virtual core (pair), decide whether to switch
                for (int i = 0; i < 4; i++) {
                    Core* littleCore = cores[i];
                    Core* bigCore = cores[i + 4];
                    
                    // Check which core in the pair is currently active
                    bool littleCoreActive = littleCore->getState() == CoreState::POWER_ON;
                    bool bigCoreActive = bigCore->getState() == CoreState::POWER_ON;
                    
                    // Calculate load for the active core in this pair
                    double load = 0.0;
                    if (littleCoreActive && !littleCore->getLoadHistory().empty()) {
                        load = static_cast<double>(littleCore->getLoadHistory().back()) / EPOCH_TIME;
                        
                        // If Little core load >= 50%, switch to big core
                        if (load >= 0.5) {
                            // Power on big core
                            bigCore->powerOnCore();
                            
                            // Migrate processes from Little to big
                            std::vector<Process*> toMigrate;
                            for (Process* p : littleCore->getScheduledProcesses()) {
                                toMigrate.push_back(p);
                            }
                            for (Process* p : toMigrate) {
                                migrateProcess(p, littleCore, bigCore);
                            }
                            
                            // Power off Little core
                            littleCore->powerOffCore();
                        }
                    }
                    else if (bigCoreActive && !bigCore->getLoadHistory().empty()) {
                        load = static_cast<double>(bigCore->getLoadHistory().back()) / EPOCH_TIME;
                        
                        // If big core load < 50%, switch to Little core
                        if (load < 0.5) {
                            // Power on Little core
                            littleCore->powerOnCore();
                            
                            // Migrate processes from big to Little
                            std::vector<Process*> toMigrate;
                            for (Process* p : bigCore->getScheduledProcesses()) {
                                toMigrate.push_back(p);
                            }
                            for (Process* p : toMigrate) {
                                migrateProcess(p, bigCore, littleCore);
                            }
                            
                            // Power off big core
                            bigCore->powerOffCore();
                        }
                    }
                }
                
                // Schedule newly arrived processes to appropriate cores
                for (auto& entry : processes) {
                    Process* process = entry.second;
                    if (process->getState() == ProcessState::ARRIVED) {
                        // Determine which virtual core to use based on round-robin
                        unsigned virtualCoreId = process->getId() % 4;
                        
                        // Check which physical core in this virtual core is active
                        Core* littleCore = cores[virtualCoreId];
                        Core* bigCore = cores[virtualCoreId + 4];
                        
                        if (littleCore->getState() == CoreState::POWER_ON) {
                            scheduleProcess(process, littleCore);
                        } else {
                            scheduleProcess(process, bigCore);
                        }
                    }
                }
        
    } else if (schedulerScheme == SchedulerScheme::GLOBAL_SCHEDULING) {
        // Global scheduling implementation
        // TODO: Implement global scheduling
                // Initialize if first epoch
                if (currentTime == 0) {
                    // Power on all cores
                    for (int i = 0; i < 8; i++) {
                        cores[i]->powerOnCore();
                    }
                    
                    // Schedule newly arrived processes to all cores in round-robin
                    for (auto& entry : processes) {
                        Process* process = entry.second;
                        if (process->getState() == ProcessState::ARRIVED) {
                            // Round-robin across all 8 cores
                            unsigned coreId = process->getId() % 8;
                            scheduleProcess(process, cores[coreId]);
                        }
                    }
                    return;
                }
                
                // No migration or power management in global scheduling
                // Just schedule newly arrived processes to cores in round-robin
                for (auto& entry : processes) {
                    Process* process = entry.second;
                    if (process->getState() == ProcessState::ARRIVED) {
                        // Round-robin across all 8 cores
                        unsigned coreId = process->getId() % 8;
                        scheduleProcess(process, cores[coreId]);
                    }
                }
        
    } else if (schedulerScheme == SchedulerScheme::BONUS) {
        // Initialize if first epoch
        if (currentTime == 0) {
            // Start with only 2 Little cores active to save energy
            cores[0]->powerOnCore();
            cores[1]->powerOnCore();
            for (int i = 2; i < 8; i++) {
                if (cores[i]->getState() == CoreState::POWER_ON) {
                    cores[i]->powerOffCore();
                }
            }
            
            // Schedule initial processes
            int coreIndex = 0;
            for (auto& entry : processes) {
                Process* process = entry.second;
                if (process->getState() == ProcessState::ARRIVED) {
                    scheduleProcess(process, cores[coreIndex]);
                    coreIndex = (coreIndex + 1) % 2; // Round-robin on active cores
                }
            }
            return;
        }
        
        // Analyze current workload
        int activeLittleCores = 0;
        int activeBigCores = 0;
        unsigned long long totalLoad = 0;
        int readyProcessCount = 0;
        int cpuBoundCount = 0;
        int ioBoundCount = 0;
        
        // Count active cores and calculate total load
        for (int i = 0; i < 4; i++) {
            if (cores[i]->getState() == CoreState::POWER_ON) {
                activeLittleCores++;
                if (!cores[i]->getLoadHistory().empty()) {
                    totalLoad += cores[i]->getLoadHistory().back();
                }
            }
        }
        for (int i = 4; i < 8; i++) {
            if (cores[i]->getState() == CoreState::POWER_ON) {
                activeBigCores++;
                if (!cores[i]->getLoadHistory().empty()) {
                    totalLoad += cores[i]->getLoadHistory().back();
                }
            }
        }
        
        // Analyze processes
        for (auto& entry : processes) {
            Process* process = entry.second;
            if (process->getState() == ProcessState::READY || 
                process->getState() == ProcessState::RUNNING) {
                readyProcessCount++;
                ProcessType type = classifyProcess(process);
                if (type == CPU_BOUND) cpuBoundCount++;
                else if (type == IO_BOUND) ioBoundCount++;
            }
        }
        
        // Calculate average load per active core
        int totalActiveCores = activeLittleCores + activeBigCores;
        double avgLoadPerCore = totalActiveCores > 0 ? 
            static_cast<double>(totalLoad) / (totalActiveCores * EPOCH_TIME) : 0.0;
        
        // Dynamic core allocation decisions
        
        // 1. Handle sporadic workload - power down if no load
        if (readyProcessCount == 0 && avgLoadPerCore < 0.1) {
            // Keep only one Little core active for future arrivals
            for (int i = 1; i < 8; i++) {
                if (cores[i]->getState() == CoreState::POWER_ON) {
                    if (cores[i]->getScheduledProcesses().empty()) {
                        cores[i]->powerOffCore();
                    }
                }
            }
            if (cores[0]->getState() == CoreState::POWER_OFF) {
                cores[0]->powerOnCore();
            }
        }
        // 2. Scale up if load is high
        else if (avgLoadPerCore > 0.8 || readyProcessCount > totalActiveCores * 2) {
            // Prefer big cores for CPU-bound workloads
            if (cpuBoundCount > ioBoundCount) {
                // Power on big cores first
                for (int i = 4; i < 8 && activeBigCores < 4; i++) {
                    if (cores[i]->getState() == CoreState::POWER_OFF) {
                        cores[i]->powerOnCore();
                        activeBigCores++;
                        break;
                    }
                }
            } else {
                // Power on Little cores first for IO-bound workloads
                for (int i = 0; i < 4 && activeLittleCores < 4; i++) {
                    if (cores[i]->getState() == CoreState::POWER_OFF) {
                        cores[i]->powerOnCore();
                        activeLittleCores++;
                        break;
                    }
                }
            }
        }
        // 3. Scale down if load is low
        else if (avgLoadPerCore < 0.3 && totalActiveCores > 1) {
            // Power off least loaded core
            Core* leastLoadedCore = nullptr;
            unsigned long long minLoad = ULONG_MAX;
            
            for (int i = 0; i < 8; i++) {
                if (cores[i]->getState() == CoreState::POWER_ON && 
                    cores[i]->getScheduledProcesses().empty()) {
                    if (!cores[i]->getLoadHistory().empty()) {
                        unsigned long long load = cores[i]->getLoadHistory().back();
                        if (load < minLoad) {
                            minLoad = load;
                            leastLoadedCore = cores[i];
                        }
                    }
                }
            }
            
            if (leastLoadedCore && totalActiveCores > 1) {
                leastLoadedCore->powerOffCore();
            }
        }
        
        // Schedule newly arrived processes
        for (auto& entry : processes) {
            Process* process = entry.second;
            if (process->getState() == ProcessState::ARRIVED) {
                ProcessType type = classifyProcess(process);
                Core* selectedCore = nullptr;
                unsigned long long minLoad = ULONG_MAX;
                
                // For CPU-bound processes, prefer big cores
                if (type == CPU_BOUND) {
                    for (int i = 4; i < 8; i++) {
                        if (cores[i]->getState() == CoreState::POWER_ON) {
                            unsigned long long load = 0;
                            if (!cores[i]->getLoadHistory().empty()) {
                                load = cores[i]->getLoadHistory().back();
                            }
                            if (load < minLoad) {
                                minLoad = load;
                                selectedCore = cores[i];
                            }
                        }
                    }
                }
                
                // If no big core available for CPU-bound, or for other types, use least loaded core
                if (!selectedCore) {
                    for (int i = 0; i < 8; i++) {
                        if (cores[i]->getState() == CoreState::POWER_ON) {
                            unsigned long long load = 0;
                            if (!cores[i]->getLoadHistory().empty()) {
                                load = cores[i]->getLoadHistory().back();
                            }
                            if (load < minLoad) {
                                minLoad = load;
                                selectedCore = cores[i];
                            }
                        }
                    }
                }
                
                if (selectedCore) {
                    scheduleProcess(process, selectedCore);
                }
            }
        }
        
        // Load balancing - migrate processes from overloaded to underloaded cores
        for (int i = 0; i < 8; i++) {
            if (cores[i]->getState() == CoreState::POWER_ON && 
                !cores[i]->getLoadHistory().empty()) {
                double coreLoad = static_cast<double>(cores[i]->getLoadHistory().back()) / EPOCH_TIME;
                
                // If core is overloaded, try to migrate some processes
                if (coreLoad > 0.9) {
                    auto processes = cores[i]->getScheduledProcesses();
                    for (Process* p : processes) {
                        if (p->getState() == ProcessState::READY) {
                            // Find least loaded core of appropriate type
                            Core* targetCore = nullptr;
                            unsigned long long minLoad = ULONG_MAX;
                            ProcessType type = classifyProcess(p);
                            
                            // For CPU-bound, prefer big cores
                            if (type == CPU_BOUND) {
                                for (int j = 4; j < 8; j++) {
                                    if (j != i && cores[j]->getState() == CoreState::POWER_ON) {
                                        unsigned long long load = 0;
                                        if (!cores[j]->getLoadHistory().empty()) {
                                            load = cores[j]->getLoadHistory().back();
                                        }
                                        if (load < minLoad && load < cores[i]->getLoadHistory().back() * 0.7) {
                                            minLoad = load;
                                            targetCore = cores[j];
                                        }
                                    }
                                }
                            }
                            
                            // If no suitable target found, try any less loaded core
                            if (!targetCore) {
                                for (int j = 0; j < 8; j++) {
                                    if (j != i && cores[j]->getState() == CoreState::POWER_ON) {
                                        unsigned long long load = 0;
                                        if (!cores[j]->getLoadHistory().empty()) {
                                            load = cores[j]->getLoadHistory().back();
                                        }
                                        if (load < minLoad && load < cores[i]->getLoadHistory().back() * 0.7) {
                                            minLoad = load;
                                            targetCore = cores[j];
                                        }
                                    }
                                }
                            }
                            
                            if (targetCore) {
                                migrateProcess(p, cores[i], targetCore);
                                break; // Migrate only one process per epoch to avoid thrashing
                            }
                        }
                    }
                }
            }
        }
    }
}