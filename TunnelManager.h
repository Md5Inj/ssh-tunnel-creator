//
// Created by maksimromashko on 6.5.23.
//

#ifndef SSH_TUNNEL_TUNNELMANAGER_H
#define SSH_TUNNEL_TUNNELMANAGER_H

#include <iostream>
#include <stdexcept>
#include <cstdio>
#include <string>
#include <jsoncpp/json/json.h>
#include <cctype>
#include <sys/wait.h>
#include <cstdlib>
#include "Config.h"
#include "bprinter/table_printer.h"

const int MYSQL_TUNNEL_TYPE = 0;
const int ELASTICSEARCH_TUNNEL_TYPE = 1;
const int CUSTOM_TUNNEL_TYPE = 2;

class TunnelManager {
private:
Config config;

    /**
     * Runs ssh tunnel process and returns its pid
     *
     * @param portToProvision
     * @param localPort
     * @param machineIp
     * @return
     */
    int runSSH(int portToProvision, int localPort, string machineIp) {
        string tunnelCommand = "ssh -L " + to_string(portToProvision) + ":127.0.0.1:" + to_string(localPort) + " www-data@" + machineIp + " -N -f";
        exec(tunnelCommand.c_str());
        string pid = exec(("ps aux | grep '[s]sh.*" + to_string(portToProvision) + ".*-f' | tr -s ' ' | cut -d ' ' -f 2 | sort").c_str());

        return stoi(pid.c_str());
    }

    string exec(const char* cmd) {
        char buffer[128];
        string result;
        FILE* pipe = popen(cmd, "r");
        if (!pipe) throw runtime_error("popen() failed!");
        try {
            while (fgets(buffer, sizeof buffer, pipe) != nullptr) {
                result += buffer;
            }
        } catch (...) {
            pclose(pipe);
            throw;
        }
        pclose(pipe);
        return result;
    }

    string lowercaseString(string str)
    {
        char * strCharArray = new char [str.length()+1];
        std::strcpy (strCharArray, str.c_str());

        for (int x=0; x<strlen(strCharArray); x++)
            strCharArray[x] = (char)tolower(strCharArray[x]);

        str.assign(strCharArray);

        return str;
    }

    int getFreePort() {
        int port = 3306;
        string output;

        do {
            port++;
            output = exec(("netstat -antp 2>/dev/null | grep " + to_string(port)).c_str());
        } while (!output.empty());

        return port;
    }
public:
    TunnelManager() {
    }

    void printTunnelsList() {
        bprinter::TablePrinter tp(&std::cout);
        tp.AddColumn("Box name", 50);
        tp.AddColumn("Provisioned port", 16);
        tp.AddColumn("Tunnel is running", 17);
        tp.AddColumn("PID", 15);
        tp.AddColumn("Local port", 15);

        tp.PrintHeader();

        auto configArray = config.getConfig();

        for (auto & boxConfig: configArray) {
            for (auto & portsConfig: boxConfig.second) {
                tp << boxConfig.first;
                tp << portsConfig.first;
                for (auto & portConfig: portsConfig.second) {
                    if (portConfig.first == "pid") {
                        string processExists = exec(("ps -p " + portConfig.second + " | sed -n '2 p'").c_str());
                        if (!processExists.empty()) {
                            tp << "Yes";
                        } else {
                            tp << "No";
                        }
                        tp << portConfig.second;
                    } else {
                        tp << portConfig.second;
                    }
                }
            }
        }

        tp.PrintFooter();
    }

    void openPort() {
        string output = exec("lxc list -f json");

        Json::Reader reader;
        Json::Value obj;
        Json::Value defaultVal;
        Json::ValueIterator i;

        reader.parse(output, obj);

        vector<map<string, string>> runningBoxes;

        for (i = obj.begin(); i != obj.end(); i++)
        {
            string status = i->get("status", defaultVal).asString();
            status = lowercaseString(status);

            if (status == "running") {
                map<string, string> boxInfo;
                boxInfo.insert(pair<string, string>("name", i->get("name", defaultVal).asString()));
                boxInfo.insert(pair<string, string>("status", i->get("status", defaultVal).asString()));
                boxInfo.insert(pair<string, string>("address", i->get("state", defaultVal).get("network", defaultVal).get("eth0", defaultVal).get("addresses", defaultVal)[0].get("address", defaultVal).asString()));

                runningBoxes.push_back(boxInfo);
            }
        }

        int index = 0;
        for (auto box = runningBoxes.begin(); box != runningBoxes.end(); box++, index++)
        {
            cout << box->find("name")->second << " " << "[" + to_string(index) + "]" << endl;
        }

        int boxIndex, boxPort, tunnelType;

        cout << "\033[0;30;44mIn which box you are want to start tunnel?\033[0m" << " ";
        cin >> boxIndex;
        cout << endl;

        cout << "Mysql [0]" << endl;
        cout << "Elasticsearch [1]" << endl;
        cout << "Custom [2]" << endl;
        cout << "\033[0;30;44mWhich kind of tunnel you are want to start? \033[0m" << " ";
        cin >> tunnelType;

        switch (tunnelType) {
            case CUSTOM_TUNNEL_TYPE:
                cout << "Input custom port: ";
                cin >> boxPort;
                break;
            case MYSQL_TUNNEL_TYPE:
                boxPort = 3306;
                break;
            case ELASTICSEARCH_TUNNEL_TYPE:
                boxPort = 9200;
                break;
            default:
                boxPort = 0;
                break;
        }

        if (boxPort == 0) {
            cout << "Box port is empty";
            return;
        }

        int portToOpen = getFreePort();
        string boxIp = runningBoxes[boxIndex].find("address")->second;
        string tunnelCommand = "ssh -L " + to_string(portToOpen) + ":127.0.0.1:" + to_string(boxPort) + " www-data@" + boxIp + " -N -f";
        int tunnelPid = runSSH(portToOpen, boxPort, boxIp);
        cout << tunnelCommand;

        string boxName = runningBoxes[boxIndex].find("name")->second;

        map<string, string> boxConfigItems = {{string("provisionedPort"), to_string(portToOpen) },
                                              {string("pid"), to_string(tunnelPid)}};

        config.writeConfig(boxConfigItems, boxName, boxPort);
    }

    void reOpenPorts(map<string, string> box) {
        auto boxesConfig = this->config.getConfig();
        for (auto boxConfig: boxesConfig) {
            if (boxConfig.first == box.find("name")->second) {
               for (auto port: boxConfig.second) {
                   cout << port.first;
               }
            }
        }
    }
};


#endif //SSH_TUNNEL_TUNNELMANAGER_H
