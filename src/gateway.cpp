/* 
    To run the gateway.cpp file:
    g++ gateway.cpp -o gateway -std=c++11 -lpthread
    To execute:
    ./gateway 3542 100 low
*/
#include "gateway.h"

void gateway::showq(queue<char> q) { 
    queue<char> g = q; 
    while(!g.empty()) { 
        cout << '\t' << g.front(); 
        g.pop(); 
    }
    cout << '\n'; 
}

void gateway::red(packet* Packet) {
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
    /* For debugging reasons
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
            printf("Dropping packet\n");
            // Resseting count to 0
            count = 0;
        } else {
            printf("Packet buffered\n");
            Queue.push(Packet);
            // Initialize count to -1 since packet is buffered
            count = -1;
        }
    } else if(maxThreshold <= avg) {
        // Queue size is more than max threshold allowed
        // Drop all packets 
        printf("Dropping packet\n");
        count = 0;
    } else {
        // Average queue length is less than minimum threshold 
        // Accept all packets
        printf("Packet buffered\n");
        Queue.push(Packet);
        // Since the average queue length is below minimum threshold, initialize count to -1
        count = -1;
    }

    // Printing the queue
    // showq(Queue);
}

void gateway::dequeQueue() {
    mtx.lock();
    while(!Queue.empty()) { 
        packet *Packet = Queue.front();
        // Send the packet to the outlink using the destPortNo and the Forwarding table
        int pNo = Packet->destPortNo;
        int outlinkPortNo = portId[pNo];
        
        int count = send(mp[outlinkPortNo], Packet, sizeof(*Packet), 0);
        if(count < 0) {
            printf("Error on sending.\n");
        }
        Queue.pop();
    }
    mtx.unlock();
    // cout << "Queue is dequeed\n";
}

void gateway::simulateRED() {
    mtx2.lock();
    while(bufferPackets.size()) {
        int buffer_size = bufferPackets.size();

        // Process the packets in the buffer using RED algorithm
        for(int i=0; i<buffer_size; i++) {
            mtx.lock();
            red(bufferPackets[i]);
            mtx.unlock();
            bufferPackets.erase(bufferPackets.begin()); 
        }
    }
    mtx2.unlock();
}

void gateway::receivePackets(int id) {
    while(1) {
        packet *recvpacket = new packet;
        int count = recv(clientsSockid[id], recvpacket, sizeof(*recvpacket), 0);
        if(count < 0) {
            printf("Error on receiving message from socket %d.\n", id);
        }
        if(recvpacket->isLast)
        {
            Queue.push(recvpacket);
            return;
        }

        // Add the recieved packet to the shared buffer
        mtx2.lock();
        bufferPackets.push_back(recvpacket);
        // printf("recieved packet %lu\n",bufferPackets.size());
        mtx2.unlock();
    }
}

void gateway::acceptMethod(string traffic) {
    // Connect to outlinks
    set<int> s;         // Contains the port nos. of the outlinks
    for(auto elem : portId) {
        s.insert(elem.second);
    }

    set<int> :: iterator itr;
    itr = s.begin();
    cout << "Outlinks port nos.\n";
    for(auto elem : s)
        cout << elem << " ";
    cout << "\n";

    for(int i=0; i<s.size(); i++) {
        int sck = socket(PF_INET, SOCK_STREAM, 0);
        struct sockaddr_in servaddr;

        servaddr.sin_family = AF_INET;
        servaddr.sin_port = htons(*itr);
        servaddr.sin_addr.s_addr = inet_addr("127.0.0.1");

        if(connect(sck, (struct sockaddr *) &servaddr, sizeof(servaddr)) < 0) {
            printf("Connection failed.\n");
        }
        cout << "Outlink on port " << *itr << " connected\n";
        
        // mapping outlink port no. to its sockid
        mp[*itr] = sck;
        itr++;
    }
    // Connection to outlinks finished

    // Connect to clients
    clientsSockid = new int[maxNumClients];
    for(int i=0; i<maxNumClients; i++) {
        clientsSockid[i] = accept(sockid, (struct sockaddr *)&clientAddr, &clilen);
        cout << "Inlink " << i + 1 << " connected\n";
    }

    thread clients[maxNumClients];
    for(int i=0; i<maxNumClients; i++) {
        // We can't call non static member methods directly from threads
        clients[i] = thread(&gateway::receivePackets, this, i);
        clients[i].detach();
    }

    cout << "Starting RED algorithm simulation for " << traffic << " traffic\n";

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

    dequeQueue();

    // Closing the sockets to the outlinks
    for(auto elem : s) {
        close(mp[elem]);
    } 
    // close(servSockid);    
}

int main(int argc, char const** argv) {
    if(argc != 4) {
        cout << "Usage ./gateway <index-no> <sim-time> <traffic>\n";
        cout << "Available traffic: high, mid, and low\n";
        exit(1);
    }

    int indexNo = stoi(argv[1]);
    int st = stoi(argv[2]); 
    gateway gt(indexNo, st, argv[3]);

    return 0;
}