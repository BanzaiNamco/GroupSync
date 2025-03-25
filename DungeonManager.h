#ifndef DUNGEONMANAGER_H
#define DUNGEONMANAGER_H

#include <queue>
#include <vector>
#include <memory>
#include <mutex>
#include <condition_variable>
#include "DungeonInstance.h"
#include "Party.h"

class DungeonManager {
public:
    DungeonManager(uint32_t numInstances, uint32_t t1, uint32_t t2);
    ~DungeonManager();
    void enqueueParty(const Party& party);
    void printAllDungeonStats();
    void waitForCompletion();
    void printAllDungeonCurrentStatus();

private:
    std::queue<Party> partyQueue;
    std::mutex queueMutex;
    std::condition_variable queueCv;
    std::vector<std::unique_ptr<DungeonInstance>> dungeonInstances;
};

#endif