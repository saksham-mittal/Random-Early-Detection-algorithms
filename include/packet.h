// Class to represent a single packet
class packet {
public:
    bool isLast;
    int destPortNo;
    char charPayload;
    int priority;

    packet() {
        isLast = false;
        priority=0;
    }    
};
