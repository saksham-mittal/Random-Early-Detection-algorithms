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



// Method for sending packets in burst

int main(int argc, char const** argv) {
    if(argc < 1) {
        cout << "Usage ./server <port-no>\n";
        
        exit(1);
    }

    //set up connection to gateway

    struct sockaddr_in addrport, serverAddr;
    struct hostent *server;

    int sockid = socket(PF_INET, SOCK_STREAM, 0);
    if(sockid < 0) {
        printf("Socket could not be opened.\n");
    }

    addrport.sin_family = AF_INET;
    addrport.sin_port = htons(atoi(argv[1]));
    addrport.sin_addr.s_addr = inet_addr("127.0.0.1");
    cout<<argv[1]<<endl;
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
    int lastPackets = 0, x,rcv_bytes;



    cout << "Starting Server " <<"\n";
    while(1)
    {
        packet *recvpacket=new packet;
        rcv_bytes=recv(sockid,recvpacket,sizeof(*recvpacket),0);
        //count recieved last packets

        //TODO: Do something with recv'd packets
        if(recvpacket->isLast==true)
        {
            lastPackets++;
            cout<<"client finished"<<endl;
        }
        //break when all last packets recv'd    
        if(lastPackets==6)
            break;

    }

    cout<<"All packets recieved, Server Terminating"<<endl;
    close(sockid);
    return 0;
}