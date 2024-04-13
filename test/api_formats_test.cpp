#include <array>
#include <cstdint>
#include <cstring>
#include "p2p.h"

#include "gtest/gtest.h"

namespace {

uint8_t operator""_b(unsigned long long x)
{
	return static_cast<uint8_t>(x);
}

uint16_t operator""_w(unsigned long long x)
{
	return static_cast<uint16_t>(x);
}

uint32_t operator""_d(unsigned long long x)
{
	return static_cast<uint32_t>(x);
}

uint64_t operator""_q(unsigned long long x)
{
	return static_cast<uint64_t>(x);
}

p2p::uint24 operator""_d24(unsigned long long x)
{
	return p2p::uint24{ static_cast<uint32_t>(x) };
}

p2p::uint48 operator""_q48(unsigned long long x)
{
	return p2p::uint48{ static_cast<uint64_t>(x) };
}


template <class T>
T to_be(T x)
{
	return p2p::detail::convert_endian<p2p::big_endian_t>(x);
}

uint8_t to_be(uint8_t x) { return x; }


template <class Traits>
void basic_test_case(std::array<typename Traits::planar_type, 4> planar, typename Traits::packed_type packed)
{
	ptrdiff_t planar_offset[4];
	planar_offset[0] = 0;
	planar_offset[1] = planar_offset[0] + Traits::pel_per_pack * sizeof(typename Traits::planar_type);
	planar_offset[2] = planar_offset[1] + (Traits::pel_per_pack >> Traits::subsampling) * sizeof(typename Traits::planar_type);
	planar_offset[3] = planar_offset[2] + (Traits::pel_per_pack >> Traits::subsampling) * sizeof(typename Traits::planar_type);

	typename Traits::packed_type packed_tmp;
	std::array<typename Traits::planar_type, 4> planar_tmp;
	void *planar_tmp_ptrs[4];

	for (unsigned p = 0; p < 4; ++p) {
		planar_tmp_ptrs[p] = reinterpret_cast<uint8_t *>(&planar_tmp) + planar_offset[p];
	}

	SCOPED_TRACE("packed_to_planar");
	std::memcpy(&packed_tmp, &packed, sizeof(packed));
	std::memset(&planar_tmp, 0, sizeof(planar_tmp));
	p2p::packed_to_planar<Traits>::unpack(&packed_tmp, planar_tmp_ptrs, 0, Traits::pel_per_pack);
	EXPECT_EQ(planar, planar_tmp);

	SCOPED_TRACE("planar_to_packed");
	std::memcpy(&planar_tmp, &planar, sizeof(planar));
	std::memset(&packed_tmp, 0, sizeof(packed_tmp));
	p2p::planar_to_packed<Traits>::pack(planar_tmp_ptrs, &packed_tmp, 0, Traits::pel_per_pack);
	EXPECT_EQ(packed, packed_tmp);
}

// a1, a2, a3, a4: materialized as native-endian
// b: materialized as big-endian
#define TEST_(format, a1, a2, a3, a4, b) \
GTEST_TEST(ApiFormatTest, test_##format) \
{ \
	basic_test_case<p2p::packed_##format>( \
		{ a1, a2, a3, a4 }, \
		to_be(b) \
	); \
}

TEST_(rgb24_be, 0x1_b, 0x2_b, 0x3_b, 0x00_b, 0x010203_d24)
TEST_(rgb24_le, 0x1_b, 0x2_b, 0x3_b, 0x00_b, 0x030201_d24)

TEST_(argb32_be, 0x1_b, 0x2_b, 0x3_b, 0x4_b, 0x04010203_d)
TEST_(argb32_le, 0x1_b, 0x2_b, 0x3_b, 0x4_b, 0x03020104_d)

TEST_(rgb48_be, 0x0102_w, 0x0304_w, 0x0506_w, 0x0000_w, 0x010203040506_q48)
TEST_(rgb48_le, 0x0102_w, 0x0304_w, 0x0506_w, 0x0000_w, 0x060504030201_q48)

TEST_(argb64_be, 0x0102_w, 0x0304_w, 0x0506_w, 0x0708_w, 0x0708010203040506_q);
TEST_(argb64_le, 0x0102_w, 0x0304_w, 0x0506_w, 0x0708_w, 0x0605040302010807_q)

TEST_(rgba32_be, 0x01_b, 0x02_b, 0x03_b, 0x04_b, 0x01020304_d);
TEST_(rgba32_le, 0x01_b, 0x02_b, 0x03_b, 0x04_b, 0x04030201_d);

TEST_(rgba64_be, 0x0102_w, 0x0304_w, 0x0506_w, 0x0708_w, 0x0102030405060708_q);
TEST_(rgba64_le, 0x0102_w, 0x0304_w, 0x0506_w, 0x0708_w, 0x0807060504030201_q);

TEST_(abgr64_be, 0x0102_w, 0x0304_w, 0x0506_w, 0x0708_w, 0x0708050603040102_q);
TEST_(abgr64_le, 0x0102_w, 0x0304_w, 0x0506_w, 0x0708_w, 0x0201040306050807_q);

TEST_(bgr48_be, 0x0102_w, 0x0304_w, 0x0506_w, 0x0000_w, 0x050603040102_q48);
TEST_(bgr48_le, 0x0102_w, 0x0304_w, 0x0506_w, 0x0000_w, 0x020104030605_q48);

TEST_(bgra64_be, 0x0102_w, 0x0304_w, 0x0506_w, 0x0708_w, 0x0506030401020708_q);
TEST_(bgra64_le, 0x0102_w, 0x0304_w, 0x0506_w, 0x0708_w, 0x0807020104030605_q);

TEST_(rgb30_be, 0x0101_w, 0x0202_w, 0x0303_w, 0x03_w, 0b11'0100000001'1000000010'1100000011_d); // 0b11010000'00011000'00001011'00000011
TEST_(rgb30_le, 0x0101_w, 0x0202_w, 0x0303_w, 0x03_w, 0b00000011'00001011'00011000'11010000_d)

TEST_(y410_be, 0x0101_w, 0x0202_w, 0x0303_w, 0x03_w, 0b11'1100000011'0100000001'1000000010_d); // 0b11110000'00110100'00000110'00000010
TEST_(y410_le, 0x0101_w, 0x0202_w, 0x0303_w, 0x03_w, 0b00000010'00000110'00110100'11110000_d)

TEST_(y412_be, 0x0102_w, 0x0304_w, 0x0506_w, 0x0708_w, 0x7080506010203040_q);
TEST_(y412_le, 0x0102_w, 0x0304_w, 0x0506_w, 0x0708_w, 0x4030201060508070_q);

TEST_(y416_be, 0x0102_w, 0x0304_w, 0x0506_w, 0x0708_w, 0x0708050601020304_q);
TEST_(y416_le, 0x0102_w, 0x0304_w, 0x0506_w, 0x0708_w, 0x0403020106050807_q);

TEST_(yuy2, 0x01, 0x02, 0x10, 0x20, 0x01100220_d);
TEST_(uyvy, 0x01, 0x02, 0x10, 0x20, 0x10012002_d);

TEST_(y210_be, 0x010A_w, 0x020B_w, 0x0301_w, 0x0302_w, 0x4280C04082C0C080_q);
TEST_(y210_le, 0x010A_w, 0x020B_w, 0x0301_w, 0x0302_w, 0x804240C0C08280C0_q);

TEST_(y212_be, 0x010A_w, 0x020B_w, 0x0301_w, 0x0302_w, 0x10A0301020B03020_q);
TEST_(y212_le, 0x010A_w, 0x020B_w, 0x0301_w, 0x0302_w, 0xA0101030B0202030_q);

TEST_(y216_be, 0x0102_w, 0x0304_w, 0x0506_w, 0x0708_w, 0x0102050603040708_q);
TEST_(y216_le, 0x0102_w, 0x0304_w, 0x0506_w, 0x0708_w, 0x0201060504030807_q);

TEST_(v216_be, 0x0102_w, 0x0304_w, 0x0506_w, 0x0708_w, 0x0506010207080304_q);
TEST_(v216_le, 0x0102_w, 0x0304_w, 0x0506_w, 0x0708_w, 0x0605020108070403_q);

TEST_(nv12_be, 0x00_b, 0x00_b, 0x01_b, 0x02_b, 0x0201_w);
TEST_(nv12_le, 0x00_b, 0x00_b, 0x01_b, 0x02_b, 0x0102_w);

TEST_(p010_be, 0x0000_w, 0x0000_w, 0x010A_w, 0x020B_w, 0x82C04280_d);
TEST_(p010_le, 0x0000_w, 0x0000_w, 0x010A_w, 0x020B_w, 0x8042C082_d);

TEST_(p012_be, 0x0000_w, 0x0000_w, 0x010A_w, 0x020B_w, 0x20B010A0_d);
TEST_(p012_le, 0x0000_w, 0x0000_w, 0x010A_w, 0x020B_w, 0xA010B020_d);

TEST_(p016_be, 0x0000_w, 0x0000_w, 0x0102_w, 0x0304_w, 0x03040102_d);
TEST_(p016_le, 0x0000_w, 0x0000_w, 0x0102_w, 0x0304_w, 0x02010403_d);

} // namespace
