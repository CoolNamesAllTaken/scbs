#ifndef _SCBS_COMMS_HH_
#define _SCBS_COMMS_HH_

#include <stdint.h>
#include <cstring>

class BSPacket {
public:
    static const uint16_t kMaxPacketLen = 200;
    static const uint16_t kMaxPacketFieldLen = 20;
    static const uint16_t kPacketHeaderLen = 6; // $BSDIS, no EOS
    static const uint16_t kPacketTailLen = 3; // *FC, no EOS
    static const uint16_t kMaxPacketContentsLen = kMaxPacketLen - kPacketHeaderLen - kPacketTailLen;

    static const uint16_t kNumPacketTypes = 6;

    typedef enum {
        DIS = 0, // cell discover
        MRD, // multi read
        MWR, // multi write
        SRD, // single read
        SWR, // single write
        // MRS, // multi response
        SRS, // single response
        UNKNOWN
    } PacketType_t;
    static_assert(static_cast<uint16_t>(UNKNOWN) == kNumPacketTypes);

    static constexpr char packet_header_strs[kNumPacketTypes+1][kPacketHeaderLen] = {
        "BSDIS",
        "BSMRD",
        "BSMWR",
        "BSSRD",
        "BSSWR",
        // "BSMRS",
        "BSSRS",
        "?????"
    }; // Note: these must be <= kPacketHeaderLen characters (not including EOS).

    // Overloaded constructors.
    BSPacket();
    BSPacket(char from_str_buf[kMaxPacketLen]);

    // Public interface functions.
    virtual void FromString(char from_str_buf[kMaxPacketLen]);
    virtual uint16_t ToString(char to_str_buf[kMaxPacketLen]);
    uint8_t CalculateChecksum();
    bool IsValid();

    PacketType_t GetPacketType();
protected:
    uint16_t PacketizeContents(char packet_contents_str[kMaxPacketContentsLen], char to_str_buf[kMaxPacketLen]);
    
    char packet_str_[kMaxPacketLen];
    // uint16_t packet_str_len_;

    bool is_valid_;
    PacketType_t packet_type_ = UNKNOWN;
};

// Battery Simulator Cell Discover Packet
class DISPacket : public BSPacket {
public:
    DISPacket(uint16_t last_cell_id_in);
    DISPacket(char from_str_buf[kMaxPacketLen]);

    void FromString(char from_str_buf[kMaxPacketLen]);
    uint16_t ToString(char to_str_buf[kMaxPacketLen]);

    uint16_t last_cell_id = 0;
};

// Battery Simulator Multi Write Packet
class MWRPacket : public BSPacket {
public:
    MWRPacket(uint32_t reg_addr_in, char value_in[kMaxPacketFieldLen]);
    MWRPacket(char from_str_buf[kMaxPacketLen]);

    void FromString(char from_str_buf[kMaxPacketLen]);
    uint16_t ToString(char to_str_buf[kMaxPacketLen]);

    uint32_t reg_addr = 0x00u;
    char value[kMaxPacketFieldLen];
};

// Battery Simulator Multi Read Packet
class MRDPacket : public BSPacket {
public:
    static const uint16_t kMaxNumValues = 20;

    MRDPacket(uint32_t reg_addr_in, char values_in[][kMaxPacketFieldLen], uint16_t num_values_in);
    MRDPacket(char from_str_buf[kMaxPacketLen]);

    void FromString(char from_str_buf[kMaxPacketLen]);
    uint16_t ToString(char to_str_buf[kMaxPacketLen]);

    uint32_t reg_addr = 0x00u;
    char values[kMaxNumValues][kMaxPacketFieldLen];
    uint16_t num_values;
};

#endif /* _SCBS_COMMS_HH_ */