/*
    To run the client.cpp file:
    g++ client.cpp -o client -std=c++11 -lpthread
    To execute:
    ./client 1 100 low
*/
#include "../../include/client.h"

void client::sendPacket(int id, int seqNo, int priority) {
    packet Packet;
    Packet.destPortNo = dPortNo;
    Packet.seqNo = seqNo;
    Packet.priority = priority;

    // Write a character to the socket
    Packet.charPayload = charArray[ind];
    
    int count = send(sockid, &Packet, sizeof(Packet), 0);
    if(count < 0) {
        printf("Error on sending.\n");
    }
    /* else {
        cout << "Sent packet with priority " << Packet.priority << endl;
    } */
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

    return true;
}

void client::simulateHost(int index, int simTime) {
    cout << "Starting client " << index + 1 << " simulation\n";
    int runningSum = 0;
    
    //to log the throughput
    ofstream foutLog;
    foutLog.open(("samples/taildrop/log/log-client/sent-" + to_string(index+1) + ".txt").c_str());
    foutLog << priority << endl;

    cout<< "Burst size= " << burstSize <<endl;
    for(int i=0; i<simTime; i++) {
        // The simTime takes to effect in 1 second
        auto start = chrono::steady_clock::now();
        srand((index + 1) * time(NULL));

        int num = rand() % 2;       // Sending bursts randomly
        
        cout << "#" << i + 1 << ": ";

        if(num == 1) {
            cout << "Sending burst\n";
            // Send burstSize packets in a burst(using threads)
            thread sendTh[burstSize];
            for(int j=0; j<burstSize; j++) {
                sendTh[j] = thread(&client::sendPacket, this, j, i, priority);
            }

            for(int j=0; j<burstSize; j++)
                sendTh[j].join();

            ind = (ind + 1) % 5;
        } else
            cout << "Not Sending burst\n";
        
        // log how many packets sent
        foutLog << burstSize * num << endl;    
        
        
        if(i == simTime - 1) {
            // Send close connection packet
            packet Packet;

            // Write a character to the socket
            Packet.isLast = true;
            // Last packet's sequence number is set to 100
            Packet.seqNo = 100;
            int count = send(sockid, &Packet, sizeof(Packet), 0);
            if(count < 0) {
                printf("Error on sending.\n");
            }
        }
        auto end = chrono::steady_clock::now();
        int tTaken = chrono::duration_cast<chrono::microseconds>(end - start).count();
        usleep(1000000 - tTaken);
    }
    cout << "Client " << index + 1 << " finished\n";
    foutLog.close();
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

    client cl(index, simTime, traffic, "././samples/taildrop/topology/topology-client.txt");

    if(!cl.connectionSetup()) {
        cout<< "Failed to create connection" << endl;
        exit(-1);
    }

    cl.ind = 0;
    cl.simulateHost(index, simTime);
    return 0;
}