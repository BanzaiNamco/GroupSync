#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <queue>
#include <vector>
#include <string>
#include <fstream>
#include <unordered_map>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <chrono>
#include <functional>
#include <algorithm>
#include <cctype>
#include <stdexcept>
#include <cmath>
#include <random>

using namespace std;

struct Party {
    uint32_t partyId;
    Party(uint32_t id) : partyId(id) {}
};

class DungeonInstance {
    public:
        DungeonInstance(uint32_t time1, uint32_t time2, uint32_t instanceID, queue<Party>& sharedQueue, mutex& sharedMutex, condition_variable& sharedCv)
            : t1(time1), t2(time2), instanceId(instanceID),
            partyQueue(sharedQueue), queueMutex(sharedMutex), queueCv(sharedCv), stop(false) {}

        void run() {
            worker = thread([this]() {
                {
                    unique_lock<mutex> lock(queueMutex);
                    cout << "Dungeon " << instanceId << " is now Ready." << endl;
                }
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

        void processParty(const Party& party) {
            mt19937 generator(random_device{}());
            uniform_int_distribution<uint32_t> distribution(t1, t2);
            uint32_t completionTime = distribution(generator);
            {
                unique_lock<mutex> lock(queueMutex);
                cout << "Dungeon " << instanceId << " is now Busy processing Party " << party.partyId << " with a completion time length:" << completionTime << "." << endl;
            }
            this_thread::sleep_for(chrono::milliseconds(completionTime));
            
            {
                unique_lock<mutex> lock(queueMutex);
                cout << "Dungeon " << instanceId << " is now Available after completing Party " << party.partyId << "." << endl;
            }
            partiesServed++;
            totalTimeServiced += completionTime;
        }

        void stopInstance() {
            stop = true;
            queueCv.notify_all();
            if (worker.joinable()) worker.join();
        }

        string getInstanceStats() {
            return "Dungeon " + to_string(instanceId) + " served " + to_string(partiesServed) + " parties with a total service time of " + to_string(totalTimeServiced) + " ms.";
        }

    private:
        uint32_t t1, t2;
        uint32_t instanceId;
        queue<Party>& partyQueue;
        mutex& queueMutex;
        condition_variable& queueCv;
        thread worker;
        uint32_t partiesServed = 0;
        uint32_t totalTimeServiced = 0;
        bool stop;
};

class DungeonManager {
    public:
        DungeonManager(uint32_t numInstances, uint32_t t1, uint32_t t2) {
            for (uint32_t i = 0; i < numInstances; ++i) {
                auto instance = make_unique<DungeonInstance>(t1, t2, i + 1, partyQueue, queueMutex, queueCv);
                instance->run();
                dungeonInstances.push_back(move(instance));
            }
        }

        ~DungeonManager() {
            for (auto& instance : dungeonInstances) {
                instance->stopInstance();
            }
            printStats();
        }

        void enqueueParty(const Party& party) {
            lock_guard<mutex> lock(queueMutex);
            partyQueue.push(party);
            queueCv.notify_one();
        }

        void printStats() {
            for (const auto& instance : dungeonInstances) {
                printf("%s\n", instance->getInstanceStats().c_str());
            }
        }

        void waitForCompletion() {
            while (true) {
                {
                    unique_lock<mutex> lock(queueMutex);
                    if (partyQueue.empty()) break;
                }
                this_thread::sleep_for(chrono::milliseconds(100));
            }
            
            for (auto& instance : dungeonInstances) {
                instance->stopInstance();
            }
        }

    private:
        queue<Party> partyQueue;
        mutex queueMutex;
        condition_variable queueCv;
        vector<unique_ptr<DungeonInstance>> dungeonInstances;
};

uint32_t parseInt(const string &s, const string &key) {
    string digits;
    bool isTime = key.find("seconds") != string::npos;
    bool isFloat = false;
    int msDigitCount = isTime ? 0 : 3;
    for (char c : s) {
        if(isFloat) msDigitCount++;
        if (c == '.') {
            if (isTime) {
                isFloat = true;
            } else {
                break;
            }
        }
        if (isdigit(c)) digits += c;
        if(msDigitCount == 3 && isTime) break;
    }
    
    unsigned long value = digits.empty() ? 0 : stoul(digits) * static_cast<unsigned long>(pow(10, 3 - msDigitCount));
    if (value > UINT32_MAX) {
        throw out_of_range("Value exceeds uint32_t limits");
    }
    return static_cast<uint32_t>(value);
}

void getConfig(uint32_t &n, uint32_t &t, uint32_t &h, uint32_t &d, uint32_t &t1, uint32_t &t2) {
    ifstream config_file("config.txt");
    if (!config_file) {
        throw runtime_error("Failed to open config file");
    }

    unordered_map<string, uint32_t*> configMap = {
        {"max_number_of_instances", &n},
        {"number_of_tanks", &t},
        {"number_of_healers", &h},
        {"number_of_dps", &d},
        {"min_finish_time_seconds", &t1},
        {"max_finish_time_seconds", &t2}
    };

    string line;
    while (getline(config_file, line)) {
        auto delimiterPos = line.find('=');
        if (delimiterPos == string::npos) continue;

        string key = line.substr(0, delimiterPos);
        transform(key.begin(), key.end(), key.begin(), ::tolower);
        string value = line.substr(delimiterPos + 1);

        key.erase(remove_if(key.begin(), key.end(), ::isspace), key.end());

        if (configMap.find(key) != configMap.end()) {
            *configMap[key] = parseInt(value, key);
        }
    }

    cout << "Configurations:" << endl;
    cout << "----------------" << endl;
    for (const auto& [key, valuePtr] : configMap) {
        cout << key << " = " << *valuePtr << endl;
    }
    cout << "----------------" << endl;

    if (t1 > t2) {
        throw invalid_argument("Minimum finish time is greater than maximum finish time");
    }
}

int main() {
    uint32_t t1, t2, h, d, t, n;
    try {
        getConfig(n, t, h, d, t1, t2);
    } catch (const exception& e) {
        cerr << "Error: " << e.what() << endl;
        return 1;
    }
    DungeonManager manager(n, t1, t2);

    uint32_t partiesToServe = min({t, h, d / 3});

    for (uint32_t i = 1; i <= partiesToServe; ++i) {
        manager.enqueueParty(Party(i));
    }

    manager.waitForCompletion();
    return 0;
}