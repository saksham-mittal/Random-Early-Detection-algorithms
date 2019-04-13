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
#include <assert.h>
#include "packet.h"

using namespace std;

class client {
public:
    int sockid, portNo, dPortNo, numHosts, burstSize, ind,priority;
    char charArray[6] = "abcde";
    struct sockaddr_in addrport;
    
    client(int index, int simTime, string traffic,string topologyPath) {
        // Reading topology file for getting the info of the gateway
        ifstream fin;
        string fileName = topologyPath;
        

        assert(fileName.find("topology")!=string::npos);
        cout<<fileName<<endl;
        fin.open(fileName);

        int n, x, y,prio;

        fin >> n;
        for(int i=0; i<n; i++) {
            fin >> x >> y>> prio;
            if(i == index) {
                portNo = x;
                dPortNo = y;
                priority=prio;
                break;
            }
        }
        
        cout << "Outlink port number = " << portNo << "\nDestination Port number = " << dPortNo << endl;
        fin.close();
        cout << "Topology file read\n";
        // Reading topology file complete

        // Reading hostrate file
       
        string outputPath = fileName.substr(0, fileName.find("topology"));
        string outFileName = outputPath + traffic + "/hostrate-" + traffic + ".txt";
        fin.open(outFileName);
        fin >> numHosts;

        for(int i=0; i<numHosts; i++) {
            fin >> x;
            if(i == index) {
                burstSize = x;
                break;
            }
        }
        cout << "Burst size of host = " << burstSize << endl;
        fin.close();
        // Reading hostrate file complete
    }

    // Method for sending packets in burst
    void sendPacket(int id, int seqNo=0, int priority=0);

    // Returns true if the connection is set up successfully
    bool connectionSetup();

    // Method for simulating the host
    void simulateHost(int index, int simTime);
};