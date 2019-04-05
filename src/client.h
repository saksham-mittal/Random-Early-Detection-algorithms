#include <stdio.h>
#include <cstdlib>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <thread>
#include <strings.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <vector>
#include <fstream>
#include <iostream>
#include <chrono>

using namespace std;

// Class to represent a single packet
class packet {
public:
    bool isLast;
    int destPortNo;
    char charPayload;
    int clientNum;

    packet() {
        isLast=false;
    }    
};

class client {
public:
    int sockid, portNo, dPortNo, numHosts, burstSize, ind = 0;
    char charArray[6] = "abcde";
    struct sockaddr_in addrport;
    
    client(int index, int simTime, string traffic) {
        // Reading topology file for getting the info of the gateway
        ifstream fin;
        string fileName = "./topology/topology-client.txt";
        fin.open(fileName);
        int n, x, y;
        fin >> n;
        for(int i=0; i<n; i++) {
            fin >> x >> y;
            if(i == index) {
                portNo = x;
                dPortNo = y;
                break;
            }
        }
        cout << portNo << " " << dPortNo << endl;
        // Reading topology file complete

        // Reading hostrate file
        fileName = "./samples/" + traffic + "/hostrate-" + traffic + ".txt";
        fin.open(fileName);
        fin >> numHosts;

        for(int i=0; i<numHosts; i++) {
            fin >> x;
            if(i == index) {
                burstSize = x;
                break;
            }
        }
        // Reading hostrate file complete
        fin.close();

        connectionSetup();

        simulateHost(index, simTime);
    }

    // Method for sending packets in burst
    void sendPacket(int id, int ind);

    // Returns true if the connection is set up successfully
    bool connectionSetup();

    // Method for simulating the host
    void simulateHost(int index, int simTime);
};