#include "scbs_comms.hh"

#include <string.h>
#include <stdlib.h> // for strtol
#include <math.h> // for macros
#include <stdio.h>
#include <cstring>

#define SCBS_PACKET_DELIM ","
#define SCBS_NUMBERS_BASE 10
#define SCBS_ADDR_BASE 16
#define SCBS_CHECKSUM_BASE 16

#define MIN(a,b) (((a)<(b))?(a):(b))
#define MAX(a,b) (((a)>(b))?(a):(b))

BSPacket::BSPacket() 
    : /*packet_str_len_(0)
    ,*/ is_valid_(false)
    , packet_type_(UNKNOWN)
{
    memset(packet_str_, '\0', kMaxPacketLen);
}

BSPacket::BSPacket(char from_str_buf[kMaxPacketLen]) 
{
    FromString(from_str_buf);
}

/**
 * @brief Populates a BSPacket instance from a packet string. Checks for start and end tokens, calculates
 * checksum, sets packet type.
 * @param[in] from_str_buf Incoming packet string buffer.
*/
void BSPacket::FromString(char from_str_buf[kMaxPacketLen]) {
    is_valid_ = false;
    // packet_str_len_ = from_str_buf_len;
    strncpy(packet_str_, from_str_buf, kMaxPacketLen);

    char * end_token_ptr = strchr(packet_str_, '*');
    if (!end_token_ptr) {
        printf("BSPacket::FromString(): Unable to parse an end token from a packet.\r\n");
        return; // guard against case where end token is not sent
    }
    uint8_t transmitted_checksum = static_cast<uint8_t>(strtol(end_token_ptr+1, NULL, SCBS_CHECKSUM_BASE));
    uint8_t expected_checksum = CalculateChecksum();
    if (expected_checksum != transmitted_checksum) {
        printf("BSPacket::FromString(): Encountered a bad checksum, expected %02x but got %02x.\r\n",
            expected_checksum,
            transmitted_checksum);
        return; // guard against bad checksum
    }

    char strtok_buf[kMaxPacketLen];
    strncpy(strtok_buf, from_str_buf, kMaxPacketFieldLen); // strtok modifies the input string, be safe!
    char * header_str = strtok(strtok_buf, SCBS_PACKET_DELIM);
    header_str++; // ignore the leading '$' sign, this is OK since CalculateChecksum() checks for this.
    for (uint16_t i = 0; i < kNumPacketTypes; i++) { // excludes UNKNOWN type
        if (strcmp(header_str, BSPacket::packet_header_strs[i]) == 0) {
            packet_type_ = static_cast<PacketType_t>(i);
            is_valid_ = true;
            return;
        }
    }

    // Encountered UNKNOWN or other type.
    printf("BSPacket::FromString(): Unable to parse a packet type from %s.\r\n", packet_str_);
    return;
}

/**
 * @brief Blindly blasts the packet into a string buffer. Child classes have smarter implementations
 * that build a string from their fields.
 * @param[out] to_str_buf String buffer to copy packet_str_ into.
*/
uint16_t BSPacket::ToString(char to_str_buf[kMaxPacketLen]) {
    strncpy(to_str_buf, packet_str_, kMaxPacketLen);
    return strlen(packet_str_);
}

/**
 * @brief Writes a packet string containing field values to a string buffer. Used by child classes to encapsulate their
 * contents into packet_str_ with valid formatting.
 * @param[in] packet_contents_str String with everything except the header and tail (does not include tokens).
 * @param[out] to_str_buf String buffer to write completed packet to. Ignored if NULL.
*/
uint16_t BSPacket::PacketizeContents(char packet_contents_str[kMaxPacketContentsLen], char to_str_buf[kMaxPacketLen]) {
    snprintf(packet_str_, kMaxPacketLen-kPacketTailLen, "$%s,%s*",
        BSPacket::packet_header_strs[packet_type_],
        packet_contents_str
    );
    uint16_t checksum = BSPacket::CalculateChecksum();
    char checksum_str[kPacketTailLen];
    snprintf(checksum_str, kPacketTailLen, "%02x", checksum);
    strcat(packet_str_, checksum_str);
    if (to_str_buf != NULL) {
        strncpy(to_str_buf, packet_str_, kMaxPacketLen);
    }
    return strlen(packet_str_);
}

/**
 * @brief Calculates a checksum for the packet string stored in the packet.
 * @retval Calculated checksum.
*/
uint8_t BSPacket::CalculateChecksum() {
    uint8_t checksum = 0;

    // Message is bounded by '$' prefix token and '*' suffix token. Tokens not used in checksum.
    char * start_token_ptr = strchr(packet_str_, '$');
    char * end_token_ptr = strchr(packet_str_, '*');
    if (!start_token_ptr || !end_token_ptr) {
        printf("BSPacket::CalculateChecksum(): Unable to find either start or end token in packet: %s\r\n", packet_str_);
        return 0; // can't find start or end token; invalid!
    } 
    uint16_t packet_start_ind = static_cast<uint16_t>(start_token_ptr - packet_str_) + 1;
    uint16_t checksum_start_ind = static_cast<uint16_t>(end_token_ptr - packet_str_);
    for (uint16_t i = packet_start_ind; i < checksum_start_ind && i < kMaxPacketLen; i++) {
        checksum ^= packet_str_[i];
    }

    return checksum;
}

bool BSPacket::IsValid() {
    return is_valid_;
}

BSPacket::PacketType_t BSPacket::GetPacketType() {
    return packet_type_;
}

/** DISPacket **/

/**
 * @brief Construct DISPacket from values.
*/
DISPacket::DISPacket(uint16_t last_cell_id_in) {
    packet_type_ = BSPacket::DIS;

    // Populate values.
    last_cell_id = last_cell_id_in;

    // Populate packet_str_
    ToString(NULL);
}

/**
 * @brief Construct DISPacket from string.
*/
DISPacket::DISPacket(char from_str_buf[kMaxPacketLen]) {
    packet_type_ = BSPacket::DIS;
    FromString(from_str_buf);
}

/**
 * @brief Fills DISPacket field values from a string buffer.
 * @param[in] from_str_buf String buffer to read.
*/
void DISPacket::FromString(char from_str_buf[kMaxPacketLen]) {
    BSPacket::FromString(from_str_buf);
    if (!is_valid_) {
        printf("DISPacket::FromString(): Failed due to invalid packet.\r\n");
        return;
    }

    char strtok_buf[kMaxPacketLen];

    strncpy(strtok_buf, from_str_buf, kMaxPacketLen); // strtok modifies the input string, be safe!
    char * header_str = strtok(strtok_buf, SCBS_PACKET_DELIM);
    if (strcmp(header_str, "$BSDIS")) {
        // Header is wrong (different packet type).
        printf("DISPacket::FromString(): Failed due to invalid header, expected $BSDIS but got %s.\r\n", header_str);
        return;
    }

    char * last_cell_id_str = strtok(NULL, SCBS_PACKET_DELIM);
    last_cell_id = (uint16_t)strtoul(last_cell_id_str, NULL, SCBS_NUMBERS_BASE);

    is_valid_ = true; // Got here without aborting, good enough!
}

/**
 * @brief Writes a packet string containing field values to a string buffer.
 * @param[out] to_str_buf String buffer to write packet to.
*/
uint16_t DISPacket::ToString(char to_str_buf[kMaxPacketLen]) {
    char contents_str[kMaxPacketContentsLen];
    snprintf(contents_str, kMaxPacketContentsLen, "%d",
        last_cell_id
    );
    BSPacket::PacketizeContents(contents_str, to_str_buf); // Send to parent class for header and tail.
    return strlen(packet_str_);
}

/** MWR Packet**/

/**
 * @brief Construct MWRPacket from values.
 * @param[in] reg_addr_in Value of register to write to (will be converted to string as hex).
 * @param[in] value_in Value to write to reg_addr.
*/
MWRPacket::MWRPacket(uint32_t reg_addr_in, char value_in[kMaxPacketFieldLen]) {
    packet_type_ = MWR;

    // Populate values.
    reg_addr = reg_addr_in;
    memset(value, '\0', kMaxPacketFieldLen);
    strncpy(value, value_in, kMaxPacketFieldLen-1); // make sure to always end with '\0'

    // Populate packet_str_.
    ToString(NULL);
}

/**
 * @brief Construct MWRPacket from string.
 * @param[in] from_str_buf String of the form $BSMWR,<reg_addr>,<value>*<checksum> (e.g. $BSMWR,85,hello*36).
*/
MWRPacket::MWRPacket(char from_str_buf[kMaxPacketLen]) {
    packet_type_ = MWR;
    FromString(from_str_buf);
}

/**
 * @brief Fills DISPacket field values from a string buffer.
 * @param[in] from_str_buf String buffer to read.
*/
void MWRPacket::FromString(char from_str_buf[kMaxPacketLen]) {
    memset(value, '\0', kMaxPacketFieldLen); // make value blank in case stuff fails

    BSPacket::FromString(from_str_buf);
    if (!is_valid_) {
        printf("MWRPacket::FromString(): Failed due to invalid packet.\r\n");
        return;
    }

    char strtok_buf[kMaxPacketLen];

    strncpy(strtok_buf, from_str_buf, kMaxPacketLen); // strtok modifies the input string, be safe!
    char * header_str = strtok(strtok_buf, SCBS_PACKET_DELIM);
    if (strcmp(header_str, "$BSMWR")) {
        // Header is wrong (different packet type).
        printf("MWRPacket::FromString(): Failed due to invalid header, expected $BSMWR but got %s.\r\n", header_str);
        return;
    }

    char * reg_addr_str = strtok(NULL, SCBS_PACKET_DELIM);
    reg_addr = (uint16_t)strtoul(reg_addr_str, NULL, SCBS_ADDR_BASE);

    char * value_str = strtok(NULL, SCBS_PACKET_DELIM);
    char * end_token_ptr = strchr(value_str, '*');
    strncpy(value, value_str, MIN(end_token_ptr - value_str, kMaxPacketFieldLen-1));

    is_valid_ = true; // Got here without aborting, good enough!
}

/**
 * @brief Writes a packet string containing field values to a string buffer.
 * @param[out] to_str_buf String buffer to write packet to.
*/
uint16_t MWRPacket::ToString(char to_str_buf[kMaxPacketLen]) {
    char contents_str[kMaxPacketContentsLen];
    snprintf(contents_str, kMaxPacketContentsLen, "%x,%s",
        reg_addr,
        value
    );
    BSPacket::PacketizeContents(contents_str, to_str_buf); // Send to parent class for header and tail.
    return strlen(packet_str_);
}

/** MRD Packet **/

MRDPacket::MRDPacket(uint32_t reg_addr_in, char values_in[][kMaxPacketFieldLen], uint16_t num_values_in) {
    packet_type_ = MRD;

    // Populate values.
    reg_addr = reg_addr_in;
    for (uint16_t i = 0; i < num_values_in; i++) {
        memset(values[i], '\0', kMaxPacketFieldLen);
        strncpy(values[i], values_in[i], kMaxPacketFieldLen-1); // make sure to always end with '\0'
    }
    num_values = num_values_in;

    // Populate packet_str_.
    ToString(NULL);
}

MRDPacket::MRDPacket(char from_str_buf[kMaxPacketLen]) {
    packet_type_ = MRD;
    FromString(from_str_buf);
}

void MRDPacket::FromString(char from_str_buf[kMaxPacketLen]) {
    for (uint16_t i = 0; i < kMaxNumValues; i++) {
        memset(values[i], '\0', kMaxPacketFieldLen);
    }
    num_values = 0;

    BSPacket::FromString(from_str_buf);
    if (!is_valid_) {
        printf("MRDPacket::FromString(): Failed due to invalid packet.\r\n");
        return;
    }

    is_valid_ = false; // set false again so if something MRD specific goes wrong it shows up
    char strtok_buf[kMaxPacketLen];

    strncpy(strtok_buf, from_str_buf, kMaxPacketLen); // strtok modifies the input string, be safe!
    // Look for end_token_ptr first since we don't know when we'll see it (variable number of values).
    char * end_token_ptr = strchr(strtok_buf, '*'); // don't guard against NULL since already done in CalculateChecksum()
    char * header_str = strtok(strtok_buf, SCBS_PACKET_DELIM);
    if (strcmp(header_str, "$BSMRD")) {
        // Header is wrong (different packet type).
        printf("MRDPacket::FromString(): Failed due to invalid header, expected $BSMRD but got %s.\r\n", header_str);
        return;
    }

    char * reg_addr_str = strtok(NULL, SCBS_PACKET_DELIM);
    reg_addr = (uint16_t)strtoul(reg_addr_str, NULL, SCBS_ADDR_BASE);

    char * value_str = strtok(NULL, SCBS_PACKET_DELIM);
    while(value_str != NULL) {
        if (num_values >= kMaxNumValues) {
            printf("MRDPacket::FromString: Tried to store too many values, got to %d but max is %d.\r\n", num_values+1, kMaxNumValues);
            return; // too many values to store!
        }
        strncpy(values[num_values], value_str, MIN(end_token_ptr - value_str, kMaxPacketFieldLen-1));
        num_values++;
        value_str = strtok(NULL, SCBS_PACKET_DELIM);
    }
    
    is_valid_ = true; // Got here without aborting, good enough!
}

uint16_t MRDPacket::ToString(char to_str_buf[kMaxPacketLen]) {
    char contents_str[kMaxPacketContentsLen];
    snprintf(contents_str, kMaxPacketContentsLen, "%x",
        reg_addr
    );
    for (uint16_t i = 0; i < num_values; i++) {
        if (strlen(contents_str) >= kMaxPacketContentsLen-kMaxPacketFieldLen-1) {
            // Use >= and -1 since leaving room for delimiters and EOF.
            printf("MRDPacket::ToString: Ran out of room for values!\r\n");
            break;
        }
        // Extra char for delimiter (still ends up as kMaxPacketFieldLen since no EOF when neighbors installed).
        char value_str[kMaxPacketFieldLen+1];
        snprintf(value_str, kMaxPacketFieldLen+1, ",%s", values[i]);
        strncat(contents_str, value_str, kMaxPacketFieldLen+1); // +1 for delimiter
    }
    BSPacket::PacketizeContents(contents_str, to_str_buf); // Send to parent class for header and tail.
    return strlen(packet_str_);
}