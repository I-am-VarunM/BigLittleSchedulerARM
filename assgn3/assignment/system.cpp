#include "simulator.hpp"

System::System(SchedulerScheme scheme) {
    // Read the input data and initialize the cores and processes
    // 1. Core information
    // <num_cores>
    // <MIPS> <idle_power> <running_power>
    unsigned numCores, coreId = 0, currentCoreId, MIPS, idlePower, runningPower;
    cin >> numCores;
	while (coreId < numCores) {
        currentCoreId = coreId++;
		cin >> MIPS >> idlePower >> runningPower;
        cores[currentCoreId] = new Core(currentCoreId, MIPS, idlePower, runningPower);
    }

    // 2. Process information
    //<num_processes>
    //<arrival_time> <num_chunks> [<cpu_instructions_in_millions> <io_wait_time>]+
    unsigned num_processes, pid = 0, currentPid, arrivalTime, numChunks, instCount, ioWaitTime;
    cin >> num_processes;

	while (pid < num_processes) {
        currentPid = pid++;
		cin >> arrivalTime >> numChunks;
        std::vector<std::pair<BurstType, unsigned>> bursts;
        for (unsigned i = 0; i < numChunks; i+=2) {
            cin >> instCount >> ioWaitTime;
            bursts.push_back(make_pair(BurstType::CPU, instCount));
            bursts.push_back(make_pair(BurstType::IO, ioWaitTime));
        }
        processes[currentPid] = new Process(currentPid, arrivalTime, bursts);
	}
    if (scheme > SchedulerScheme::BONUS) {
        cout << "Incorrect scheduler scheme specified\n";
        exit(0);
    }
    schedulerScheme = scheme;
    cout << "[System] Initialized system with";
    switch (scheme) {
        case CLUSTERING:
            cout << " Clustering ";
            break;
        case SWITCHER:
            cout << " Switcher ";
            break;
        case GLOBAL_SCHEDULING:
            cout << " Global scheduling ";
            break;
        case BONUS:
            cout << " Bonus ";
            break;
    }
    cout << "scheduler scheme\n";
}

void System::scheduleProcess(Process *process, Core *core) {
    if (process->getState() != ProcessState::ARRIVED)
        cout << "[WARN] Trying to schedule a process (" << process->getId() 
                << ") that is not in ARRIVED state\n";
    process->setState(ProcessState::READY);
    core->scheduleProcess(process);
}

void System::migrateProcess(Process *process, Core *srcCore, Core *dstCore) {
    srcCore->descheduleProcess(process);
    dstCore->scheduleProcess(process);
    process->setMigrationPenaltyTime();
    if (process->getState() == ProcessState::RUNNING)
        process->setState(ProcessState::READY);
    // Else if it was doing IO, let it do IO
}

void System::run() {
    cout << "[System] Simulation starting\n";
    while (true) {
        // If all processes have terminated, shut down the system
        bool allProcessesTerminated = true;
        for (auto entry: processes) {
            if (entry.second->getState() != TERMINATED) {
                allProcessesTerminated = false;
                break;
            }
        }
        if (allProcessesTerminated) break;
        
        unsigned colWidth = 12;
        // Print the current state of the cores to help visualize
        if (currentTime % EPOCH_TIME == 1) {
            cout << std::setw(colWidth) << "Epoch " + std::to_string((currentTime / EPOCH_TIME) + 1);
            for (unsigned i=0; i<cores.size(); i++)
                cout << std::setw(colWidth) << "Core " + std::to_string(i);
            cout << "\n";
        }
        if (currentTime > 0) {
            cout << std::setw(colWidth) << currentTime;
            for (unsigned i=0; i<cores.size(); i++) {
                if (cores[i]->getState() == POWER_OFF) {
                    cout << std::setw(colWidth) << "O"; // O for Off
                } else {
                    Process* currentProcess = cores[i]->getCurrentProcess();
                    if (currentProcess)
                        cout << std::setw(colWidth) << "Pid " + std::to_string(cores[i]->getCurrentProcess()->getId());
                    else
                        cout << std::setw(colWidth) << "I"; // I for Idle
                }
            }
            cout << "\n";
        }

        // Run cycle functions for all cores and processes
        for (auto entry: processes)
            entry.second->cycle(currentTime);
        for (auto entry: cores)
            entry.second->cycle(currentTime);

        if (currentTime % EPOCH_TIME == 0) {
            epoch(currentTime);
            currentEpoch++;
        }
        currentTime++;
    }
    cout << "[System] Simulation completed at " << currentTime << " ms\n";
}

void System::printStats() {
    cout << "[System] Total Time   : " << currentTime << "\n";
    cout << "[System] Total Epochs : " << currentEpoch - 1 << "\n";
    unsigned long long totalEnergyConsumed = 0;
    for (auto entry: cores) {
        entry.second->printStats();
        totalEnergyConsumed += entry.second->getTotalEnergyConsumed();
    }
    unsigned long long totalTurnAroundTime = 0;
    for (auto entry: processes) {
        entry.second->printStats();
        totalTurnAroundTime += entry.second->getTurnAroundTime();
    }
    cout << "[System] Avg Turn Around Time : " << totalTurnAroundTime / processes.size() << " ms\n";
    cout << "[System] Energy Delay Product : " << totalEnergyConsumed * currentTime << "\n";
}
