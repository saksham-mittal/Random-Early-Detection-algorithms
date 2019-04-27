/* 
    To run the gateway.cpp file:
    g++ gateway.cpp -o gateway -std=c++11 -lpthread
    To execute:
    ./gateway 1 100 low
*/
#include "../../include/gateway.h"

void gateway::setupConnection() {
    // Creating socket for the gateway
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
        // servlen = sizeof(serverAddr);
        clilen = sizeof(clientAddr);
    }
}

bool gateway::REDBlock(packet &Packet) {
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
            printf("Count has reached 1/maxp. Dropping packet\n");
            pa = 1.0;
            return true;
        }
        double randomP = (rand()%100)/100.00;
        // Dropping packet with probability pa
        if(randomP <= pa) {
            if(count != 50)
                printf("Dropping packet\n");
            // Resetting count to 0
            count = 0;
            return true;
        } else {
            printf("Packet buffered\n");
            Queue.push(Packet);
            // Initialize count to -1 since packet is buffered
            count = -1;
            return false;
        }
    } else if(maxThreshold <= avg) {
        // Queue size is more than max threshold allowed
        // Drop all packets 
        printf("Dropping packet\n");
        count = 0;
        return true;
    } else {
        // Average queue length is less than minimum threshold 
        // Accept all packets
        printf("Packet buffered\n");
        Queue.push(Packet);
        // Since the average queue length is below minimum threshold, initialize count to -1
        count = -1;
        return false;
    }
    // Printing the queue
    // showq(Queue);
}

void gateway::simulateRRED(int arrivalTime) {
    mtx2.lock();
    while(bufferPackets.size()) {
        int buffer_size = bufferPackets.size();

        // Process the packets in the buffer using RRED algorithm
        for(int i=0; i<buffer_size; i++) {
            mtx.lock();
            flow f;
            packet Packet = bufferPackets.front();
            pair<int, int> pr = make_pair(Packet.clientNo, Packet.destPortNo);
            if(mpFlow.find(pr) != mpFlow.end()) {
                // The flow already exists
                f = mpFlow[pr];
            } else {
                // Adding the flow to the unordered map
                flow fNew;
                mpFlow[pr] = fNew;
            }
            // Finding the most recent time when packet was dropped
            int Tmax = max(f.T1, T2);

            if(arrivalTime >= Tmax and arrivalTime <= Tmax + Tstar) {
                // The packet is coming very rapidly
                // Decrease the indicator of flow by 1
                f.indicator = min(f.indicator - 1,-3);
            } else {
                // Else increase the indicator of flow by 1
                f.indicator = max(10,f.indicator+1);
            }
            if(f.indicator >= 0) {
                // Calling RED block on the packet
                if(REDBlock(Packet)) {
                    // True means, the packet was dropped
                    T2 = arrivalTime;
                }
            } else {
                // Means the packet was dropped
                // Assigning the T1 value of the flow
                f.T1 = arrivalTime;
            }
            mpFlow[pr] = f;
            mtx.unlock();
            bufferPackets.erase(bufferPackets.begin()); 
        }
    }
    mtx2.unlock();
}

void gateway::dequeQueue() {
    mtx.lock();
    while(!Queue.empty()) { 
        packet Packet = Queue.front();
        // Send the packet to the outlink using the destPortNo and the Forwarding table
        int pNo = Packet.destPortNo;

        if(pNo != -1) {
            int outlinkPortNo = portId[pNo];
            int count = send(mp[outlinkPortNo], &Packet, sizeof(Packet), 0);
            if(count < 0) {
                printf("Error on sending.\n");
            }
        } else {
            for(auto elem : mp) {
                int count = send(elem.second, &Packet, sizeof(Packet), 0);
                if(count < 0) {
                    printf("Error on sending.\n");
                } else {
                    printf("Last packet sent to outlink on port %d\n", elem.first);
                }
            }
        }
        
        Queue.pop();
    }
    mtx.unlock();
    // cout << "Queue is dequeed\n";
}

void gateway::receivePackets(int id) {
    while(1) {
        packet recvpacket;
        int count = recv(clientsSockid[id], &recvpacket, sizeof(recvpacket), 0);
        if(count < 0) {
            printf("Error on receiving message from socket %d.\n", id);
        }
        if(recvpacket.isLast) {
            mtx3.lock();
            receivedLastPackets++;
            mtx3.unlock();
            cout << "Recieved Last Packet" << endl;
            return;
        }
        // Add the recieved packet to the shared buffer
        mtx2.lock();
        bufferPackets.push_back(recvpacket);
        //  printf("recieved packet with prioitiy %d\n",recvpacket.priority);
        mtx2.unlock();
    }
}

void gateway::acceptMethod(int index, string traffic) {
    // Connect to outlinks
    set<int> s;         // Contains the port nos. of the outlinks
    for(auto elem : portId) {
        s.insert(elem.second);
    }

    set<int> :: iterator itr;
    itr = s.begin();
    cout << "---------------------\n";
    cout << "Outlink's port numbers\n";
    for(auto elem : s)
        cout << elem << "\n";
    cout << "---------------------\n";

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
    if(index==2)
        usleep(1300000);

    receivedLastPackets = 0;
    thread clients[maxNumClients];
    for(int i=0; i<maxNumClients; i++) {
        // We can't call non static member methods directly from threads
        clients[i] = thread(&gateway::receivePackets, this, i);
    }

    cout << "Starting RED algorithm simulation for " << traffic << " traffic\n";

    // Queue is idle when created
    // so intializing qTime with current time
    qTime = time(NULL);

    for(int t=0; t<simTime; t++) {
        auto start = chrono::steady_clock::now();
        usleep(100000 * (index + 2));

        // We are simulating the case that the gateway is 
        // forwarding all the packets to the respective servers
        dequeQueue();

        cout << "#" << t + 1 << ": " << endl;
        simulateRRED(t);
        auto end = chrono::steady_clock::now();
        int tTaken = chrono::duration_cast<chrono::microseconds>(end - start).count();
        
        mtx.lock();
        fout << Queue.size() << "\t" << avg << endl;
        mtx.unlock();
        usleep(1000000 - tTaken);
    }
    cout << "Simulation finished\n";

    for(int i=0; i<maxNumClients; i++) {
        clients[i].join();
    }
    packet recvpacket;
    recvpacket.isLast = true;
    recvpacket.destPortNo = -1;
    Queue.push(recvpacket);

    for(int i=0; i<maxNumClients; i++)
        close(clientsSockid[i]);

    dequeQueue();
    cout << "Close connection packet sent\n";
    
    // Closing the sockets to the outlinks
    for(auto elem : s) {
        close(mp[elem]);
    }
}

int main(int argc, char const** argv) {
    if(argc != 4) {
        cout << "Usage ./gateway <index-no> <sim-time> <traffic>\n";
        cout << "Available traffic: high, mid, and low\n";
        exit(1);
    }

    int indexNo = stoi(argv[1]) - 1;
    int st = stoi(argv[2]); 
    gateway gt(indexNo, st, string(argv[3]), "././samples/RRED/topology/topology-gateway.txt");

    gt.setupConnection();

    // Starting simulation for this gateway 
    gt.acceptMethod(indexNo, argv[3]);
    return 0;
}