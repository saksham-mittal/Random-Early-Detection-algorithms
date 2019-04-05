/*
    To run the client.cpp file:
    g++ client-new.cpp -o client -std=c++11 -lpthread
    To execute:
    ./client 3542 1 100 low
*/
#include "server.h"

void server::receivePackets(int id) {
    while(1) {
        packet *recvpacket = new packet;
        int count = recv(clientsSockid[id], recvpacket, sizeof(*recvpacket), 0);
        if(count < 0) {
            printf("Error on receiving message from socket %d.\n", id);
        }
        if(recvpacket->isLast==true)
            break;   
    }
}

void server::acceptMethod() {
    // Connect to clients
    clientsSockid = new int[maxNumClients];
    for(int i=0; i<maxNumClients; i++) {
        clientsSockid[i] = accept(sockid, (struct sockaddr *)&clientAddr, &clilen);
        cout << "Inlink " << i + 1 << " connected\n";
    }

    thread clients[maxNumClients];
    for(int i=0; i<maxNumClients; i++) {
        // We can't call non static member methods directly from threads
        clients[i] = thread(&server::receivePackets, this, i);
    }

    for(int i=0; i<maxNumClients; i++) {
        clients[i].join();
    }
}

int main(int argc, char const** argv) {
    if(argc != 2) {
        cout << "Usage ./server <index>\n";
        exit(1);
    }

    int index = stoi(argv[1]) - 1;

    server sv(index);

    return 0;
}