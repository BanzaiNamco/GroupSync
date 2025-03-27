#include "DungeonManager.h"
#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <algorithm>
#include <memory>

using namespace std;

DungeonManager::DungeonManager(uint32_t numInstances, uint32_t t1, uint32_t t2) {
    for (uint32_t i = 0; i < numInstances; ++i) {
        auto instance = make_unique<DungeonInstance>(t1, t2, i + 1, loggerMutex, loggerCv);
        instance->run();
        dungeonInstances.push_back(move(instance));
    }

    // run logger thread
    loggerThread = thread([this]() {
        while (!stop) {
            unique_lock<mutex> lock(loggerMutex);
            loggerCv.wait(lock);
            if (!stop)
                printAllDungeonCurrentStatus();
        }
    });
}

void DungeonManager::run() {
    while (!partyQueue.empty()) {
        unique_ptr<Party> party;
        party = move(partyQueue.front());
        partyQueue.pop();

        bool assigned = false;
        uint32_t instances = static_cast<uint32_t>(dungeonInstances.size());
        uint32_t lastUsedIndex = 0;
        while (!assigned) {
            for (uint32_t i = 0; i < instances; ++i) {
                uint32_t index = (lastUsedIndex + i) % instances;
                if (!dungeonInstances[index]->isBusy()) {
                    dungeonInstances[index]->assignParty(std::move(party));
                    assigned = true;
                    lastUsedIndex = index + 1;
                    break;
                }
            }
            if (!assigned) {
                this_thread::sleep_for(chrono::milliseconds(100));
            }
        }
    }
}

DungeonManager::~DungeonManager() {
    stop = true;
    for (auto& instance : dungeonInstances) {
        instance->stopInstance();
    }
    loggerCv.notify_all();
    loggerThread.join();
}

void DungeonManager::enqueueParty(Party party) {
    partyQueue.push(std::make_unique<Party>(party));
}

void DungeonManager::waitForCompletion() {
    while (true) {
        this_thread::sleep_for(chrono::milliseconds(1000));

        bool allInstancesIdle = all_of(dungeonInstances.begin(), dungeonInstances.end(), [](const auto& instance) {
            return !instance->isBusy();
        });

        if (partyQueue.empty() && allInstancesIdle) {
            break;
        }
    }

    for (auto& instance : dungeonInstances) {
        instance->stopInstance();
    }

    stop = true;
    loggerCv.notify_all();
    printAllDungeonStats();
}

void DungeonManager::printAllDungeonStats() {
    cout << endl;
    uint32_t totalPartiesServed = 0;
    uint32_t totalTimeServiced = 0;
    for (auto& instance : dungeonInstances) {
        cout << instance->getInstanceStats() << endl;
        totalPartiesServed += instance->getTotalPartiesServed();
        totalTimeServiced += instance->getTotalTimeServiced();
    }
    cout << "Total parties served: " << totalPartiesServed << endl;
    cout << "Total service time of all dungeons added up: " << totalTimeServiced / 1000.00 << "s" << endl;
}

void DungeonManager::printAllDungeonCurrentStatus() {
    cout << "----------------------------------------" << endl;
    cout << "Dungeon Status Summary:" << endl;
    for (auto& instance : dungeonInstances) {
        cout << instance->getCurrentStatus() << endl;
    }
    cout << "----------------------------------------" << endl;
}