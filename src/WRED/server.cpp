/*
    To run the server.cpp file:
    g++ server.cpp -o server -std=c++11 -lpthread
    To execute:
    ./server 1
*/
#include "../../include/server.h"

void server::receivePackets(int id,int index) {
    int lastseqNo=0;
    map<int,int> runningSums;
    ofstream fout(("recvd-"+to_string(id)+"-"+to_string(index)+".txt").c_str());
    while(1) {
        packet *recvpacket = new packet;
        int count = recv(clientsSockid[id], recvpacket, sizeof(*recvpacket), 0);
        if(count < 0) {
            printf("Error on receiving message from socket %d.\n", id);
        }
        
       
        //check if packet with this priority has been recieved before
        if(runningSums.find(recvpacket->priority)==runningSums.end())
        {
            runningSums[recvpacket->priority]=0;
        }
        //if new seqNo detected print previous counts for all priorities
        if(recvpacket->seqNo!=lastseqNo)
        {
            fout<<lastseqNo<<endl;
            for(auto elem:runningSums)
            {
                cout<<elem.first<<" "<<elem.second<<endl;
                fout<<elem.first<<" "<<elem.second<<endl;
            }
            lastseqNo=recvpacket->seqNo;
        }
        //update running sums

         if(recvpacket->isLast==true) {
            cout << "Last packet received"<<endl;
            break;   
        }

        runningSums[recvpacket->priority]+=1;
        printf("packet recieved with priority %d,running sum=%d\n",recvpacket->priority,runningSums[recvpacket->priority]);
    }
    fout.close();
}

void server::acceptMethod(int index) {
    // Connect to clients
    clientsSockid = new int[maxNumClients];
    for(int i=0; i<maxNumClients; i++) {
        clientsSockid[i] = accept(sockid, (struct sockaddr *)&clientAddr, &clilen);
        cout << "Inlink " << i + 1 << " connected\n";
    }

    thread clients[maxNumClients];
    for(int i=0; i<maxNumClients; i++) {
        // We can't call non static member methods directly from threads
        clients[i] = thread(&server::receivePackets, this, i,index);
    }

    for(int i=0; i<maxNumClients; i++) {
        clients[i].join();
    }
    cout << "Server " << index + 1 << "'s simulation finished\n";
}


void server::createConnection(){

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

}



int main(int argc, char const** argv) {
    if(argc != 2) {
        cout << "Usage ./server <index>\n";
        exit(1);
    }

    int index = stoi(argv[1]) - 1;

    server sv(index,"././samples/WRED/topology/topology-server.txt");
    
    sv.createConnection();

    sv.acceptMethod(index);

    return 0;
}