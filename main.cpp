#include "TunnelManager.h"

using namespace std;

void usage()
{
    cout << "SSH tunnel creator " << "[options]" << endl <<
         "Options:" << endl <<
         "-h        Print this help" << endl <<
         "-l        Print list of saved SSH tunnels" << endl <<
         "-o        Opens SSH tunnel based on the running LXC boxes" << endl <<
         "-rp       Reopen all SSH tunnels for specific virtual machine" << endl <<
         "-P        Path to PID file (default: " << endl;
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

int main(int argc, char * argv[]) {
    TunnelManager tunnelManager;

    if (argc == 1) {
        usage();
        return 0;
    }

    std::string arg1(argv[1]);

    if (arg1 == "-l") {
        tunnelManager.printTunnelsList();

        return 0;
    }

    if (arg1 == "-o") {
        tunnelManager.openPort();

        return 0;
    }

    if (arg1 == "-rp") {
        std::string boxName(argv[1]);
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

        int boxIndex;

        cout << "\033[0;30;44mIn which box you are want to reopen all tunnels?\033[0m" << " ";
        cin >> boxIndex;
        cout << endl;

        tunnelManager.reOpenPorts(runningBoxes[boxIndex]);

        return 0;
    }

    usage();

    return 0;
}