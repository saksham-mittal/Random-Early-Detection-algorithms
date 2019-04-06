/*
    To run the client.cpp file:
    g++ client.cpp -o client -std=c++11 -lpthread
    To execute:
    ./client 1 100 low
*/
#include "../include/client.h"

void client::sendPacket(int id, int ind) {
    packet *Packet = new packet();
    Packet->destPortNo = dPortNo;

    // Write a character to the socket
    Packet->charPayload = charArray[ind];
    
    int count = send(sockid, Packet, sizeof(*Packet), 0);
    if(count < 0) {
        printf("Error on sending.\n");
    }
}

bool client::connectionSetup() {
    // Creating socket
    sockid = socket(PF_INET, SOCK_STREAM, 0);
    if(sockid < 0) {
        printf("Socket could not be opened.\n");
        return false;
    }

    addrport.sin_family = AF_INET;
    addrport.sin_port = htons(portNo);
    addrport.sin_addr.s_addr = inet_addr("127.0.0.1");

    if(connect(sockid, (struct sockaddr *) &addrport, sizeof(addrport)) < 0) {
        printf("Connection failed.\n");
        return false;
    }

    int n = 1;
    // Set socket options to TCP_NODELAY which disables Nagle's algorithm -
    //  - buffering short packets until a large packet is created to save bandwidth
    n = setsockopt(sockid, IPPROTO_TCP, TCP_NODELAY, &n, sizeof(n));
    if (n < 0)  {
        printf("ERROR enabling TCP_NODELAY");
        return false;
    }
}

void client::simulateHost(int index, int simTime) {
    cout << "Starting client " << index + 1 << " simulation\n";

    for(int i=0; i<simTime; i++) {
        // The simTime takes to effect in 1 second
        auto start = chrono::steady_clock::now();
        srand((index + 1) * time(NULL));
        // srand((index + 1) * (i + 1));
        int num = rand() % 2;       // Sending bursts randomly
        
        cout << "#" << i + 1 << ": ";
        // cout << num << endl;
        if(num == 1) {
            cout << "Sending burst\n";
            // Send burstSize packets in a burst(using threads)
            thread sendTh[burstSize];
            for(int j=0; j<burstSize; j++) {
                sendTh[j] = thread(&client::sendPacket, this, j, ind);
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
        usleep(1000000 - tTaken);
    }
    cout << "Client " << index + 1 << " finished\n";
}

int main(int argc, char const** argv) {
    if(argc != 4) {
        cout << "Usage ./client <index> <sim-time> <traffic>\n";
        cout << "Available traffic: high, mid, and low\n";
        exit(1);
    }
    int index = stoi(argv[1]) - 1;
    int simTime = stoi(argv[2]);
    string traffic = argv[3];

    client cl(index, simTime, traffic);
    return 0;
}