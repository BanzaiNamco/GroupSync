#ifndef DUNGEONINSTANCE_H
#define DUNGEONINSTANCE_H

#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <string>
#include "Party.h" 

class DungeonManager;

class DungeonInstance {
public:
    DungeonInstance(uint32_t time1, uint32_t time2, uint32_t instanceID, 
                    std::queue<Party>& sharedQueue, std::mutex& sharedMutex, 
                    std::condition_variable& sharedCv, DungeonManager& managerRef);
    void run();
    void stopInstance();
    std::string getInstanceStats();
    uint32_t getInstanceId() const;
    bool isBusy() const;
    std::string getCurrentStatus();

private:
    void processParty(const Party& party);
    uint32_t t1, t2;
    uint32_t instanceId;
    std::queue<Party>& partyQueue;
    std::mutex& queueMutex;
    std::condition_variable& queueCv;
    std::thread worker;
    uint32_t partiesServed = 0;
    uint32_t totalTimeServiced = 0;
    bool stop;
    bool busy = false;
    DungeonManager& manager;
};

#endif // DUNGEONINSTANCE_H