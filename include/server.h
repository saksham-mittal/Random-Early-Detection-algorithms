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
#include <set>
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

        sockid = socket(PF_INET, SOCK_STREAM, 0);
        if(sockid < 0) {
            printf("Socket could not be opened.\n");
        }

        addrport.sin_family = AF_INET;
        addrport.sin_port = htons(portNo);
        addrport.sin_addr.s_addr = htonl(INADDR_ANY);
        int t = 1;
        setsockopt(sockid, SOL_SOCKET, SO_REUSEADDR, &t, sizeof(int));

        if(bind(sockid, (struct sockaddr *)&addrport, sizeof(addrport)) < 0) {
            printf("Error in binding socket\n");
        } else {
            // Socket is bound
            int status = listen(sockid, maxNumClients);
            if(status < 0) {
                printf("Error in listening.\n");
            }
            clilen = sizeof(clientAddr);
        }

        acceptMethod(index);
    }

    // This method accepts connection from all clients
    // and calls the receive method on threads for each client
    void acceptMethod(int index);

    // The server creates thread for each inlink and calls this method
    void receivePackets(int id);
};