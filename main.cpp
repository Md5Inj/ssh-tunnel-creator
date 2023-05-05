#include <iostream>
#include <stdexcept>
#include <cstdio>
#include <string>
#include <jsoncpp/json/json.h>
#include <cctype>

using namespace std;

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

int main() {
    string output = exec("lxc list -f json");

    Json::Reader reader;
    Json::Value obj;
    Json::Value defaultVal;
    Json::ValueIterator i;

    reader.parse(output, obj);

    vector<string> runningBoxes;

    for (i = obj.begin(); i != obj.end(); i++)
    {
        string status = i->get("status", defaultVal).asString();
        status = lowercaseString(status);

        if (status == "running") {
            runningBoxes.push_back(i->get("name", defaultVal).asString());
        }
    }

    return 0;
}