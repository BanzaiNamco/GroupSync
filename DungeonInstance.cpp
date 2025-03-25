#include "DungeonInstance.h"
#include "DungeonManager.h" // Include the full definition

#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <random>
#include <chrono>

using namespace std;

DungeonInstance::DungeonInstance(uint32_t time1, uint32_t time2, uint32_t instanceID, 
                                 std::queue<Party>& sharedQueue, std::mutex& sharedMutex, 
                                 std::condition_variable& sharedCv, DungeonManager& managerRef)
    : t1(time1), t2(time2), instanceId(instanceID), 
      partyQueue(sharedQueue), queueMutex(sharedMutex), queueCv(sharedCv), 
      stop(false), manager(managerRef) {}

void DungeonInstance::run() {
    cout << "Dungeon " << instanceId << " is now Ready and EMPTY." << endl;
    worker = thread([this]() {
        while (!stop) {
            unique_lock<mutex> lock(queueMutex);
            queueCv.wait(lock, [this]() { return !partyQueue.empty() || stop; });

            if (stop && partyQueue.empty()) return;

            Party party = partyQueue.front();
            partyQueue.pop();
            lock.unlock();

            processParty(party);
        }
    });
}

void DungeonInstance::processParty(const Party& party) {
    mt19937 generator(random_device{}());
    uniform_int_distribution<uint32_t> distribution(t1, t2);
    uint32_t completionTime = distribution(generator);
    
    {
        unique_lock<mutex> lock(queueMutex);
        cout << "Dungeon " << instanceId << " is now ACTIVE serving Party " << party.partyId << " for " << completionTime / 1000.00 << "s." << endl;
        busy = true;
        manager.printAllDungeonCurrentStatus();
    }
    
    this_thread::sleep_for(chrono::milliseconds(completionTime));
    
    {
        unique_lock<mutex> lock(queueMutex);
        cout << "Dungeon " << instanceId << " is now EMPTY after completing Party " << party.partyId << "." << endl;
        busy = false;
        manager.printAllDungeonCurrentStatus();
    }
    
    partiesServed++;
    totalTimeServiced += completionTime;
}

void DungeonInstance::stopInstance() {
    stop = true;
    queueCv.notify_all();
    if (worker.joinable()) worker.join();
}

string DungeonInstance::getInstanceStats() {
    return "Dungeon " + to_string(instanceId) + " served " + to_string(partiesServed) + 
           " parties with a total service time of " + to_string(totalTimeServiced / 1000.00) + "s.";
}

uint32_t DungeonInstance::getInstanceId() const {
    return instanceId;
}

bool DungeonInstance::isBusy() const {
    return busy;
}

string DungeonInstance::getCurrentStatus() {
    return "Dungeon " + to_string(instanceId) + " is " + (isBusy() ? "ACTIVE" : "EMPTY") + ".";
}