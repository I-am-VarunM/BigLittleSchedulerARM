#pragma once
#include <map>
#include <list>
#include <vector>
#include <iomanip>
#include <iostream>
using namespace std;

/*****************************************************************************
 *                           Enums used throughout                           *
 *****************************************************************************/

typedef enum {
    NOTARRIVED,     // Process scheduled to arrive at a future time
    ARRIVED,        // Process arrived, but is not yet scheduled to a core
    READY,          // Process scheduled to a core, ready to execute
    RUNNING,        // Process running on a core
    WAITING_FOR_IO, // Process waiting for IO (not using CPU cycles)
    TERMINATED      // Process completed
} ProcessState;

typedef enum {
    POWER_ON,
    POWER_OFF
} CoreState;

typedef enum {
    CPU,
    IO
} BurstType;

typedef enum {
    CLUSTERING,
    SWITCHER,
    GLOBAL_SCHEDULING,
    BONUS
} SchedulerScheme;

/*****************************************************************************
 *                           Simulation constants                            *
 *****************************************************************************/

const unsigned EPOCH_TIME = 100;
const unsigned MIGRATION_PENALTY = 2;

/*****************************************************************************
 *                              Classes                                      *
 *****************************************************************************/

class Core;

class Process {
        unsigned id;
        unsigned arrivalTime, completionTime;
        ProcessState state;
        std::vector<std::pair<BurstType, unsigned>> bursts;
        unsigned currentBurstIndex;
        unsigned long long totalEnergyConsumed, remainingCPUBurstInstructions, 
                      remainingIOBurstTime, remainingMigrationPenaltyTime;

	public:
		Process(unsigned id, unsigned arrivalTime, std::vector<std::pair<BurstType, unsigned>> bursts) : 
            id{id}, arrivalTime{arrivalTime}, bursts{bursts}, state{ProcessState::NOTARRIVED},
            currentBurstIndex{0}, totalEnergyConsumed{0}, remainingCPUBurstInstructions {bursts[0].second},
            remainingMigrationPenaltyTime {0}, remainingIOBurstTime {0} {}

        // Getters
        unsigned getId()            { return id; }
        unsigned getArrivalTime()   { return arrivalTime; }
        ProcessState getState()     { return state; }
        std::vector<std::pair<BurstType, unsigned>> getPastBursts() {
            std::vector<std::pair<BurstType, unsigned>> pastBursts;
            for (unsigned i = 0; i < currentBurstIndex; i++)
                pastBursts.push_back(bursts[i]);
            return pastBursts;
        }
        unsigned long long getTurnAroundTime() { return completionTime - arrivalTime; }
        void setState(ProcessState newState)    { state = newState; }
        void setMigrationPenaltyTime()          { remainingMigrationPenaltyTime = MIGRATION_PENALTY; }

        // Executed every clock
        void cycle(unsigned long currentTime);

        // Called when the CPU executes this process for one time unit
        void updateCpuBurst(unsigned long currentTime, unsigned coreId,
                unsigned long long instructionsExecuted,
                unsigned long long energyConsumed);

        // Prints process stats
        void printStats();
};

class Core {
        const unsigned id, MIPS, idlePower, runningPower;
        CoreState state;
        Process *currentProcess;
        std::vector<Process*> scheduledProcesses;
        std::vector<unsigned long long> loadHistory;        // Stores average Load values from past epochs
        std::vector<unsigned long long> energyHistory;      // Stores average energy values from past epochs
        unsigned long long currentEpochLoad, currentEpochEnergy;
        unsigned long long totalTimePoweredOn, totalTimePoweredOnAndIdle;

	public:
		Core(unsigned id, unsigned MIPS, unsigned idlePower, unsigned runningPower) : 
            id{id}, MIPS{MIPS}, idlePower{idlePower}, runningPower{runningPower}, 
            currentEpochEnergy{0}, currentEpochLoad{0}, state {CoreState::POWER_OFF},
            totalTimePoweredOn{0}, totalTimePoweredOnAndIdle{0} {}

        // Getter functions
        unsigned getId()            { return id; }
        unsigned getMIPS()          { return MIPS; }
        unsigned getIdlePower()     { return idlePower; }
        unsigned getRunningPower()  { return runningPower; }
        CoreState getState()        { return state; }
        Process* getCurrentProcess()                                { return currentProcess; }
        std::vector<Process*> getScheduledProcesses()               { return scheduledProcesses; }
        std::vector<unsigned long long> const& getLoadHistory()     { return loadHistory; }
        std::vector<unsigned long long> const& getEnergyHistory()   { return energyHistory; }
        unsigned long long getTotalEnergyConsumed() { 
            unsigned long long totalEnergyConsumed = currentEpochEnergy;
            for (auto energy : energyHistory)
                totalEnergyConsumed += energy;
            return totalEnergyConsumed;
        }

        // Schedule and Deschedule a process on a Core
        void scheduleProcess(Process *process);
        void descheduleProcess(Process *process);

        // To turn the core on or off
        void powerOnCore();
        void powerOffCore();

        // Get a ready process scheduled on this core, to be executed next
        Process* getNextReadyProcess();

        // Executed every clock
        void cycle(unsigned long currentTime);

        // Prints core stats
        void printStats();
};

class System {
    unsigned long currentTime = 0;
    unsigned long currentEpoch = 0;
    SchedulerScheme schedulerScheme;
    std::map<unsigned, Process*> processes;
    std::map<unsigned, Core*> cores;

	public:
		System(SchedulerScheme scheme);

        // Schedule a process on a core
        void scheduleProcess(Process *p, Core *core);

        // Migrate process from one core to another
        void migrateProcess(Process *p, Core *srcCore, Core *dstCore);

        // Executed every epoch
        void epoch(unsigned long currentTime);

        // Prints scheduler stats
        void printStats();

        // Execute the system simulation
        void run();
};
