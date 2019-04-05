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

class server {
public:
    int sockid, portNo, maxNumClients;
    struct sockaddr_in addrport, clientAddr;
    socklen_t clilen;
    int *clientsSockid;
    int numClients=0,lastPacketsRecieved=0;
    mutex mtx;

    server(int index) {
        // Reading topology file for getting the info of the server
        ifstream fin;
        string fileName = "./topology/topology-server.txt";
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
        cout << portNo << " " << maxNumClients << endl;

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

        acceptMethod();
    }

    // This method accepts connection from all clients
    // and calls the receive method on threads for each client
    void acceptMethod();

    void receivePackets(int id);
};