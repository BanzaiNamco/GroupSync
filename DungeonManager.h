#ifndef DUNGEONMANAGER_H
#define DUNGEONMANAGER_H

#include <queue>
#include <vector>
#include <memory>
#include <mutex>
#include <thread>
#include <condition_variable>
#include <atomic>
#include "DungeonInstance.h"
#include "Party.h"

class DungeonManager {
public:
    DungeonManager(uint32_t numInstances, uint32_t t1, uint32_t t2);
    ~DungeonManager();
    void enqueueParty(Party party);
    void printAllDungeonStats();
    void waitForCompletion();
    void printAllDungeonCurrentStatus();
    void run();
    
    private:
    std::mutex queueMutex;
    std::condition_variable queueCv;
    std::queue<std::unique_ptr<Party>> partyQueue;
    std::vector<std::unique_ptr<DungeonInstance>> dungeonInstances;
    std::thread loggerThread;
    std::mutex loggerMutex;
    std::condition_variable loggerCv;
    std::atomic<bool> stop = false;
};

#endif