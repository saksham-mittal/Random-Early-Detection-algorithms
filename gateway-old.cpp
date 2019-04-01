/* 
    To run the gateway.cpp file:
    g++ gateway.cpp -o gateway -std=c++11 -lpthread
    To execute:
    ./gateway 3542 2
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
#include <queue>
#include <mutex>
#include <iostream>
#include <fstream>

using namespace std;
int sockid, maxNumClients;
struct sockaddr_in addrport, clientAddr;
socklen_t clilen;
int *clientsSockid;
queue<char> Queue;
mutex mtx;

ofstream fout;

// Class to represent a single packet
class packet {
    public:
        bool isLast;
        char charPayload;

    packet() {
        isLast=false;
    }    
};

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
int minThreshold = 5, maxThreshold = 15;
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
    printf("Queue length: %lu\n", Queue.size());
    printf("Average queue length: %f\n", avg);

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
        // Dropping packet with probability p 
        if(randomP < pa) {
            printf("Dropping packet: %c\n", buffer[0]);
            // Resseting count to 0
            count = 0;
        } else {
            Queue.push(buffer[0]);
        }
    } else if(maxThreshold <= avg) {
        // Queue size is more than max threshold allowed
        // Drop all packets 
        printf("Dropping packet: %c\n", buffer[0]);
        count = 0;
    } else {
        // Average queue length is less than minimum threshold 
        // Accept all packets
        Queue.push(buffer[0]);
        // Since the average queue length is below minimum threshold, initialize count to -1
        count = -1;
    }
}

void simulateRED(int index) {
    char* buffer = (char *)malloc(sizeof(char));
    // i denotes the number of packets received by gateway till now
    int i = 0;
    while(1) {
        packet *recvpacket=new packet;
        int count = recv(clientsSockid[index], recvpacket, sizeof(*recvpacket), 0);
        if(count < 0) {
            printf("Error on receiving message from socket %d.\n", index);
        }

        // Process the packets using RED algorithm
        mtx.lock();
        red(&(recvpacket->charPayload));
        mtx.unlock();

        // Printing the queue
        showq(Queue);
        i++;
        // Delete 3 elements for every 5th packet added to the queue
        // This is used to simulate that the gateway is also delivering 
        // the packets and hence queue is also getting dequeued
        if(i % 5 == 0) {
            i = 0;
            if(!Queue.empty())
                Queue.pop();
            if(!Queue.empty())
                Queue.pop();
            if(!Queue.empty())
                Queue.pop();
        }


        if(recvpacket->isLast)
            break;
    }
    close(clientsSockid[index]);
}

void acceptingThread() {
    clientsSockid = new int[maxNumClients];
    thread clients[maxNumClients];
    // Queue is idle when created
    qTime = time(NULL);

    for(int i=0; i<maxNumClients; i++) {
        clientsSockid[i] = accept(sockid, (struct sockaddr *)&clientAddr, &clilen);
        clients[i] = thread(simulateRED, i);
    }
    for(int i=0; i<maxNumClients; i++) {
        clients[i].join();
    }
}

void dequeingThread() {
    while(1) {
        mtx.lock();
        fout << Queue.size() << "\t" << avg << endl;
        while(!Queue.empty()) { 
            Queue.pop(); 
        }
        mtx.unlock();
        sleep(1);
        cout << "Queue is dequeed\n";
    }
}

int main(int argc, char const** argv) {
    if(argc < 3) {
        cout << "Usage ./gateway <port-no> <max-clients>\n";
        exit(1);
    }
    maxNumClients = stoi(argv[2]);
    sockid = socket(PF_INET, SOCK_STREAM, 0);
    if(sockid < 0) {
        printf("Socket could not be opened.\n");
    }

    addrport.sin_family = AF_INET;
    addrport.sin_port = htons(stoi(argv[1]));
    addrport.sin_addr.s_addr = htonl(INADDR_ANY);
    int t = 1;
    setsockopt(sockid, SOL_SOCKET, SO_REUSEADDR, &t, sizeof(int));

    fout.open("log-red.txt");
    
    if(bind(sockid, (struct sockaddr *)&addrport, sizeof(addrport)) != -1) {
        // Socket is binded
        cout << "Socket binding successful\n";
        int status = listen(sockid, maxNumClients);
        if(status < 0) {
            printf("Error in listening.\n");
        }
        clilen = sizeof(clientAddr);
        thread dequeThread = thread(dequeingThread);
        dequeThread.detach();

        thread acceptThread = thread(acceptingThread);
        acceptThread.join();
    }
    return 0;
}