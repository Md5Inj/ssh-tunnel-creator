//
// Created by maksimromashko on 6.5.23.
//

#ifndef SSH_TUNNEL_CONFIG_H
#define SSH_TUNNEL_CONFIG_H

#include <sys/stat.h>
#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>
#include <string>
#include <filesystem>
#include <iostream>
#include <fstream>
#include <jsoncpp/json/json.h>

namespace fs = std::filesystem;
using namespace std;

class Config {
private:
    map<string, map<string, map<string, string>>> config;

    string getConfigDir() {
        struct passwd *pw = getpwuid(getuid());
        const char *homedir = pw->pw_dir;
        return string(homedir) + "/" + ".config/MaksimRamashka/sshTunnel";
    }

    string getConfigFilePath() {
        string configDir = this->getConfigDir();
        return configDir + "/" + "tunnels";
    }

    void createConfigFile() {
        string configFilePath = this->getConfigFilePath();

        if (!filesystem::exists(configFilePath)) {
            ofstream output(configFilePath);
        }
    }

    void createConfigDir() {
        struct stat sb;
        int statResult = stat(this->getConfigDir().c_str(), &sb);
        if (stat(this->getConfigDir().c_str(), &sb) == 0) {
            // Directory exists
        } else {
            try {
                fs::create_directories(this->getConfigDir().c_str());
            } catch (const exception& e) {
                cerr << e.what() << '\n';
            }
        }
    }

    map<string, map<string, map<string, string>>> readConfigFile() {
        ifstream t(this->getConfigFilePath());
        stringstream buffer;
        buffer << t.rdbuf();
        string configJson = buffer.str();

        Json::Reader reader;
        Json::Value jsonObject;
        reader.parse(configJson, jsonObject);

        map<string, map<string, map<string, string>>> boxConfig;

        for (auto box = jsonObject.begin(); box != jsonObject.end(); box++) {
            if (box->isObject()) {
                map<string, map<string, string>> portsConfig;

                for (auto ports = box->begin(); ports != box->end(); ports++) {
                    map<string, string> portConfig;

                    for (auto port = ports->begin(); port != ports->end(); port++) {
                        portConfig[port.key().asString()] = port->asString();
                    }

                    portsConfig.insert(pair<string, map<string, string>>(ports.key().asString(), portConfig));
                }

                boxConfig[box.key().asString()] = portsConfig;
            }
        }

        this->config = boxConfig;

        return this->config;
    }

public:
    Config() {
        this->createConfigDir();
        this->createConfigFile();
        this->readConfigFile();
    }

    map<string, map<string, map<string, string>>> getConfig() {
        return this->config;
    }

    void writeConfig(map<string, string> portConfig, string boxName, int provisionedPort) {
        if (this->config[boxName].empty()) {
            map<string, map<string, string>> portsConfig;
            portsConfig.insert(pair<string, map<string, string>>(to_string(provisionedPort), portConfig));
            config[boxName] = portsConfig;
        } else {
            auto ports = this->config.find(boxName)->second;
            ports.insert(pair<string, map<string, string>>(
                    to_string(provisionedPort),
                    portConfig
            ));

            this->config[boxName] = ports;
        }

        Json::Value boxConfig;
        for (auto & parentConfigItem : this->config) {
            Json::Value portsConfig;

            for (auto & childConfigItem : parentConfigItem.second) {
                Json::Value portConfigArray;

                for (auto & portConfigItem: childConfigItem.second) {
                    portConfigArray[portConfigItem.first] = portConfigItem.second;
                }

                portsConfig[childConfigItem.first] = portConfigArray;
            }

            boxConfig[parentConfigItem.first] = portsConfig;
        }

        ofstream outputFile(this->getConfigFilePath(), ios::trunc);

        if (outputFile.is_open()) {
            outputFile << boxConfig.toStyledString() + "\n";
            outputFile.close();
        } else {
            cerr << "Error opening file for appending." << endl;
        }
    }
};


#endif //SSH_TUNNEL_CONFIG_H
