#include "gtest/gtest.h"
#include "scbs_comms.hh"
#include <string.h>

#define FLOAT_RELTOL 0.0005f

TEST(BSPacketChecksum, BSDIS) {
	// Examples generated with NMEA checksum calculator: https://nmeachecksum.eqth.net/
	char str_buf[BSPacket::kMaxPacketLen] = "$BSDIS,0*53";
	BSPacket test_packet = BSPacket(str_buf);
	uint16_t expected_checksum = strtoul("53", NULL, 16);
	ASSERT_EQ(expected_checksum, test_packet.CalculateChecksum());

	memset(str_buf, '\0', BSPacket::kMaxPacketLen);
	strcpy(str_buf, "$BSDIS,48789729587*5F");
	test_packet = BSPacket(str_buf);
	expected_checksum = strtoul("5F", NULL, 16);
	ASSERT_EQ(expected_checksum, test_packet.CalculateChecksum());
}

TEST(BSPacketChecksum, BSSRD) {
	char str_buf[BSPacket::kMaxPacketLen] = "$BSSRD,53,0285*5D";
	BSPacket test_packet = BSPacket(str_buf);
	uint16_t expected_checksum = strtoul("5D", NULL, 16);
	ASSERT_EQ(expected_checksum, test_packet.CalculateChecksum());
}

TEST(BSPacketIsValid, ValidPacket) {
	char str_buf[BSPacket::kMaxPacketLen] = "$BSSRD,53,0285*5D";
	BSPacket test_packet = BSPacket(str_buf);
	ASSERT_TRUE(test_packet.IsValid());
}

TEST(BSPacketIsValid, BadTransmittedChecksum) {
	char str_buf[BSPacket::kMaxPacketLen] = "$BSSRD,53,0285*5C";
	BSPacket test_packet = BSPacket(str_buf);
	ASSERT_FALSE(test_packet.IsValid());
}

TEST(BSPacketIsValid, InjectedGibberish) {
	// Corrupted packet contents
	char str_buf[BSPacket::kMaxPacketLen] = "$BSSRD,53,02asgasgasgaewrrhgeg85*5D";
	BSPacket test_packet = BSPacket(str_buf);
	ASSERT_FALSE(test_packet.IsValid());
}

TEST(BSPacketIsValid, NoEndToken) {
	// '*' suffix token not transmitted
	char str_buf[BSPacket::kMaxPacketLen] = "$BSSRD,53,02855D";
	BSPacket test_packet = BSPacket(str_buf);
	ASSERT_FALSE(test_packet.IsValid());
}

TEST(BSPacketIsValid, NoStartToken) {
	// '$' prefix token not transmitted
	char str_buf[BSPacket::kMaxPacketLen] = "BSSRD,53,0285*5D";
	BSPacket test_packet = BSPacket(str_buf);
	ASSERT_FALSE(test_packet.IsValid());
}

TEST(TestBSPacketConstructor, SenseHeader) {
	char str_buf[BSPacket::kMaxPacketLen] = "$BSSRD,53,0285*5D";
	BSPacket test_packet = BSPacket(str_buf);
	ASSERT_EQ(test_packet.GetPacketType(), BSPacket::SRD);
}

TEST(DISPacketConstructor, Basic) {
	char str_buf[BSPacket::kMaxPacketLen];
	DISPacket packet = DISPacket(53);
	ASSERT_EQ(packet.last_cell_id, 53);
	packet.ToString(str_buf);
	ASSERT_STREQ(str_buf, "$BSDIS,53*65");

	packet = DISPacket((char *)"$BSDIS,1*52\r\n");
	ASSERT_EQ(packet.last_cell_id, 1);
}

TEST(DISPacketConstructor, StringToStringValid) {
	char str_buf[BSPacket::kMaxPacketLen] = "$BSDIS,56*60";
	DISPacket test_packet = DISPacket(str_buf);
	ASSERT_EQ(test_packet.GetPacketType(), BSPacket::DIS);
	ASSERT_TRUE(test_packet.IsValid());
	ASSERT_EQ(test_packet.last_cell_id, 56);

	char str_out_buf[BSPacket::kMaxPacketLen];
	test_packet.ToString(str_out_buf);
	ASSERT_STREQ(str_out_buf, "$BSDIS,56*60");
}

TEST(DISPacketConstructor, StringToStringInvalid) {
	char str_buf[BSPacket::kMaxPacketLen] = "$BSDIS,56safd*60";
	DISPacket test_packet = DISPacket(str_buf);
	ASSERT_EQ(test_packet.GetPacketType(), BSPacket::DIS);
	ASSERT_FALSE(test_packet.IsValid());
	ASSERT_EQ(test_packet.last_cell_id, 0);

	char str_out_buf[BSPacket::kMaxPacketLen];
	test_packet.ToString(str_out_buf);
	ASSERT_STREQ(str_out_buf, "$BSDIS,0*53"); // value of last_cell_id should be unread
}

TEST(MWRPacketConstructor, StringToStringValid) {
	char str_buf[BSPacket::kMaxPacketLen];
	MWRPacket packet = MWRPacket(0x78, (char *)"potatoes");
	ASSERT_EQ(packet.reg_addr, 0x78u);
	ASSERT_STREQ(packet.value, "potatoes");
	packet.ToString(str_buf);
	ASSERT_STREQ(str_buf, "$BSMWR,78,potatoes*51");

	packet = MWRPacket((char *)"$BSMWR,09234,elbowsyay*04\r\n");
	ASSERT_TRUE(packet.IsValid());
	ASSERT_EQ(packet.reg_addr, 0x09234u);
	ASSERT_STREQ(packet.value, "elbowsyay");
}

TEST(MWRPacketConstructor, StringToStringInvalid) {
	MWRPacket packet = MWRPacket((char *)"$BSMWR,09234,elasgahateatht44bowsyay*04\r\n");
	ASSERT_FALSE(packet.IsValid());
	ASSERT_EQ(packet.reg_addr, 0x00u);
	ASSERT_STREQ(packet.value, "");

	char str_buf[BSPacket::kMaxPacketLen];
	packet.ToString(str_buf);
	ASSERT_STREQ(str_buf, "$BSMWR,0,*69");
}

TEST(MWRPacketConstructor, StringTooLong) {
	MWRPacket packet = MWRPacket(
		(char *)"$BSMWR,09234,elasgahateatht44bowsasdggggggggggggeadsgaerwheatrh"
		"taedrshatethaeshgashbashfashrfbgashgbashbasbhggshlasgahateatht44bowsasd"
		"ggggggggggggeadsgaerwheatrhtaedrshatethaeshgashbashfashrfbgashgbashbasb"
		"hggshyayay*04\r\n"
	);
	ASSERT_FALSE(packet.IsValid());
	char str_buf[BSPacket::kMaxPacketLen];
	packet.ToString(str_buf);
	ASSERT_STREQ(str_buf, "$BSMWR,0,*69");
}