#include "DungeonManager.h"
#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <algorithm>

using namespace std;

DungeonManager::DungeonManager(uint32_t numInstances, uint32_t t1, uint32_t t2) {
    for (uint32_t i = 0; i < numInstances; ++i) {
        auto instance = make_unique<DungeonInstance>(t1, t2, i + 1, partyQueue, queueMutex, queueCv, *this);
        instance->run();
        dungeonInstances.push_back(move(instance));
    }
}

DungeonManager::~DungeonManager() {
    for (auto& instance : dungeonInstances) {
        instance->stopInstance();
    }
    // printAllDungeonStats();
}

void DungeonManager::enqueueParty(const Party& party) {
    lock_guard<mutex> lock(queueMutex);
    partyQueue.push(party);
    queueCv.notify_one();
}

void DungeonManager::waitForCompletion() {
    while (true) {
        this_thread::sleep_for(chrono::milliseconds(1000));
        {
            unique_lock<mutex> lock(queueMutex);
            bool allInstancesIdle = all_of(dungeonInstances.begin(), dungeonInstances.end(), [](const auto& instance) {
                return !instance->isBusy();
            });
            if (partyQueue.empty() && allInstancesIdle) {
                break;
            }
        }
    }
    
    for (auto& instance : dungeonInstances) {
        instance->stopInstance();
    }
    printAllDungeonStats();
}

void DungeonManager::printAllDungeonStats() {
    cout << endl;
    for (auto& instance : dungeonInstances) {
        cout << instance->getInstanceStats() << endl;
    }
}

void DungeonManager::printAllDungeonCurrentStatus() {
    cout << "----------------------------------------" << endl;
    cout << "Dungeon Status Summary:" << endl;
    for (auto& instance : dungeonInstances) {
        cout << instance->getCurrentStatus() << endl;
    }
    cout << "----------------------------------------" << endl;
}