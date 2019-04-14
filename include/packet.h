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
    
    packet(const packet &packet2) {
        isLast = packet2.isLast;
        destPortNo = packet2.destPortNo;
        charPayload =packet2.charPayload;
        priority=packet2.priority;
        seqNo=packet2.seqNo;

    }    
};
