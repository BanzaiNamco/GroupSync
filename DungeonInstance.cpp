#include "DungeonManager.h"

#include <iostream>
#include <random>
#include <chrono>
#include <thread>
#include <sstream>
#include <iomanip>

using namespace std;

DungeonInstance::DungeonInstance(uint32_t time1, uint32_t time2, uint32_t instanceID, mutex& logMutex, 
                                 condition_variable& logCv)
    : t1(time1), t2(time2), instanceId(instanceID), 
      loggerMutex(logMutex), loggerCv(logCv), currentParty(nullptr) {}

void DungeonInstance::run() {
    worker = thread([this]() {
        while (true) {
            unique_ptr<Party> partyToProcess;

            {
                unique_lock<mutex> lock(mtx);
                cv.wait(lock, [this]() { return currentParty != nullptr || stop; });

                if (stop && currentParty == nullptr) {
                    break;
                }

                partyToProcess = move(currentParty);
                currentParty = nullptr;
            }

            if (partyToProcess) {
                processParty(*partyToProcess);
                busy = false;
            }
        }
    });
}

void DungeonInstance::processParty(const Party& party) {
    mt19937 generator(random_device{}());
    uniform_int_distribution<uint32_t> distribution(t1, t2);
    uint32_t completionTime = distribution(generator);
    {
        lock_guard<mutex> lock(loggerMutex);
        cout << "Dungeon " << instanceId << " is now ACTIVE serving Party " << party.partyId << " for " << completionTime / 1000.00 << "s." << endl;
    }
    loggerCv.notify_all();

    this_thread::sleep_for(chrono::milliseconds(completionTime));

    {
        lock_guard<mutex> lock(loggerMutex);
        cout << "Dungeon " << instanceId << " is now EMPTY after completing Party " << party.partyId << "." << endl;
    }
    loggerCv.notify_all();
    partiesServed++;
    totalTimeServiced += completionTime;
}

void DungeonInstance::assignParty(unique_ptr<Party> party) {
    {
        lock_guard<mutex> lock(mtx);
        currentParty = move(party);
        busy = true;
    }
    cv.notify_one();
}

void DungeonInstance::stopInstance() {
    stop = true;
    cv.notify_all();
    if (worker.joinable()) {
        worker.join();
    }
}

string DungeonInstance::getInstanceStats() {
    stringstream stream;
    stream << fixed << setprecision(3) << (totalTimeServiced / 1000.00);
    return "Dungeon " + to_string(instanceId) + " served " + to_string(partiesServed) + 
           " parties with a total service time of " + stream.str() + "s.";
}

uint32_t DungeonInstance::getInstanceId() const {
    return instanceId;
}

bool DungeonInstance::isBusy() {
    return busy.load();
}

string DungeonInstance::getCurrentStatus() {
    return "Dungeon " + to_string(instanceId) + " is " + (busy.load() ? "ACTIVE" : "EMPTY") + ".";
}

uint32_t DungeonInstance::getTotalTimeServiced() {
    return totalTimeServiced;
}

uint32_t DungeonInstance::getTotalPartiesServed() {
    return partiesServed;
}