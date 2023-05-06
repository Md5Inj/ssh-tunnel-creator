#include "TunnelManager.h"

using namespace std;

void usage()
{
    cout << "SSH tunnel creator " << "[options]" << endl <<
         "Options:" << endl <<
         "-h        Print this help" << endl <<
         "-l        Print list of saved SSH tunnels" << endl <<
         "-V        Print the proxy version" << endl <<
         "-d        Run as daemon" << endl <<
         "-P        Path to PID file (default: " << endl;
}

int main(int argc, char * argv[]) {
    TunnelManager tunnelManager;

    if (argc == 1) {
        usage();
        return 0;
    }

    if (argv[1] == "-l") {
        tunnelManager.
    }

//    tunnelManager.openPort();

    return 0;
}