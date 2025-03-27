#ifndef DUNGEONINSTANCE_H
#define DUNGEONINSTANCE_H

#include <queue>
#include <thread>
#include <mutex>
#include <atomic>
#include <condition_variable>
#include <string>
#include "Party.h" 

class DungeonManager;

class DungeonInstance {
public:
    DungeonInstance(uint32_t time1, uint32_t time2, uint32_t instanceID, 
                    std::mutex& logMutex, 
                    std::condition_variable& logCv);
    void run();
    void stopInstance();
    std::string getInstanceStats();
    uint32_t getInstanceId() const;
    bool isBusy();
    std::string getCurrentStatus();
    void assignParty(std::unique_ptr<Party> party);
    uint32_t getTotalTimeServiced();
    uint32_t getTotalPartiesServed();

private:
    void processParty(const Party &party);
    uint32_t t1, t2;
    uint32_t instanceId;
    std::mutex& loggerMutex;
    std::condition_variable& loggerCv;
    std::mutex mtx;
    std::condition_variable cv;
    std::thread worker;
    uint32_t partiesServed = 0;
    uint32_t totalTimeServiced = 0;
    std::atomic<bool> stop = false;
    std::atomic<bool> busy = false;
    std::unique_ptr<Party> currentParty = nullptr;
};

#endif