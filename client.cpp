/* 
    To run the client.cpp file:
    g++ client.cpp -o client
    To execute:
    ./client 3542 10
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
#include <iostream>
#include <chrono>

using namespace std;

int main(int argc, char const** argv) {
    if(argc < 3) {
        cout << "Usage ./client <port-no> <burst-size>\n";
        exit(1);
    }
    int burstSize = stoi(argv[2]);
    int sockid;
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
    char charArray[6] = "abcde";
    int ind = 0, burstTime = 0;
    while(1) {
        // Write a character to the socket
        int count = send(sockid, (void*)&charArray[ind], 1, 0);
        if(count < 0) {
            printf("Error on sending.\n");
        }
        ind = (ind + 1) % 5;
        if(burstTime % burstTime == 0) {
            // Sleep for some time after sending burst
            sleep(1);
            burstTime = 0;
        }
        burstTime++;
    }

    return 0;
}