#include "DungeonManager.h"
#include <iostream>
#include <fstream>
#include <unordered_map>
#include <stdexcept>
#include <algorithm>
#include <cmath>

using namespace std;

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
        if(key.find("seconds") != string::npos) 
            cout << key << " = " << *valuePtr / 1000.00 << "s" << endl;
        else
            cout << key << " = " << *valuePtr << endl;
    }
    cout << "----------------" << endl;

    if (t1 > t2) {
        throw invalid_argument("Minimum finish time is greater than maximum finish time");
    }
    if (t1 == 0 || t2 == 0) {
        throw invalid_argument("Finish times cannot be zero");
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
    cout << "Parties to serve: " << partiesToServe << endl;

    uint32_t dpsOverflow = d - (partiesToServe * 3);
    uint32_t healerOverflow = h - partiesToServe;
    uint32_t tankOverflow = t - partiesToServe;

    for (uint32_t i = 1; i <= partiesToServe; ++i) {
        manager.enqueueParty(Party(i));
    }
    manager.run();
    manager.waitForCompletion();
    cout << "\nPlayers remaining in queue without a party: " << endl;
    cout << "Tanks: " << tankOverflow << endl;
    cout << "Healers: " << healerOverflow << endl;
    cout << "DPS: " << dpsOverflow << endl;
    return 0;
}