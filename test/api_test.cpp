#include <array>
#include <cstdint>
#include <cstring>
#include <memory>
#include <vector>
#include "p2p_api.h"
#include "gtest/gtest.h"

namespace {

GTEST_TEST(APITest, test_simple)
{
	constexpr unsigned R = 0, G = 1, B = 2, A = 3;

	unsigned w = 631;
	unsigned h = 480;
	size_t stride_packed = 632 * 4;
	size_t stride_planar = 640;
	size_t plane_size_planar = stride_planar * h;

	std::vector<uint8_t> planar(stride_planar * h * 4);
	std::vector<uint8_t> packed(stride_packed * h);

	auto checksum = [](unsigned i, unsigned j, unsigned c)
	{
		return ((i & 0x7) << 5) | ((j & 0x7) << 2) | (c & 0x3);
	};

	// Write test pattern.
	for (unsigned i = 0; i < h; ++i) {
		for (unsigned j = 0; j < w; ++j) {
			uint8_t val = ((i & 0x7) << 5) | ((j & 0x7) << 2);
			uint8_t r = val | 0;
			uint8_t g = val | 1;
			uint8_t b = val | 2;
			uint8_t a = val | 3;

			packed[i * stride_packed + j * 4 + 0] = checksum(i, j, A);
			packed[i * stride_packed + j * 4 + 1] = checksum(i, j, R);
			packed[i * stride_packed + j * 4 + 2] = checksum(i, j, G);
			packed[i * stride_packed + j * 4 + 3] = checksum(i, j, B);

			planar[plane_size_planar * 0 + i * stride_planar + j] = checksum(i, j, R);
			planar[plane_size_planar * 1 + i * stride_planar + j] = checksum(i, j, G);
			planar[plane_size_planar * 2 + i * stride_planar + j] = checksum(i, j, B);
			planar[plane_size_planar * 3 + i * stride_planar + j] = checksum(i, j, A);
		}
	}

	{
		SCOPED_TRACE("unpack_frame");
		std::vector<uint8_t> planar_tmp(planar.size());
		p2p_buffer_param param{};
		param.src[0] = packed.data();
		param.dst[0] = planar_tmp.data() + 0 * plane_size_planar;
		param.dst[1] = planar_tmp.data() + 1 * plane_size_planar;
		param.dst[2] = planar_tmp.data() + 2 * plane_size_planar;
		param.dst[3] = planar_tmp.data() + 3 * plane_size_planar;
		param.src_stride[0] = stride_packed;
		param.dst_stride[0] = stride_planar;
		param.dst_stride[1] = stride_planar;
		param.dst_stride[2] = stride_planar;
		param.dst_stride[3] = stride_planar;
		param.width = w;
		param.height = h;
		param.packing = p2p_argb32_be;

		p2p_unpack_frame(&param, 0);

		// Verify.
		for (unsigned i = 0; i < h; ++i) {
			for (unsigned j = 0; j < w; ++j) {
				uint8_t r = planar_tmp[plane_size_planar * 0 + i * stride_planar + j];
				uint8_t g = planar_tmp[plane_size_planar * 1 + i * stride_planar + j];
				uint8_t b = planar_tmp[plane_size_planar * 2 + i * stride_planar + j];
				uint8_t a = planar_tmp[plane_size_planar * 3 + i * stride_planar + j];

				EXPECT_EQ(checksum(i, j, R), r);
				EXPECT_EQ(checksum(i, j, G), g);
				EXPECT_EQ(checksum(i, j, B), b);
				EXPECT_EQ(checksum(i, j, A), a);

				ASSERT_FALSE(::testing::Test::HasFailure()) << '(' << i << ',' << j << ')';
			}
		}
	}

	{
		SCOPED_TRACE("pack_frame");
		std::vector<uint8_t> packed_tmp(packed.size());
		p2p_buffer_param param{};
		param.src[0] = planar.data() + 0 * plane_size_planar;
		param.src[1] = planar.data() + 1 * plane_size_planar;
		param.src[2] = planar.data() + 2 * plane_size_planar;
		param.src[3] = planar.data() + 3 * plane_size_planar;
		param.dst[0] = packed_tmp.data();
		param.src_stride[0] = stride_planar;
		param.src_stride[1] = stride_planar;
		param.src_stride[2] = stride_planar;
		param.src_stride[3] = stride_planar;
		param.dst_stride[0] = stride_packed;
		param.width = w;
		param.height = h;
		param.packing = p2p_argb32_be;

		p2p_pack_frame(&param, 0);

		// Verify.
		for (unsigned i = 0; i < h; ++i) {
			for (unsigned j = 0; j < w; ++j) {
				uint8_t a = packed_tmp[i * stride_packed + j * 4 + 0];
				uint8_t r = packed_tmp[i * stride_packed + j * 4 + 1];
				uint8_t g = packed_tmp[i * stride_packed + j * 4 + 2];
				uint8_t b = packed_tmp[i * stride_packed + j * 4 + 3];

				EXPECT_EQ(checksum(i, j, R), r);
				EXPECT_EQ(checksum(i, j, G), g);
				EXPECT_EQ(checksum(i, j, B), b);
				EXPECT_EQ(checksum(i, j, A), a);

				ASSERT_FALSE(::testing::Test::HasFailure()) << '(' << i << ',' << j << ')';
			}
		}
	}
}

GTEST_TEST(APITest, test_nv_noskip)
{
	std::array<uint16_t, 6> planar = {
		0x0101, 0x0102,
		0x0101, 0x0102,
		0x0201,
		0x0301
	};
	std::array<uint8_t, 12> packed = {
		0x40, 0x40, 0x40, 0x80,
		0x40, 0x40, 0x40, 0x80,
		0xC0, 0x40, 0x80, 0x40,
	};

	{
		SCOPED_TRACE("unpack_frame");
		std::array<uint16_t, 6> planar_tmp{};
		p2p_buffer_param param{};
		param.src[0] = &packed;
		param.src[1] = &packed[4 * sizeof(uint16_t)];
		param.dst[0] = &planar_tmp[0];
		param.dst[1] = &planar_tmp[4];
		param.dst[2] = &planar_tmp[5];
		param.src_stride[0] = 2 * sizeof(uint16_t);
		param.src_stride[1] = 2 * sizeof(uint16_t);
		param.dst_stride[0] = 2 * sizeof(uint16_t);
		param.dst_stride[1] = 1 * sizeof(uint16_t);
		param.dst_stride[2] = 1 * sizeof(uint16_t);
		param.width = 2;
		param.height = 2;
		param.packing = p2p_p010_be;

		p2p_unpack_frame(&param, 0);
		ASSERT_EQ(planar, planar_tmp);
	}

	{
		SCOPED_TRACE("pack_frame");
		std::array<uint8_t, 12> packed_tmp{};
		p2p_buffer_param param{};
		param.src[0] = &planar[0];
		param.src[1] = &planar[4];
		param.src[2] = &planar[5];
		param.dst[0] = &packed_tmp;
		param.dst[1] = &packed_tmp[4 * sizeof(uint16_t)];
		param.src_stride[0] = 2 * sizeof(uint16_t);
		param.src_stride[1] = 1 * sizeof(uint16_t);
		param.src_stride[2] = 1 * sizeof(uint16_t);
		param.dst_stride[0] = 2 * sizeof(uint16_t);
		param.dst_stride[1] = 2 * sizeof(uint16_t);
		param.width = 2;
		param.height = 2;
		param.packing = p2p_p010_be;

		p2p_pack_frame(&param, 0);
		ASSERT_EQ(packed, packed_tmp);
	}
}

GTEST_TEST(APITest, test_nv_skip)
{
	std::array<uint16_t, 6> planar = {
		0x0101, 0x0102,
		0x0101, 0x0102,
		0x0201,
		0x0301
	};
	std::array<uint8_t, 12> packed = {
		0x40, 0x40, 0x40, 0x80,
		0x40, 0x40, 0x40, 0x80,
		0xC0, 0x40, 0x80, 0x40,
	};

	{
		SCOPED_TRACE("unpack_frame");
		std::array<uint16_t, 6> planar_tmp{};
		p2p_buffer_param param{};
		param.src[1] = &packed[4 * sizeof(uint16_t)];
		param.dst[1] = &planar_tmp[4];
		param.dst[2] = &planar_tmp[5];
		param.src_stride[1] = 2 * sizeof(uint16_t);
		param.dst_stride[1] = 1 * sizeof(uint16_t);
		param.dst_stride[2] = 1 * sizeof(uint16_t);
		param.width = 2;
		param.height = 2;
		param.packing = p2p_p010_be;

		p2p_unpack_frame(&param, P2P_SKIP_UNPACKED_PLANES);

		for (unsigned i = 0; i < 4; ++i) {
			ASSERT_EQ(0, planar_tmp[i]);
		}
		for (unsigned i = 4; i < 6; ++i) {
			ASSERT_EQ(planar[i], planar_tmp[i]);
		}
	}

	{
		SCOPED_TRACE("pack_frame");
		std::array<uint8_t, 12> packed_tmp{};
		p2p_buffer_param param{};
		param.src[1] = &planar[4];
		param.src[2] = &planar[5];
		param.dst[1] = &packed_tmp[4 * sizeof(uint16_t)];
		param.src_stride[1] = 1 * sizeof(uint16_t);
		param.src_stride[2] = 1 * sizeof(uint16_t);
		param.dst_stride[1] = 2 * sizeof(uint16_t);
		param.width = 2;
		param.height = 2;
		param.packing = p2p_p010_be;

		p2p_pack_frame(&param, P2P_SKIP_UNPACKED_PLANES);

		for (unsigned i = 0; i < 4 * sizeof(uint16_t); ++i) {
			ASSERT_EQ(0, packed_tmp[i]);
		}
		for (unsigned i = 4 * sizeof(uint16_t); i < 12; ++i) {
			ASSERT_EQ(packed[i], packed_tmp[i]);
		}
	}
}

GTEST_TEST(APITest, test_one_fill)
{
	uint8_t planar[3 * 3] = { 0x11, 0x12, 0x13, 0x21, 0x22, 0x23, 0x31, 0x32, 0x33 };
	uint8_t packed[3 * 4];

	p2p_buffer_param params{};
	params.src[0] = planar;
	params.src[1] = planar + 3;
	params.src[2] = planar + 6;
	params.dst[0] = packed;
	params.src_stride[0] = 3;
	params.src_stride[1] = 3;
	params.src_stride[2] = 3;
	params.dst_stride[0] = 12;
	params.width = 3;
	params.height = 1;
	params.packing = p2p_argb32_be;

	std::memset(packed, 0xAA, sizeof(packed));
	p2p_pack_frame(&params, 0);
	ASSERT_EQ(0, packed[0]);
	ASSERT_EQ(0, packed[4]);
	ASSERT_EQ(0, packed[8]);

	std::memset(packed, 0xAA, sizeof(packed));
	p2p_pack_frame(&params, P2P_ALPHA_SET_ONE);
	ASSERT_EQ(0xFF, packed[0]);
	ASSERT_EQ(0xFF, packed[4]);
	ASSERT_EQ(0xFF, packed[8]);
}

} // namespace
