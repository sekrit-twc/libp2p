#include <array>
#include <cstdint>
#include <cstring>
#include "p2p.h"

#include "gtest/gtest.h"

namespace {

uint32_t be(uint32_t x)
{
	return p2p::detail::endian_swap<p2p::big_endian_t>(x);
}

GTEST_TEST(V210Test, test_v210_be)
{
	const std::array<uint16_t, 12> planar = {
		0x0101, 0x0102, 0x0103, 0x0104, 0x0105, 0x0106,
		0x0201, 0x0202, 0x0203,
		0x0301, 0x0302, 0x0303,
	};
	const std::array<uint32_t, 4> packed = {
		be(0b00'1100000001'0100000001'1000000001), // V1 Y1 U1 (0b00110000'00010100'00000110'00000001)
		be(0b00'0100000011'1000000010'0100000010), // Y3 U2 Y2 (0b00010000'00111000'00001001'00000010)
		be(0b00'1000000011'0100000100'1100000010), // U3 Y4 V2 (0b00100000'00110100'00010011'00000010)
		be(0b00'0100000110'1100000011'0100000101), // Y6 V3 Y5 (0b00010000'01101100'00001101'00000101)
	};

	std::array<uint16_t, 12> planar_tmp;
	std::array<uint32_t, 4> packed_tmp;
	void *planar_tmp_ptrs[4] = { &planar_tmp[0], &planar_tmp[6], &planar_tmp[9], nullptr };

	SCOPED_TRACE("packed_to_planar");
	std::memcpy(&packed_tmp, &packed, sizeof(packed));
	std::memset(&planar_tmp, 0, sizeof(planar_tmp));
	p2p::packed_to_planar<p2p::packed_v210_be>::unpack(&packed_tmp, planar_tmp_ptrs, 0, 6);
	ASSERT_EQ(planar, planar_tmp);

	SCOPED_TRACE("planar_to_packed");
	std::memcpy(&planar_tmp, &planar, sizeof(planar));
	std::memset(&packed_tmp, 0, sizeof(packed_tmp));
	p2p::planar_to_packed<p2p::packed_v210_be>::pack(planar_tmp_ptrs, &packed_tmp, 0, 6);
	ASSERT_EQ(packed, packed_tmp);
}

GTEST_TEST(V210Test, test_v210_le)
{
	const std::array<uint16_t, 12> planar = {
		0x0101, 0x0102, 0x0103, 0x0104, 0x0105, 0x0106,
		0x0201, 0x0202, 0x0203,
		0x0301, 0x0302, 0x0303,
	};
	const std::array<uint32_t, 4> packed = {
		be(0b00000001'00000110'00010100'00110000), // V1 Y1 U1
		be(0b00000010'00001001'00111000'00010000), // Y3 U2 Y2
		be(0b00000010'00010011'00110100'00100000), // U3 Y4 V2
		be(0b00000101'00001101'01101100'00010000), // Y6 V3 Y5
	};

	std::array<uint16_t, 12> planar_tmp;
	std::array<uint32_t, 4> packed_tmp;
	void *planar_tmp_ptrs[4] = { &planar_tmp[0], &planar_tmp[6], &planar_tmp[9], nullptr };

	SCOPED_TRACE("packed_to_planar");
	std::memcpy(&packed_tmp, &packed, sizeof(packed));
	std::memset(&planar_tmp, 0, sizeof(planar_tmp));
	p2p::packed_to_planar<p2p::packed_v210_le>::unpack(&packed_tmp, planar_tmp_ptrs, 0, 6);
	ASSERT_EQ(planar, planar_tmp);

	SCOPED_TRACE("planar_to_packed");
	std::memcpy(&planar_tmp, &planar, sizeof(planar));
	std::memset(&packed_tmp, 0, sizeof(packed_tmp));
	p2p::planar_to_packed<p2p::packed_v210_le>::pack(planar_tmp_ptrs, &packed_tmp, 0, 6);
	ASSERT_EQ(packed, packed_tmp);
}

} // namespace
