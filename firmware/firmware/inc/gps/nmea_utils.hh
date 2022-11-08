#ifndef _NMEA_UTILS_HH_
#define _NMEA_UTILS_HH_

#include <stdint.h>

class NMEAPacket {
public:
    static const uint16_t kMaxPacketLen = 200;

    static const uint16_t kMaxPacketFieldLen = 20;
    // static const uint16_t kMessageIDStrLen = 7;
    // static const uint16_t kUTCTimeStrLen = 11;
    // static const uint16_t kLatitudeStrLen = 11;
    // static const uint16_t kLongitudeStrLen = 12;

    typedef enum {
        UNKNOWN = 0,
        GGA, // time, position, fix type data
        GSA, // gps solution information
        GSV, // satellite information
        RMC, // minimum navigation information
        VTG // course and speed info
    } PacketType_t;

    NMEAPacket(char packet_str[kMaxPacketLen], uint16_t packet_str_len);

    // Static utility functions.
    static uint32_t UTCTimeStrToUint(char utc_time_str[kMaxPacketFieldLen]);

    // Public interface functions.
    uint8_t CalculateChecksum();
    bool IsValid();
    PacketType_t GetPacketType();
protected:
    char packet_str_[kMaxPacketLen];
    uint16_t packet_str_len_;

    bool is_valid_;
    PacketType_t packet_type_ = UNKNOWN;
};

/**
 * Packet containing time, position, and fix type data.
 */
class GGAPacket : public NMEAPacket {
public:
    typedef enum {
        FIX_NOT_AVAILABLE = 0,
        GPS_FIX = 1,
        DIFFERENTIAL_GPS_FIX = 2
    } PositionFixIndicator_t;

    GGAPacket(char packet_str[kMaxPacketLen], uint16_t packet_str_len); // constructor

    void GetUTCTimeStr(char * str_buf);
    void GetLatitudeStr(char * str_buf); // includes "N" or "S" suffix
    void GetLongitudeStr(char * str_buf); // includes "E" ow "W" suffix
    float GetLatitude();
    float GetLongitude();
    GGAPacket::PositionFixIndicator_t GetPositionFixIndicator();
    uint16_t GetSatellitesUsed();
    float GetHDOP();
    float GetMSLAltitude(); // [meters]
    float GetGeoidalSeparation();

private:
    // uint16_t utc_time_hr_;
    // uint16_t utc_time_min_;
    // uint16_t utc_time_sec_;
    // uint16_t utc_time_ms_;

    char utc_time_str_[kMaxPacketFieldLen];
    char latitude_str_[kMaxPacketFieldLen];
    char longitude_str_[kMaxPacketFieldLen];
    float latitude_;
    float longitude_;
    PositionFixIndicator_t pos_fix_ = FIX_NOT_AVAILABLE;
    uint16_t satellites_used_ = 0;
    float hdop_;
    float msl_altitude_;
    float geoidal_separation_;
};

#endif /* _NMEA_UTILS_HH_ */