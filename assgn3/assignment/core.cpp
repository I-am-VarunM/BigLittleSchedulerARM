#include "simulator.hpp"
#include <algorithm>

void Core::scheduleProcess(Process *processToSchedule) {
    if (state == POWER_OFF)
        cout << "[WARN] scheduling process " << processToSchedule->getId() << " on a powered off core " << id << "\n";
    //cout << "[Core " << id << "] Process " << processToSchedule->getId() << " scheduled on core " << id << "\n";
    for (Process *process: scheduledProcesses) {
        if (process == processToSchedule) {
            cout << "[WARN] Trying to schedule a process (" << process->getId() 
                << ") that was already scheduled on core (" << id << ")\n";
            return;
        }
    }
    scheduledProcesses.push_back(processToSchedule);
}

void Core::descheduleProcess(Process *processToDeschedule) {
    auto iter = std::find(scheduledProcesses.begin(), scheduledProcesses.end(), processToDeschedule);
    if (iter == scheduledProcesses.end()) {
        cout << "[WARN] Trying to deschedule a process (" << processToDeschedule->getId() 
            << ") that was not scheduled on core (" << id << ")\n";
    } else scheduledProcesses.erase(iter);
    if (processToDeschedule == currentProcess)
     currentProcess = getNextReadyProcess();
}

void Core::powerOnCore() {
    if (state == CoreState::POWER_ON)
        cout << "[WARN] Trying to power on an already powered on core (" << id << ")\n";
    state = CoreState::POWER_ON;
}

void Core::powerOffCore() {
    if (state == CoreState::POWER_OFF)
        cout << "[WARN] Trying to power off an already powered off core (" << id << ")\n";
    state = CoreState::POWER_OFF;
}

Process* Core::getNextReadyProcess() {
    // Returns NULL when there are no ready processes
    // When a new burst arrives, schedule it with FCFS (break ties prioritizing lower PID)
    Process* selectedProcess = nullptr;
    unsigned selectedPid = -1;
    for (Process* process : scheduledProcesses) {
        if (process->getState() == ProcessState::READY) {
            if ((!selectedProcess) || (process->getId() < selectedPid)) {
                selectedProcess = process;
                selectedPid = process->getId();
            }
        }
    }
    if (selectedProcess) selectedProcess->setState(ProcessState::RUNNING);
    return selectedProcess;
}

void Core::cycle(unsigned long currentTime) {
    if (state == CoreState::POWER_ON) {
        totalTimePoweredOn++;
        if (!currentProcess) {
            // There is no process burst scheduled currently
            totalTimePoweredOnAndIdle++;
            currentEpochEnergy += idlePower;        // Add Idle power
            currentProcess = getNextReadyProcess();
        } else {
            // Core is executing a process burst currently
            currentEpochEnergy += runningPower;     // Add running power
            currentEpochLoad   += 1;                // Add load
            // Update CPU burst of currently running process 
            // with instruction count and energy values (also updating process state)
            currentProcess->updateCpuBurst(currentTime, id,
                    this->getMIPS() / 1000, this->getRunningPower());
            if (currentProcess->getState() != ProcessState::RUNNING) {
                // Process terminated or paused for I/O,
                // schedule burst from another process if available
                currentProcess = getNextReadyProcess();
            }
        }
    }

    if (currentTime % EPOCH_TIME == 0) {
        loadHistory.push_back(currentEpochLoad);
        energyHistory.push_back(currentEpochEnergy);
        currentEpochLoad = 0;
        currentEpochEnergy = 0;
    }
}

void Core::printStats() {
    cout << "[Core " << id << "] Total time powered on          : " << std::setw(10) << totalTimePoweredOn           << " ms\n";
    cout << "[Core " << id << "] Total time powered on and idle : " << std::setw(10) << totalTimePoweredOnAndIdle    << " ms\n";
    cout << "[Core " << id << "] Total energy consumed          : " << std::setw(10) << getTotalEnergyConsumed() << " mJ \n";
}
