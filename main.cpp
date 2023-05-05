#include <iostream>
#include <stdexcept>
#include <cstdio>
#include <string>
#include <jsoncpp/json/json.h>
#include <cctype>
#include <sys/wait.h>
#include <stdlib.h>

using namespace std;

const int MYSQL_TUNNEL_TYPE = 0;
const int ELASTICSEARCH_TUNNEL_TYPE = 1;
const int CUSTOM_TUNNEL_TYPE = 2;

#define READ   0
#define WRITE  1
FILE * popen2(string command, string type, int & pid)
{
    pid_t child_pid;
    int fd[2];
    pipe(fd);

    if((child_pid = fork()) == -1)
    {
        perror("fork");
        exit(1);
    }

    /* child process */
    if (child_pid == 0)
    {
        if (type == "r")
        {
            close(fd[READ]);    //Close the READ end of the pipe since the child's fd is write-only
            dup2(fd[WRITE], 1); //Redirect stdout to pipe
        }
        else
        {
            close(fd[WRITE]);    //Close the WRITE end of the pipe since the child's fd is read-only
            dup2(fd[READ], 0);   //Redirect stdin to pipe
        }

        setpgid(child_pid, child_pid); //Needed so negative PIDs can kill children of /bin/sh
        execl("/bin/sh", "/bin/sh", "-c", command.c_str(), NULL);
        exit(0);
    }
    else
    {
        if (type == "r")
        {
            close(fd[WRITE]); //Close the WRITE end of the pipe since parent's fd is read-only
        }
        else
        {
            close(fd[READ]); //Close the READ end of the pipe since parent's fd is write-only
        }
    }

    pid = child_pid;

    if (type == "r")
    {
        return fdopen(fd[READ], "r");
    }

    return fdopen(fd[WRITE], "w");
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
    string output = "";

    do {
        output = exec(("ss -tunlp | grep " + to_string(port)).c_str());
        port++;
    } while (!output.empty());

    return port;
}

int main() {
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
        return 1;
    }

    int portToOpen = getFreePort();
    string boxIp = runningBoxes[boxIndex].find("address")->second;
    string tunnelCommand = "ssh -L " + to_string(boxPort) + ":127.0.0.1:" + to_string(portToOpen) + " www-data@" + boxIp + " -N -f";
    int tunnelPid;
    cout << tunnelCommand;
    popen2(tunnelCommand, "r", tunnelPid);
    cout << tunnelPid;

    return 0;
}