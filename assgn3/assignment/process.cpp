#include "simulator.hpp"

void ERROR(std::string errorMessage) {
    cout << "[ERROR] " << errorMessage << "\n";
    exit(0);
}

void Process::cycle(unsigned long currentTime) {
    if (state == ProcessState::NOTARRIVED) {
        if (currentTime == arrivalTime) {
            //cout << "[Process " << id << "] Arrived\n";
            state = ProcessState::ARRIVED;
        }
    } else if (state == ProcessState::WAITING_FOR_IO) {
        // We don't count energy consumed due to IO
        //cout << "[Process " << id << "] Executed IO Burst for one unit\n";
        remainingIOBurstTime--;
        if (remainingIOBurstTime == 0) {
            // Current IO burst is completed
            currentBurstIndex++;
            if (currentBurstIndex >= bursts.size()) {
                //cout << "[Process " << id << "] Finished Execution\n";
                state = ProcessState::TERMINATED;
                completionTime = currentTime;
            } else {
                //cout << "[Process " << id << "] IO Burst completed\n";
                state = ProcessState::READY;
                if (remainingCPUBurstInstructions != 0)
                    ERROR("remainingCPUBurstInstructions non zero at end of IO burst");
                remainingCPUBurstInstructions = bursts[currentBurstIndex].second;
            }
        }
    }
}

void Process::updateCpuBurst(unsigned long currentTime, unsigned coreId,
        unsigned long long instructionsExecuted,
        unsigned long long energyConsumed) {
    // Called when the CPU executes this process for one time unit
    if (state == ProcessState::READY) {
        // We started execution of this process from ARRIVED or WAITING_FOR_IO state
        // So update the remainingCPUBurstInstructions and then proceed
        remainingCPUBurstInstructions = bursts[currentBurstIndex].second;
        state = ProcessState::RUNNING;
    }
    if (state == ProcessState::RUNNING) {
        totalEnergyConsumed += energyConsumed;
        if (remainingMigrationPenaltyTime > 0) {
            // The process just migrated and is incurring the migration penalty
            //cout << "[Process " << id << "] Incurred migration penalty on Core " 
            //    << coreId << " for one unit\n";
            remainingMigrationPenaltyTime--;
        } else {
            //cout << "[Process " << id << "] Executed CPU Burst on Core " 
            //    << coreId << " for one unit\n";
            unsigned long long executedInstructionCount = instructionsExecuted;
            if (executedInstructionCount > remainingCPUBurstInstructions)
                remainingCPUBurstInstructions = 0;
            else remainingCPUBurstInstructions -= executedInstructionCount;
            if (remainingCPUBurstInstructions == 0) {
                // Current CPU burst is completed
                currentBurstIndex++;
                if (currentBurstIndex >= bursts.size()) {
                    //cout << "[Process " << id << "] Finished Execution\n";
                    state = ProcessState::TERMINATED;
                    completionTime = currentTime;
                } else {
                    //cout << "[Process " << id << "] CPU Burst completed\n";
                    state = ProcessState::WAITING_FOR_IO;
                    if (remainingIOBurstTime != 0)
                        ERROR("remainingIOBurstTime non zero at end of CPU burst");
                    remainingIOBurstTime = bursts[currentBurstIndex].second;
                }
            }
        }
    } 
}

void Process::printStats() {
    cout << "[Process " << id << "] Turn around time            : " << std::setw(10) << getTurnAroundTime() << " ms\n";
}
