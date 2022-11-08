#include "gtest/gtest.h"
#include "nmea_utils.hh"
#include <string.h>

#define FLOAT_RELTOL 0.0005f

const uint16_t kStrBufLen = 100;

TEST(NMEAPacketCalculateChecksum, NMEAPacket) {
	// Example sentence from https://www.rfwireless-world.com/Terminology/GPS-sentences-or-NMEA-sentences.html.
	char str_buf[kStrBufLen] = "$GPGGA,161229.487,3723.2475,N,12158.3416,W,1,07,1.0,9.0,M,,,,0000*18";
	ASSERT_EQ(strchr(str_buf, '*'), str_buf + 65);
	NMEAPacket test_packet = NMEAPacket(str_buf, 70);
	uint16_t expected_checksum = strtol("18", NULL, 16);
	// ASSERT_EQ(expected_checksum, test_packet.CalculateChecksum());

	// Example sentence from PA1616S datasheet.
	memset(str_buf, '\0', kStrBufLen);
	strcpy(str_buf, "$GPGGA,091626.000,2236.2791,N,12017.2818,E,1,10,1.00,8.8,M,18.7,M,,*66");
	test_packet = NMEAPacket(str_buf, 71);
	expected_checksum = strtol("66", NULL, 16);
	ASSERT_EQ(expected_checksum, test_packet.CalculateChecksum());
}

TEST(NMEAPacketCalculateChecksum, GGAPacket) {
	// Example sentence from https://www.rfwireless-world.com/Terminology/GPS-sentences-or-NMEA-sentences.html.
	char str_buf[kStrBufLen] = "$GPGGA,161229.487,3723.2475,N,12158.3416,W,1,07,1.0,9.0,M,,,,0000*18";
	GGAPacket test_packet = GGAPacket(str_buf, 70);
	uint16_t expected_checksum = strtol("18", NULL, 16);
	// ASSERT_EQ(expected_checksum, test_packet.CalculateChecksum());

	// Example sentence from PA1616S datasheet.
	memset(str_buf, '\0', kStrBufLen);
	strcpy(str_buf, "$GPGGA,091626.000,2236.2791,N,12017.2818,E,1,10,1.00,8.8,M,18.7,M,,*66");
	test_packet = GGAPacket(str_buf, 71);
	expected_checksum = strtol("66", NULL, 16);
	ASSERT_EQ(expected_checksum, test_packet.CalculateChecksum());
}

TEST(NMEAPacketIsValid, ValidPacket) {
	// Example GPGGA sentence from https://www.rfwireless-world.com/Terminology/GPS-sentences-or-NMEA-sentences.html.
	char str_buf[kStrBufLen] = "$GPGGA,161229.487,3723.2475,N,12158.3416,W,1,07,1.0,9.0,M,,,,0000*18";
	GGAPacket test_packet = GGAPacket(str_buf, 70);
	ASSERT_TRUE(test_packet.IsValid());
}

TEST(NMEAPacketIsValid, BadTransmittedChecksum) {
	// Invalid transmitted checksum.
	char str_buf[kStrBufLen] = "$GPGGA,091626.000,2236.2791,N,12017.2818,E,1,10,1.00,8.8,M,18.7,M,,*67";
	GGAPacket test_packet = GGAPacket(str_buf, 71);
	ASSERT_FALSE(test_packet.IsValid());
}

TEST(NMEAPacketIsValid, InjectedGibberish) {
	// Corrupted packet contents
	char str_buf[kStrBufLen] = "$GPGGA,09162ashgsaggh88586N,12017.2818,E,1,10,1.00,8.8,M,18.7,M,,*66";
	GGAPacket test_packet = GGAPacket(str_buf, 71);
	ASSERT_FALSE(test_packet.IsValid());
}

TEST(NMEAPacketIsValid, NoEndToken) {
	// '*' suffix token not transmitted
	char str_buf[kStrBufLen] = "$GPGGA,09162ashgsaggh88586N,12017.2818,E,1,10,1.00,8.8,M,18.7,M,,66";
	GGAPacket test_packet = GGAPacket(str_buf, 71);
	ASSERT_FALSE(test_packet.IsValid());
}

TEST(NMEAPacketIsValid, NoStartToken) {
	// '$' prefix token not transmitted
	char str_buf[kStrBufLen] = "GPGGA,091626.000,2236.2791,N,12017.2818,E,1,10,1.00,8.8,M,18.7,M,,*66";
	GGAPacket test_packet = GGAPacket(str_buf, 71);
	ASSERT_FALSE(test_packet.IsValid());
}

TEST(NMEAPacketConstructor, SenseHeader) {
    // Example GPGGA sentence from https://www.rfwireless-world.com/Terminology/GPS-sentences-or-NMEA-sentences.html.
	char str_buf[kStrBufLen] = "$GPGGA,161229.487,3723.2475,N,12158.3416,W,1,07,1.0,9.0,M,,,,0000*18";
	GGAPacket test_packet = GGAPacket(str_buf, 70);
	ASSERT_EQ(test_packet.GetPacketType(), NMEAPacket::GGA);
}

TEST(GGAPacketParser, ReadUTCTimeStr) {
    // Example GPGGA sentence from https://www.rfwireless-world.com/Terminology/GPS-sentences-or-NMEA-sentences.html.
	char message_str_buf[kStrBufLen] = "$GPGGA,161229.487,3723.2475,N,12158.3416,W,1,07,1.0,9.0,M,,,,0000*18";
	GGAPacket test_packet = GGAPacket(message_str_buf, 70);
	char field_str_buf[NMEAPacket::kMaxPacketFieldLen];
	test_packet.GetUTCTimeStr(field_str_buf);
	ASSERT_TRUE(test_packet.IsValid());
	ASSERT_EQ(strcmp(field_str_buf, "161229.487"), 0); // strings should match
}

TEST(GGAPacketParser, ReadLatitudeN) {
    // Example GPGGA sentence from https://www.rfwireless-world.com/Terminology/GPS-sentences-or-NMEA-sentences.html.
	char message_str_buf[kStrBufLen] = "$GPGGA,161229.487,3723.2475,N,12158.3416,W,1,07,1.0,9.0,M,,,,0000*18";
	GGAPacket test_packet = GGAPacket(message_str_buf, 70);
	char field_str_buf[NMEAPacket::kMaxPacketFieldLen];
	test_packet.GetLatitudeStr(field_str_buf);
	ASSERT_TRUE(test_packet.IsValid());
	ASSERT_EQ(strcmp(field_str_buf, "3723.2475N"), 0); // strings should match
	ASSERT_NEAR(test_packet.GetLatitude(), 37.3875f, FLOAT_RELTOL);
}

TEST(GGAPacketParser, ReadLatitudeS) {
    // Example GPGGA sentence from https://www.rfwireless-world.com/Terminology/GPS-sentences-or-NMEA-sentences.html.
	char message_str_buf[kStrBufLen] = "$GPGGA,161229.487,3723.2475,S,12158.3416,W,1,07,1.0,9.0,M,,,,0000*05";
	GGAPacket test_packet = GGAPacket(message_str_buf, 70);
	char field_str_buf[NMEAPacket::kMaxPacketFieldLen];
	test_packet.GetLatitudeStr(field_str_buf);
	ASSERT_TRUE(test_packet.IsValid());
	ASSERT_EQ(strcmp(field_str_buf, "3723.2475S"), 0); // strings should match
	ASSERT_NEAR(test_packet.GetLatitude(), -37.3875f, FLOAT_RELTOL);
}

TEST(GGAPacketParser, ReadLongitudeE) {
    // Example GPGGA sentence from https://www.rfwireless-world.com/Terminology/GPS-sentences-or-NMEA-sentences.html.
	char message_str_buf[kStrBufLen] = "$GPGGA,161229.487,3723.2475,N,12158.3416,E,1,07,1.0,9.0,M,,,,0000*0A";
	GGAPacket test_packet = GGAPacket(message_str_buf, 70);
	char field_str_buf[NMEAPacket::kMaxPacketFieldLen];
	test_packet.GetLongitudeStr(field_str_buf);
	ASSERT_TRUE(test_packet.IsValid());
	ASSERT_EQ(strcmp(field_str_buf, "12158.3416E"), 0); // strings should match
	ASSERT_NEAR(test_packet.GetLongitude(), 121.97236f, FLOAT_RELTOL);
}

TEST(GGAPacketParser, ReadLongitudeW) {
    // Example GPGGA sentence from https://www.rfwireless-world.com/Terminology/GPS-sentences-or-NMEA-sentences.html.
	char message_str_buf[kStrBufLen] = "$GPGGA,161229.487,3723.2475,N,12158.3416,W,1,07,1.0,9.0,M,,,,0000*18";
	GGAPacket test_packet = GGAPacket(message_str_buf, 70);
	char field_str_buf[NMEAPacket::kMaxPacketFieldLen];
	test_packet.GetLongitudeStr(field_str_buf);
	ASSERT_TRUE(test_packet.IsValid());
	ASSERT_EQ(strcmp(field_str_buf, "12158.3416W"), 0); // strings should match
	ASSERT_NEAR(test_packet.GetLongitude(), -121.97236f, FLOAT_RELTOL);
}

TEST(GGAPacketParser, ReadPositionFixIndicatorBasic) {
	// Example GPGGA sentence from https://www.rfwireless-world.com/Terminology/GPS-sentences-or-NMEA-sentences.html.
	char message_str_buf[kStrBufLen] = "$GPGGA,161229.487,3723.2475,N,12158.3416,W,1,07,1.0,9.0,M,,,,0000*18";
	GGAPacket test_packet = GGAPacket(message_str_buf, 70);
	ASSERT_TRUE(test_packet.IsValid());
	ASSERT_EQ(test_packet.GetPositionFixIndicator(), GGAPacket::PositionFixIndicator_t::GPS_FIX);
}

TEST(GGAPacketParser, ReadPositionFixIndicatorFixNotAvailable) {
	// Example GPGGA sentence from https://www.rfwireless-world.com/Terminology/GPS-sentences-or-NMEA-sentences.html.
	// Modify fix indicator and put through checksum calculator at https://nmeachecksum.eqth.net/.
	char message_str_buf[kStrBufLen] = "$GPGGA,161229.487,3723.2475,N,12158.3416,W,0,07,1.0,9.0,M,,,,0000*19";
	GGAPacket test_packet = GGAPacket(message_str_buf, 70);
	ASSERT_TRUE(test_packet.IsValid());
	ASSERT_EQ(test_packet.GetPositionFixIndicator(), GGAPacket::PositionFixIndicator_t::FIX_NOT_AVAILABLE);
}

TEST(GGAPacketParser, ReadPositionFixIndicatorFixDifferential) {
	// Example GPGGA sentence from https://www.rfwireless-world.com/Terminology/GPS-sentences-or-NMEA-sentences.html.
	// Modify fix indicator and put through checksum calculator at https://nmeachecksum.eqth.net/.
	char message_str_buf[kStrBufLen] = "$GPGGA,161229.487,3723.2475,N,12158.3416,W,2,07,1.0,9.0,M,,,,0000*1B";
	GGAPacket test_packet = GGAPacket(message_str_buf, 70);
	ASSERT_TRUE(test_packet.IsValid());
	ASSERT_EQ(test_packet.GetPositionFixIndicator(), GGAPacket::PositionFixIndicator_t::DIFFERENTIAL_GPS_FIX);
}

TEST(GGAPacketParser, ReadPositionFixIndicatorFixOther) {
	// Example GPGGA sentence from https://www.rfwireless-world.com/Terminology/GPS-sentences-or-NMEA-sentences.html.
	// Modify fix indicator and put through checksum calculator at https://nmeachecksum.eqth.net/.
	char message_str_buf[kStrBufLen] = "$GPGGA,161229.487,3723.2475,N,12158.3416,W,8,07,1.0,9.0,M,,,,0000*11";
	GGAPacket test_packet = GGAPacket(message_str_buf, 70);
	ASSERT_FALSE(test_packet.IsValid());
	ASSERT_EQ(test_packet.GetPositionFixIndicator(), GGAPacket::PositionFixIndicator_t::FIX_NOT_AVAILABLE);
}

TEST(GGAPacketParser, ReadSatellitesUsed) {
	// Example GPGGA sentence from https://www.rfwireless-world.com/Terminology/GPS-sentences-or-NMEA-sentences.html.
	char message_str_buf[kStrBufLen] = "$GPGGA,161229.487,3723.2475,N,12158.3416,W,1,07,1.0,9.0,M,,,,0000*18";
	GGAPacket test_packet = GGAPacket(message_str_buf, 70);
	ASSERT_TRUE(test_packet.IsValid());
	ASSERT_EQ(test_packet.GetSatellitesUsed(), 7);
}

TEST(GGAPacketParser, ReadHDOP) {
	// Example GPGGA sentence from https://www.rfwireless-world.com/Terminology/GPS-sentences-or-NMEA-sentences.html.
	char message_str_buf[kStrBufLen] = "$GPGGA,161229.487,3723.2475,N,12158.3416,W,1,07,1.0,9.0,M,,,,0000*18";
	GGAPacket test_packet = GGAPacket(message_str_buf, 70);
	ASSERT_TRUE(test_packet.IsValid());
	ASSERT_EQ(test_packet.GetHDOP(), 1.0);
}

TEST(GGAPacketParser, ReadMSLAltitudeBasic) {
	// Example GPGGA sentence from https://www.rfwireless-world.com/Terminology/GPS-sentences-or-NMEA-sentences.html.
	char message_str_buf[kStrBufLen] = "$GPGGA,161229.487,3723.2475,N,12158.3416,W,1,07,1.0,9.0,M,,,,0000*18";
	GGAPacket test_packet = GGAPacket(message_str_buf, 70);
	ASSERT_TRUE(test_packet.IsValid());
	ASSERT_EQ(test_packet.GetMSLAltitude(), 9.0);
}

TEST(GGAPacketParser, ReadMSLAltitudeWrongUnits) {
	// Example GPGGA sentence from https://www.rfwireless-world.com/Terminology/GPS-sentences-or-NMEA-sentences.html.
	// Modify units and put through checksum calculator at https://nmeachecksum.eqth.net/.
	char message_str_buf[kStrBufLen] = "$GPGGA,161229.487,3723.2475,N,12158.3416,W,1,07,1.0,9.0,FT,,,,0000*47";
	GGAPacket test_packet = GGAPacket(message_str_buf, 70);
	ASSERT_FALSE(test_packet.IsValid());
}

TEST(GGAPacketParser, ReadGeoidalSeparationBasic) {
	// Example GPGGA sentence from PA1616S Datasheet pg.14.
	char message_str_buf[kStrBufLen] = "$GPGGA,091626.000,2236.2791,N,12017.2818,E,1,10,1.00,8.8,M,18.7,M,,*66";
	GGAPacket test_packet = GGAPacket(message_str_buf, 70);
	ASSERT_TRUE(test_packet.IsValid());
	ASSERT_EQ(test_packet.GetGeoidalSeparation(), 18.7f);
}

TEST(GGAPacketParser, ReadGeoidalSeparationWrongUnits) {
	// Example GPGGA sentence from PA1616S Datasheet pg.14.
	// Modify units and put through checksum calculator at https://nmeachecksum.eqth.net/.
	char message_str_buf[kStrBufLen] = "$GPGGA,091626.000,2236.2791,N,12017.2818,E,1,10,1.00,8.8,M,18.7,FT,,*39";
	GGAPacket test_packet = GGAPacket(message_str_buf, 70);
	ASSERT_FALSE(test_packet.IsValid());
}