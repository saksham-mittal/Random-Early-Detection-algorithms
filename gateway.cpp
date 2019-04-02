/* 
    To run the gateway.cpp file:
    g++ gateway.cpp -o gateway -std=c++11 -lpthread
    To execute:
    ./gateway 3542 100 low
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <math.h>
#include <netdb.h>
#include <thread>
#include <chrono>
#include <queue>
#include <mutex>
#include <iostream>
#include <fstream>

using namespace std;

// Class to represent a single packet
class packet {
    public:
        bool isLast;
        char charPayload;

    packet() {
        isLast=false;
    }    
};

int sockid, maxNumClients, simTime;
struct sockaddr_in addrport, clientAddr;
socklen_t clilen;
int *clientsSockid;
queue<char> Queue;
mutex mtx;
vector<packet*> bufferPackets;
ofstream fout;

void showq(queue<char> q) { 
    queue<char> g = q; 
    while(!g.empty()) { 
        cout << '\t' << g.front(); 
        g.pop(); 
    }
    cout << '\n'; 
}

// RED Algorithm's parameter initialization
double avg = 0;             // Average queue length
int count = -1;             // Count of packets since last probabilistic drop
double wq = 0.003;          // Queue weight; standard value of 0.002 for early congestion detection
int minThreshold = 5, maxThreshold = 17;
double maxp = 0.02;         // Maximum probability of dropping a packet; standard value of 0.02
double pb = 0;              // Probability of dropping a packet
time_t qTime;               // Time since the queue was last idle

void red(char* buffer) {
    // Calculating queue length
    if(Queue.size() == 0) {
        // double m = (time(NULL) - qTime)/0.001;
        double m = (time(NULL) - qTime);
        avg = pow((1 - wq), m) * avg;

        // Update qTime, since the queue is now empty
        qTime = time(NULL);
    } else {
        avg = ((1 - wq) * avg) + (wq * Queue.size());
    }
    /* 
    printf("Queue length: %lu\n", Queue.size());
    printf("Average queue length: %f\n", avg);
    */

    // Check if the average queue length is between minimum
    // and maximum threshold, then probabilistically drop
    // a packet
    if(minThreshold <= avg and avg < maxThreshold) {
        count++;
        pb = avg - minThreshold;
        pb = pb * maxp;
        pb = pb/(maxThreshold - minThreshold);
        double pa = pb/(1 - (count * pb));
        if(count == 50) {
            // count has reached 1/maxp, 
            // Need to drop packets now
            printf("Count has reached 1/maxp. Dropping next packet\n");
            pa = 1.0;
        }
        double randomP = (rand()%100)/100.00;
        // Dropping packet with probability pa
        if(randomP < pa) {
            printf("Dropping packet: %c\n", buffer[0]);
            // Resseting count to 0
            count = 0;
        } else {
            printf("Packet buffered\n");
            Queue.push(buffer[0]);
            // Initialize count to -1 since packet is buffered
            count = -1;
        }
    } else if(maxThreshold <= avg) {
        // Queue size is more than max threshold allowed
        // Drop all packets 
        printf("Dropping packet: %c\n", buffer[0]);
        count = 0;
    } else {
        // Average queue length is less than minimum threshold 
        // Accept all packets
        printf("Packet buffered\n");
        Queue.push(buffer[0]);
        // Since the average queue length is below minimum threshold, initialize count to -1
        count = -1;
    }

    // Printing the queue
    // showq(Queue);
}

int *hostRate;

void dequeQueue() {
    mtx.lock();
    while(!Queue.empty()) { 
        Queue.pop(); 
    }
    mtx.unlock();
    // cout << "Queue is dequeed\n";
}

// For bufferPackets synchronization
mutex mtx2;

void simulateRED() {
    mtx2.lock();
    while(bufferPackets.size()) {
        int buffer_size = bufferPackets.size();

        // Process the packets in the buffer using RED algorithm
        for(int i=0; i<buffer_size; i++) {
            mtx.lock();
            red(&(bufferPackets[i]->charPayload));
            mtx.unlock();
            bufferPackets.erase(bufferPackets.begin()); 
        }
    }
    mtx2.unlock();
}

void receivePackets(int id) {
    while(1) {
        packet *recvpacket = new packet;
        int count = recv(clientsSockid[id], recvpacket, sizeof(*recvpacket), 0);
        if(count < 0) {
            printf("Error on receiving message from socket %d.\n", id);
        }
        if(recvpacket->isLast)
            return;
        // Add the recieved packet to the shared buffer
        mtx2.lock();
        bufferPackets.push_back(recvpacket);
        // printf("recieved packet %lu\n",bufferPackets.size());
        mtx2.unlock();
    }
}

void acceptMethod() {
    clientsSockid = new int[maxNumClients];
    for(int i=0; i<maxNumClients; i++) {
        clientsSockid[i] = accept(sockid, (struct sockaddr *)&clientAddr, &clilen);
        cout << "Client " << i + 1 << " connected\n";
    }

    thread clients[maxNumClients];
    for(int i=0; i<maxNumClients; i++) {
        clients[i] = thread(receivePackets, i);
        clients[i].detach();
    }

    // Queue is idle when created
    // so intializing qTime with current time
    qTime = time(NULL);

    for(int t=0; t<simTime; t++) {
        auto start = chrono::steady_clock::now();
        fout << Queue.size() << "\t" << avg << endl;

        // We are simulating the case that the gateway is 
        // forwarding all the packets to the respective servers
        dequeQueue();

        simulateRED();
        auto end = chrono::steady_clock::now();
        int tTaken = chrono::duration_cast<chrono::microseconds>(end - start).count();
        usleep(1000000 - tTaken);
    }

    for(int i=0; i<maxNumClients; i++)
        close(clientsSockid[i]);
}

int main(int argc, char const** argv) {
    if(argc < 3) {
        cout << "Usage ./gateway <port-no> <sim-time> <traffic>\n";
        cout << "Available traffic: high, mid, and low\n";
        exit(1);
    }

    simTime = stoi(argv[2]);
    string traffic = argv[3];

    ifstream fin;
    string fileName = traffic + "/hostrate-" + traffic + ".txt";
    fin.open(fileName);
    fin >> maxNumClients;
    hostRate = new int[maxNumClients];

    for(int i=0; i<maxNumClients; i++)
        fin >> hostRate[i];
        
    sockid = socket(PF_INET, SOCK_STREAM, 0);
    if(sockid < 0) {
        printf("Socket could not be opened.\n");
    }
    addrport.sin_family = AF_INET;
    addrport.sin_port = htons(stoi(argv[1]));
    addrport.sin_addr.s_addr = htonl(INADDR_ANY);
    int t = 1;
    setsockopt(sockid, SOL_SOCKET, SO_REUSEADDR, &t, sizeof(int));

    fout.open("log.txt");
    // NOTE: Writing traffic level to log file
    // for plotter to read 
    fout << traffic << endl;

    if(bind(sockid, (struct sockaddr *)&addrport, sizeof(addrport)) != -1) {
        // Socket is binded
        int status = listen(sockid, maxNumClients);
        if(status < 0) {
            printf("Error in listening.\n");
        }
        clilen = sizeof(clientAddr);

        cout << "Starting RED algorithm simulation for " << traffic << " traffic\n";
        acceptMethod();
    }
    return 0;
}