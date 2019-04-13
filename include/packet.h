// Class to represent a single packet
class packet {
public:
    bool isLast;
    int destPortNo;
    char charPayload;
    int priority;
    int seqNo;

    packet() {
        isLast = false;
        priority=0;
        seqNo=0;
    }    
};
