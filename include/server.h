#include <stdio.h>
#include <cstdlib>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <math.h>
#include <netdb.h>
#include <thread>
#include <arpa/inet.h>
#include <chrono>
#include <queue>
#include <mutex>
#include <iostream>
#include <fstream>
#include <map>
#include "packet.h"

using namespace std;

class server {
public:
    int sockid, portNo, maxNumClients;
    struct sockaddr_in addrport, clientAddr;
    socklen_t clilen;
    int *clientsSockid;
    int numClients=0,lastPacketsRecieved=0;
    mutex mtx;

    server(int index,string topologyPath) {
        // Reading topology file for getting the info of the server
        ifstream fin;
        string fileName = topologyPath;
        fin.open(fileName);
        int numServs, x, y;
        
        fin >> numServs;
        for(int i=0; i<numServs; i++) {
            fin >> x >> y;
            if(i == index) {
                portNo = x;
                maxNumClients = y;
                break;
            }
        }

        cout << "Port number = " << portNo << "\nNumber of inlinks = " << maxNumClients << endl;
        fin.close();
        cout << "Topology file read\n";

    }

    // This method accepts connection from all clients
    // and calls the receive method on threads for each client
    void acceptMethod(int index);

    //create scoket and listen for connections from gateways
    void createConnection();

    // The server creates thread for each inlink and calls this method
    void receivePackets(int id,int index);
};