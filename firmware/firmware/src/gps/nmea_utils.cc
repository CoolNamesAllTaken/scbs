#include "nmea_utils.hh"

#include <string.h>
#include <stdlib.h> // for strtol
#include <math.h> // for macros

#define GPS_PACKET_DELIM ","
#define GPS_NUMBERS_BASE 10
#define GPS_CHECKSUM_BASE 16
#define LATITUDE_DEGREES_NUM_DIGITS 2
#define LONGITUDE_DEGREES_NUM_DIGITS 3
#define MINUTES_PER_DEGREE 60.0f

#define MIN(a,b) (((a)<(b))?(a):(b))
#define MAX(a,b) (((a)>(b))?(a):(b))

NMEAPacket::NMEAPacket(
    char packet_str[kMaxPacketLen],
    uint16_t packet_str_len)
    : packet_str_len_(MIN(kMaxPacketLen, packet_str_len))
    , is_valid_(false)
    , packet_type_(UNKNOWN)
{
    strncpy(packet_str_, packet_str, packet_str_len_);

    char * end_token_ptr = strchr(packet_str_, '*');
    if (!end_token_ptr) {
        return; // guard against case where end token is not sent
    }
    uint16_t transmitted_checksum = static_cast<uint16_t>(strtol(end_token_ptr+1, NULL, GPS_CHECKSUM_BASE));
    if (CalculateChecksum() != transmitted_checksum) {
        return; // guard against bad checksum
    }

    char strtok_buf[kMaxPacketLen];
    strncpy(strtok_buf, packet_str, kMaxPacketFieldLen); // strtok modifies the input string, be safe!
    char * header_str = strtok(strtok_buf, GPS_PACKET_DELIM);
    if (strcmp(header_str, "$GPGGA") == 0) {
        packet_type_ = GGA;
    } else if (strcmp(header_str, "$GPGSA") == 0) {
        packet_type_ = GSA;
    } else if (strcmp(header_str, "$GPGSV") == 0) {
        packet_type_ = GSV;
    } else if (strcmp(header_str, "$GPRMC") == 0) {
        packet_type_ = RMC;
    } else if (strcmp(header_str, "$GPVTG") == 0) {
        packet_type_ = VTG;
    } else {
        return; // guard against unknown header type; set packet type as invalid, unknown
    }

    is_valid_ = true;
}

uint8_t NMEAPacket::CalculateChecksum() {
    uint8_t checksum = 0;

    // GPS message is bounded by '$' prefix token and '*' suffix token. Tokens not used in checksum.
    char * start_token_ptr = strchr(packet_str_, '$');
    char * end_token_ptr = strchr(packet_str_, '*');
    if (!start_token_ptr || !end_token_ptr) {
        return 0; // can't find start or end token; invalid!
    } 
    uint16_t packet_start_ind = static_cast<uint16_t>(start_token_ptr - packet_str_) + 1;
    uint16_t checksum_start_ind = static_cast<uint16_t>(end_token_ptr - packet_str_);
    for (uint16_t i = packet_start_ind; i < checksum_start_ind && i < packet_str_len_; i++) {
        checksum ^= packet_str_[i];
    }

    return checksum;
}

bool NMEAPacket::IsValid() {
    return is_valid_;
}

NMEAPacket::PacketType_t NMEAPacket::GetPacketType() {
    return packet_type_;
}

/**
 * @brief Constructor for a $GPGGA packet.
 * @param[in] packet_str C-string buffer with packet contents.
 * @param[in] packet_str_len Number of characters in packet_str.
 */
GGAPacket::GGAPacket(
    char packet_str[kMaxPacketLen],
    uint16_t packet_str_len) 
    : NMEAPacket(packet_str, packet_str_len)
{
    // Make string fields safe to use.
    memset(utc_time_str_, '\0', kMaxPacketFieldLen);
    memset(latitude_str_, '\0', kMaxPacketFieldLen);
    memset(longitude_str_, '\0', kMaxPacketFieldLen);
    
    if (!is_valid_) {
        return; // validity check failed in parent consturctor, abort!
    }

    // Parse GGA message.
    is_valid_ = false; // set this false by default and return if parsing any field fails

    // Parse header.
    char strtok_buf[kMaxPacketLen];
    strncpy(strtok_buf, packet_str, kMaxPacketLen); // strtok modifies the input string, be safe!
    char * header_str = strtok(strtok_buf, GPS_PACKET_DELIM);
    if (strcmp(header_str, "$GPGGA")) {
        // Header is wrong (different packet type).
        return;
    }

    // Parse UTC time.
    strncpy(utc_time_str_, strtok(NULL, GPS_PACKET_DELIM), kMaxPacketFieldLen); // utc time

    // Parse latitude.
    strncpy(latitude_str_, strtok(NULL, GPS_PACKET_DELIM), kMaxPacketFieldLen-1); // latitude as string
    char * latitude_indicator = strtok(NULL, GPS_PACKET_DELIM);
    // Latitude has form DDmm.mm. Convert to degrees!
    char latitude_degrees_str[kMaxPacketFieldLen];
    strncpy(latitude_degrees_str, latitude_str_, LATITUDE_DEGREES_NUM_DIGITS);
    latitude_degrees_str[LATITUDE_DEGREES_NUM_DIGITS] = '\0'; // null-terminate so that strtof doesn't break
    latitude_ = strtof(latitude_degrees_str, NULL);
    latitude_ += (strtof(latitude_str_+LATITUDE_DEGREES_NUM_DIGITS, NULL) / MINUTES_PER_DEGREE);
    if (strcmp(latitude_indicator, "S") == 0) {
        latitude_ *= -1.0f; // South is negative
    } else if (strcmp(latitude_indicator, "N") != 0) {
        return; // Unknown N/S indicator character, packet is invalid.
    }
    strncat(latitude_str_, latitude_indicator, 1); // Add N/S indicator to string.

    // Parse longitude.
    strncpy(longitude_str_, strtok(NULL, GPS_PACKET_DELIM), kMaxPacketFieldLen-1); // longitude as string
    char * longitude_indicator = strtok(NULL, GPS_PACKET_DELIM);
    // Longitude has form DDDmm.mm. Convert to degrees!
    char longitude_degrees_str[kMaxPacketFieldLen];
    strncpy(longitude_degrees_str, longitude_str_, LONGITUDE_DEGREES_NUM_DIGITS);
    longitude_degrees_str[LONGITUDE_DEGREES_NUM_DIGITS] = '\0'; // null-terminate so that strtof doesn't break
    longitude_ = strtof(longitude_degrees_str, NULL);
    longitude_ += (strtof(longitude_str_+LONGITUDE_DEGREES_NUM_DIGITS, NULL) / MINUTES_PER_DEGREE);
    if (strcmp(longitude_indicator, "W") == 0) {
        longitude_ *= -1.0f; // West is negative
    } else if (strcmp(longitude_indicator, "E") != 0) {
        return; // Unknown E/W indicator character, packet is invalid.
    }
    strncat(longitude_str_, longitude_indicator, 1); // Add E/W indicator to string.
    
    // Parse position fix.
    char pos_fix_str_[kMaxPacketFieldLen];
    strncpy(pos_fix_str_, strtok(NULL, GPS_PACKET_DELIM), kMaxPacketFieldLen); // position fix indicator
    int32_t pos_fix = strtol(pos_fix_str_, NULL, GPS_NUMBERS_BASE);
    if (pos_fix >= 0 && pos_fix <= static_cast<int32_t>(PositionFixIndicator_t::DIFFERENTIAL_GPS_FIX)) {
        // Position fix is in the valid enum range.
        pos_fix_ = static_cast<PositionFixIndicator_t>(pos_fix);
    } else {
        // Position fix value is unrecognized. Throw a fit about it.
        pos_fix_ = PositionFixIndicator_t::FIX_NOT_AVAILABLE;
        return;
    }
    
    // Parse satellites.
    satellites_used_ = strtol(strtok(NULL, GPS_PACKET_DELIM), NULL, GPS_NUMBERS_BASE);
    
    // Parse horizontal dilution of position.
    hdop_ = strtof(strtok(NULL, GPS_PACKET_DELIM), NULL); // horizontal dilution of position
    
    // Parse MSL altitude.
    msl_altitude_ = strtof(strtok(NULL, GPS_PACKET_DELIM), NULL); // mean sea level altitude
    if (strcmp(strtok(NULL, GPS_PACKET_DELIM), "M") != 0) {
        // MSL altitude not in meters, abort!
        return;
    }

    // Parse geoidal separation.
    // Reading geoidal separation is a special case, since it might not be populated.
    // If it's populated, it needs to have the right units for the packet to be called valid.
    // If it's not populated, set to 0.0, ignore it, and say the packet is valid if other stuff is OK.
    char * geoidal_separation_str = strtok(NULL, GPS_PACKET_DELIM);
    if (*(geoidal_separation_str-1) == '\0') {
        // strtok did not skip any fields, so geoidal_separation_str was populated.
        geoidal_separation_ = strtof(geoidal_separation_str, NULL); // geoidal separation
        if (strcmp(strtok(NULL, GPS_PACKET_DELIM), "M") != 0) {
            // Geoidal separation not in meters, abort!
            return;
        }
    } else { // previous character is a delimiter or something
        // strtok skipped at least one field, don't try reading geoidal separation.
        geoidal_separation_ = 0.0;
    }
    
    // Skip age of correction data.
    // Skip differential base station ID.

    is_valid_ = true; // Got here without aborting, good enough!
}

void GGAPacket::GetUTCTimeStr(char * str_buf) {
    strcpy(str_buf, utc_time_str_);
}

void GGAPacket::GetLatitudeStr(char * str_buf) {
    strcpy(str_buf, latitude_str_);
}

void GGAPacket::GetLongitudeStr(char * str_buf) {
    strcpy(str_buf, longitude_str_);
}

/**
 * @brief Returns latitude as a float.
 * @retval Latitude, positive if N, negative if S.
 */
float GGAPacket::GetLatitude() {
    return latitude_;
}

/**
 * @brief Returns longitude as a float.
 * @retval Longitude, positive if E, negative if W.
 */
float GGAPacket::GetLongitude() {
    return longitude_;
}

GGAPacket::PositionFixIndicator_t GGAPacket::GetPositionFixIndicator() {
    return pos_fix_;
}

uint16_t GGAPacket::GetSatellitesUsed() {
    return satellites_used_;
}

float GGAPacket::GetHDOP() {
    return hdop_;
}

float GGAPacket::GetMSLAltitude() {
    return msl_altitude_;
}

float GGAPacket::GetGeoidalSeparation() {
    return geoidal_separation_;
}