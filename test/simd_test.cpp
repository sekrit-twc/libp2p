#include <array>
#include <cstring>
#include <random>
#include "p2p.h"
#include "simd/p2p_simd.h"

#include "gtest/gtest.h"

// Load P2P again as a scalar-only library.
#undef P2P_SIMD
#undef P2P_NAMESPACE
#undef P2P_H_
#define P2P_USER_NAMESPACE p2p_scalar
#include "p2p.h"

namespace {

template <class T>
struct closest_type {
	typedef T type;
};

template <>
struct closest_type<p2p_scalar::detail::uint24> {
	typedef uint32_t type;
};

template <>
struct closest_type<p2p_scalar::detail::uint48> {
	typedef uint64_t type;
};

template <class T>
using closest_type_t = typename closest_type<T>::type;


template <class Traits, uint64_t PackedMask = typename Traits::packed_type(~0ULL)>
void unpack_test(p2p::detail::unpack_func func)
{
	using planar_type = typename Traits::planar_type;
	using packed_type = typename Traits::packed_type;
	constexpr planar_type guard_value = ~planar_type();

	// 48 pels to cover 16-wide loops with prologue and epilogue.
	std::array<packed_type, (48 >> Traits::subsampling)> packed = {};

	std::mt19937_64 mt;
	std::generate(packed.begin(), packed.end(), [&]()
	{
		return static_cast<packed_type>(static_cast<closest_type_t<packed_type>>(mt()));
	});
	std::for_each(packed.begin(), packed.end(), [](packed_type &x)
	{
		uint64_t val = x;
		val &= PackedMask;
		x = static_cast<packed_type>(static_cast<closest_type_t<packed_type>>(val));
	});

	std::array<planar_type, 48> planar_scalar_ch0 = {};
	std::array<planar_type, (48 >> Traits::subsampling)> planar_scalar_ch1 = {};
	std::array<planar_type, (48 >> Traits::subsampling)> planar_scalar_ch2 = {};
	std::array<planar_type, 48> planar_scalar_ch3 = {};

	planar_scalar_ch0[0] = planar_scalar_ch0[47] = guard_value;
	planar_scalar_ch1[0] = planar_scalar_ch1[47 >> Traits::subsampling] = guard_value;
	planar_scalar_ch2[0] = planar_scalar_ch2[47 >> Traits::subsampling] = guard_value;
	planar_scalar_ch3[0] = planar_scalar_ch3[47] = guard_value;

	{
		void *planar_ptrs[4] = { planar_scalar_ch0.data(), planar_scalar_ch1.data(), planar_scalar_ch2.data(), planar_scalar_ch3.data() };
		p2p_scalar::packed_to_planar<Traits>::unpack(packed.data(), planar_ptrs, 1, 47);
	}

	std::array<planar_type, 48> planar_vector_ch0 = {};
	std::array<planar_type, (48 >> Traits::subsampling)> planar_vector_ch1 = {};
	std::array<planar_type, (48 >> Traits::subsampling)> planar_vector_ch2 = {};
	std::array<planar_type, 48> planar_vector_ch3 = {};

	planar_vector_ch0[0] = planar_vector_ch0[47] = guard_value;
	planar_vector_ch1[0] = planar_vector_ch1[47 >> Traits::subsampling] = guard_value;
	planar_vector_ch2[0] = planar_vector_ch2[47 >> Traits::subsampling] = guard_value;
	planar_vector_ch3[0] = planar_vector_ch3[47] = guard_value;

	{
		void *planar_ptrs[4] = { planar_vector_ch0.data(), planar_vector_ch1.data(), planar_vector_ch2.data(), planar_vector_ch3.data() };
		func(packed.data(), planar_ptrs, 1, 47);
	}

	EXPECT_EQ(planar_scalar_ch0, planar_vector_ch0);
	EXPECT_EQ(planar_scalar_ch1, planar_vector_ch1);
	EXPECT_EQ(planar_scalar_ch2, planar_vector_ch2);
	EXPECT_EQ(planar_scalar_ch3, planar_vector_ch3);

	EXPECT_EQ(guard_value, planar_vector_ch0[0]);
	EXPECT_EQ(guard_value, planar_vector_ch0[47]);
	EXPECT_EQ(guard_value, planar_vector_ch1[0]);
	EXPECT_EQ(guard_value, planar_vector_ch1[47 >> Traits::subsampling]);
	EXPECT_EQ(guard_value, planar_vector_ch2[0]);
	EXPECT_EQ(guard_value, planar_vector_ch2[47 >> Traits::subsampling]);
	EXPECT_EQ(guard_value, planar_vector_ch3[0]);
	EXPECT_EQ(guard_value, planar_vector_ch3[47]);
}

//GTEST_TEST(SIMDTest, test_pack_argb32_le_sse41)
template <class Traits, bool AlphaOneFill>
void pack_test(p2p::detail::pack_func func)
{
	using planar_type = typename Traits::planar_type;
	using packed_type = typename Traits::packed_type;
	constexpr packed_type guard_value = ~closest_type_t<packed_type>();

	// 48 pels to cover 16-wide loops with prologue and epilogue.
	std::array<planar_type, 48> planar_ch0 = {};
	std::array<planar_type, (48 >> Traits::subsampling)> planar_ch1 = {};
	std::array<planar_type, (48 >> Traits::subsampling)> planar_ch2 = {};
	std::array<planar_type, 48> planar_ch3 = {};

	std::mt19937_64 mt;
	std::generate(planar_ch0.begin(), planar_ch0.end(), [&]()
	{
		return static_cast<planar_type>(mt() & ((1ULL << Traits::depth_mask[p2p_scalar::C_R]) - 1));
	});
	std::generate(planar_ch1.begin(), planar_ch1.end(), [&]()
	{
		return static_cast<planar_type>(mt() & ((1ULL << Traits::depth_mask[p2p_scalar::C_G]) - 1));
	});
	std::generate(planar_ch2.begin(), planar_ch2.end(), [&]()
	{
		return static_cast<planar_type>(mt() & ((1ULL << Traits::depth_mask[p2p_scalar::C_B]) - 1));
	});
	std::generate(planar_ch3.begin(), planar_ch3.end(), [&]()
	{
		return static_cast<planar_type>(mt() & ((1ULL << Traits::depth_mask[p2p_scalar::C_A]) - 1));
	});

	std::array<packed_type, 48> packed_scalar_alpha = {};
	std::array<packed_type, 48> packed_scalar_noalpha = {};

	packed_scalar_alpha[0] = guard_value;
	packed_scalar_alpha[47] = guard_value;
	packed_scalar_noalpha[0] = guard_value;
	packed_scalar_noalpha[47] = guard_value;

	{

		void *planar_ptrs[4] = { planar_ch0.data(), planar_ch1.data(), planar_ch2.data(), planar_ch3.data() };
		p2p_scalar::planar_to_packed<Traits, AlphaOneFill>::pack(planar_ptrs, packed_scalar_alpha.data(), 1, 47);

		planar_ptrs[3] = nullptr;
		p2p_scalar::planar_to_packed<Traits, AlphaOneFill>::pack(planar_ptrs, packed_scalar_noalpha.data(), 1, 47);
	}

	std::array<packed_type, 48> packed_vector_alpha = {};
	std::array<packed_type, 48> packed_vector_noalpha = {};

	packed_vector_alpha[0] = guard_value;
	packed_vector_alpha[47] = guard_value;
	packed_vector_noalpha[0] = guard_value;
	packed_vector_noalpha[47] = guard_value;

	{
		void *planar_ptrs[4] = { planar_ch0.data(), planar_ch1.data(), planar_ch2.data(), planar_ch3.data() };
		func(planar_ptrs, packed_vector_alpha.data(), 1, 47);

		planar_ptrs[3] = nullptr;
		func(planar_ptrs, packed_vector_noalpha.data(), 1, 47);
	}

	EXPECT_EQ(packed_scalar_alpha, packed_vector_alpha);
	EXPECT_EQ(packed_scalar_noalpha, packed_vector_noalpha);

	EXPECT_EQ(static_cast<closest_type_t<packed_type>>(guard_value), static_cast<closest_type_t<packed_type>>(packed_vector_alpha[0]));
	EXPECT_EQ(static_cast<closest_type_t<packed_type>>(guard_value), static_cast<closest_type_t<packed_type>>(packed_vector_alpha[47]));
	EXPECT_EQ(static_cast<closest_type_t<packed_type>>(guard_value), static_cast<closest_type_t<packed_type>>(packed_vector_noalpha[0]));
	EXPECT_EQ(static_cast<closest_type_t<packed_type>>(guard_value), static_cast<closest_type_t<packed_type>>(packed_vector_noalpha[47]));
}


#define UNPACK_TEST(format, cpu) \
  GTEST_TEST(SIMDTest, test_unpack_##format##_##cpu) \
  { \
    unpack_test<p2p_scalar::packed_##format>(p2p::simd::unpack_##format##_##cpu); \
  }

#define PACK_TEST(format, cpu) \
  GTEST_TEST(SIMDTest, test_pack_##format##_##cpu) \
  { \
    { \
      SCOPED_TRACE("AlphaZeroFill"); \
      pack_test<p2p_scalar::packed_##format, 0>(p2p::simd::pack_##format##_0_##cpu); \
    } \
    { \
      SCOPED_TRACE("AlphaOneFill"); \
      pack_test<p2p_scalar::packed_##format, 1>(p2p::simd::pack_##format##_1_##cpu); \
    } \
  }

#if defined(__x86_64__) || defined(__i386__) || defined(_M_IX86) || defined(_M_X64)
UNPACK_TEST(argb32_be, sse41)
UNPACK_TEST(argb32_le, sse41)
UNPACK_TEST(rgba32_be, sse41)
UNPACK_TEST(rgba32_le, sse41)

PACK_TEST(argb32_be, sse41)
PACK_TEST(argb32_le, sse41)
PACK_TEST(rgba32_be, sse41)
PACK_TEST(rgba32_le, sse41)
#endif

} // namespace
