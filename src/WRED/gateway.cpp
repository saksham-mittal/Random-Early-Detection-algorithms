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

void gateway::wred(packet &Packet) {
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
    if(WREDminThresholds[Packet.priority] <= avg and avg < WREDmaxThresholds[Packet.priority]) {
        count++;
        pb = avg - WREDminThresholds[Packet.priority];
        pb = pb * maxp;
        pb = pb/(WREDmaxThresholds[Packet.priority] - WREDminThresholds[Packet.priority]+0.0);
        double pa = pb/(1 - (count * pb));
        if(count == 50) {
            // count has reached 1/maxp, 
            // Need to drop packets now
            printf("Count has reached 1/maxp. Dropping packet\n");
            pa = 1.0;
        }
        double randomP = (rand()%100)/100.00;
        // Dropping packet with probability pa
        if(randomP <= pa) {
            if(count != 50)
                printf("Dropping packet\n");
            // Resetting count to 0
            count = 0;
        } else {
            printf("Packet buffered\n");
            Queue.push(Packet);
            // Initialize count to -1 since packet is buffered
            count = -1;
        }
    } else if(WREDmaxThresholds[Packet.priority] <= avg) {
        // Queue size is more than max threshold allowed
        // Drop all packets of that priority
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
            //termination code
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

//setter for priorities
void gateway::setThresholds(const int *minThresholds,const int *maxThresholds,const int n_priorities)
{
    WREDminThresholds=new int[n_priorities];
    WREDmaxThresholds=new int[n_priorities];
    for(int i=0;i<n_priorities;i++)
    {
        WREDminThresholds[i]=minThresholds[i];
        WREDmaxThresholds[i]=maxThresholds[i];
    }
}

void gateway::simulateWRED() {
    mtx2.lock();
    while(bufferPackets.size()) {
        int buffer_size = bufferPackets.size();

        // Process the packets in the buffer using RED algorithm
        for(int i=0; i<buffer_size; i++) {
            mtx.lock();
            wred(bufferPackets.front());
            mtx.unlock();
            bufferPackets.erase(bufferPackets.begin());
        }
    }
    mtx2.unlock();
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
        assert(bufferPackets[bufferPackets.size()-1].priority == recvpacket.priority);
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

    //Required for synchronisation
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
        simulateWRED();

        mtx.lock();
        fout << Queue.size() << "\t" << avg << endl;
        mtx.unlock();

        auto end = chrono::steady_clock::now();
        int tTaken = chrono::duration_cast<chrono::microseconds>(end - start).count();
        
        usleep(1000000 - tTaken);
    }
    cout << "Simulation finished\n";

    for(int i=0; i<maxNumClients; i++) {
        clients[i].join();
    }

    packet *recvpacket = new packet;
    recvpacket->isLast = true;
    recvpacket->destPortNo = -1;
    Queue.push(*recvpacket);

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
    int minThresholds[]={5,8};
    int maxThresholds[]={17,22};
    gateway gt(indexNo, st, string(argv[3]),"././samples/WRED/topology/topology-gateway.txt");


    gt.setThresholds(minThresholds,maxThresholds,2);

    gt.setupConnection();

    // Starting simulation for this gateway 
    gt.acceptMethod(indexNo, argv[3]);
    return 0;
}