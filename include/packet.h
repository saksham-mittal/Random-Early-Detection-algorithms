// Class to represent a single packet
class packet {
public:
    bool isLast;
    int destPortNo;
    char charPayload;

    packet() {
        isLast = false;
    }    
};
