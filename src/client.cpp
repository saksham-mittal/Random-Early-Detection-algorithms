/*
    To run the client.cpp file:
    g++ client-new.cpp -o client -std=c++11 -lpthread
    To execute:
    ./client 3542 1 100 low
*/
#include <stdio.h>
#include <stdlib.h>
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

class packet
{
    public:
        bool isLast;
        char charPayload;

    packet()
    {
        isLast=false;
    }    
};

int sockid;
char charArray[6] = "abcde";

// Method for sending packets in burst
void sendPacket(int id, int ind) {
    packet *Packet = new packet();

    // Write a character to the socket
    Packet->charPayload = charArray[ind];
    int count = send(sockid, Packet, sizeof(*Packet), 0);
    if(count < 0) {
        printf("Error on sending.\n");
    }
}

int main(int argc, char const** argv) {
    if(argc < 3) {
        cout << "Usage ./gateway <port-no> <index> <sim-time> <traffic>\n";
        cout << "Available traffic: high, mid, and low\n";
        exit(1);
    }
    int index = stoi(argv[2]) - 1;
    int simTime = stoi(argv[3]);
    string traffic = argv[4];

    struct sockaddr_in addrport, clientAddr;
    struct hostent *server;

    sockid = socket(PF_INET, SOCK_STREAM, 0);
    if(sockid < 0) {
        printf("Socket could not be opened.\n");
    }

    addrport.sin_family = AF_INET;
    addrport.sin_port = htons(stoi(argv[1]));
    addrport.sin_addr.s_addr = inet_addr("127.0.0.1");

    if(connect(sockid, (struct sockaddr *) &addrport, sizeof(addrport)) < 0) {
        printf("Connection failed.\n");
    }
    
    int n = 1;
    // Set socket options to TCP_NODELAY which disables Nagle's algorithm -
    //  - buffering short packets until a large packet is created to save bandwidth
    n = setsockopt(sockid, IPPROTO_TCP, TCP_NODELAY, &n, sizeof(n));
    if (n < 0)  {
        printf("ERROR enabling TCP_NODELAY");
        exit(1);
    }
    int ind = 0, numHosts, x, burstSize;
    
    ifstream fin;
    string fileName = "./samples/" + traffic + "/hostrate-" + traffic + ".txt";
    fin.open(fileName);
    fin >> numHosts;

    for(int i=0; i<numHosts; i++) {
        fin >> x;
        if(i == index)
            burstSize = x; 
    }
    fin.close();

    cout << "Starting client " << index + 1 << "\n";
    for(int i=0; i<simTime; i++) {
        // The simTime takes to effect in 1 second
        auto start = chrono::steady_clock::now();

        srand((index + 1) * time(NULL));
        // srand((index + 1) * (i + 1));
        int num = rand() % 2;       // Sending bursts randomly
        
        // cout << num << endl;
        if(num == 1) {
            cout << "Sending burst\n";
            // Send burstSize packets in a burst(using threads)
            thread sendTh[burstSize];
            for(int j=0; j<burstSize; j++) {
                sendTh[j] = thread(sendPacket, j, ind);
            }

            for(int j=0; j<burstSize; j++)
                sendTh[j].join();

            ind = (ind + 1) % 5;
        } else
            cout << "Not Sending burst\n";
        if(i == simTime - 1) {
            // Send close connection packet
            packet *Packet = new packet();

            // Write a character to the socket
            Packet->isLast = true;
            int count = send(sockid, Packet, sizeof(*Packet), 0);
            if(count < 0) {
                printf("Error on sending.\n");
            }
        }
        auto end = chrono::steady_clock::now();
        int tTaken = chrono::duration_cast<chrono::microseconds>(end - start).count();
        // cout << tTaken << endl;
        usleep(1000000 - tTaken);
    }
    cout << "Client " << index + 1 << " finished\n";

    return 0;
}