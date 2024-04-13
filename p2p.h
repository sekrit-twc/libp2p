#ifndef P2P_H_
#define P2P_H_

#include <cstddef>
#include <cstdint>
#include <climits>
#include <type_traits>
#ifdef P2P_SIMD
#include <typeinfo>
#endif

#ifdef _WIN32
  #include <stdlib.h> // _byteswap_x
#endif

/**
 * Custom namespace for embedding libp2p in libraries.
 */
#ifdef P2P_USER_NAMESPACE
  #define P2P_NAMESPACE P2P_USER_NAMESPACE
#else
  #define P2P_NAMESPACE p2p
#endif

static_assert(CHAR_BIT == 8, "8-bit char required");


namespace P2P_NAMESPACE {

struct little_endian_t {}; /**< Tag type for little endian. */
struct big_endian_t {};    /**< Tag type for big endian. */

// Endian detection.
#ifdef _WIN32
  #define P2P_LITTLE_ENDIAN
#elif defined(__BYTE_ORDER__)
  #if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
	#define P2P_BIG_ENDIAN
  #elif __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
	#define P2P_LITTLE_ENDIAN
  #endif
#endif

/** Tag type for native endian. */
using native_endian_t =
#if defined(P2P_BIG_ENDIAN)
  big_endian_t;
#elif defined(P2P_LITTLE_ENDIAN)
  little_endian_t;
#else
  #error could not detect native endian;
#endif

#undef P2P_BIG_ENDIAN
#undef P2P_LITTLE_ENDIAN

/** Type selection by native endianness. */
template <class Big, class Little>
using endian_select = std::conditional_t<std::is_same<native_endian_t, big_endian_t>::value, Big, Little>;


/** POD type for loading/storing 24-bit packing formats. */
struct uint24 {
	uint8_t _[3];

	uint24() = default;

	constexpr uint24(uint8_t a, uint8_t b, uint8_t c);

	explicit constexpr uint24(uint32_t val);

	constexpr operator uint32_t() const;
};

/** POD type for loading/storing 48-bit packing formats. */
struct uint48 {
	uint8_t _[6];

	uint48() = default;

	constexpr uint48(uint8_t a, uint8_t b, uint8_t c, uint8_t d, uint8_t e, uint8_t f);

	explicit constexpr uint48(uint64_t val);

	constexpr operator uint64_t() const;
};

static_assert(std::is_standard_layout<uint24>::value, "uint24 must be POD");
static_assert(std::is_standard_layout<uint48>::value, "uint48 must be POD");
static_assert(sizeof(uint24) == 3, "uint24 must not have padding");
static_assert(sizeof(uint48) == 6, "uint48 must not have padding");

/**
 * Native integer type corresponding to packed type.
 */
template <class T>
struct numeric_type {
	typedef T type;
};

template <> struct numeric_type<uint24> { typedef uint32_t type; };
template <> struct numeric_type<uint48> { typedef uint64_t type; };

template <class T>
using numeric_type_t = typename numeric_type<T>::type;


/**
 * Color channel constants.
 */
enum {
	C_Y = 0,   /**< Luma */
	C_U = 1,   /**< Chroma Cb */
	C_V = 2,   /**< Chroma Cr */
	C_R = 0,   /**< Red */
	C_G = 1,   /**< Green */
	C_B = 2,   /**< Blue */
	C_A = 3,   /**< Alpha */
	C__ = 0xFF /**< Padding bits */
};


/** Initialize channel mask (i.e. u8[4]) from four values. */
constexpr uint32_t mask(uint8_t a, uint8_t b, uint8_t c, uint8_t d)
{
	return static_cast<uint32_t>(a) | (static_cast<uint32_t>(b) << 8) | (static_cast<uint32_t>(c) << 16) | (static_cast<uint32_t>(d) << 24);
}

/** Initialize channel mask (i.e. u8[4]) by broadcasting. */
constexpr uint32_t mask(uint8_t x) { return mask(x, x, x, x); }

/**
 * Packing format descriptor.
 *
 * The descriptor is used by {@ref packed_to_planar} and {@ref planar_to_packed}
 * to convert data types. Packed to planar conversion follows the algorithm:
 *
 *   1. Load a sequence of bytes of type Packed from the packed array.
 *   2. Swap endian on the Packed value if Endian is not the native endian tag.
 *   3. Cast the value to the corresponding native type.
 *   4. For each component in ComponentMask, extract a bitfield of type Planar
 *      starting at the offset indicated in ShiftMask and length in DepthMask.
 *   5. If the component is {@ref C__}, the bits are discarded. Otherwise, the
 *      planar array corresponding to ComponentMask is written and incremented.
 *
 * Planar to packed conversion operates in reverse:
 *
 *   1. Default-initialize a value of native type corresponding to Packed.
 *   2. For each component in ComponentMask, read and increment a value of type
 *      Planar from the planar array corresponding to the component.
 *   3. Write the component into a bitfield of length indicated in DepthMask to
 *      the offset in ShiftMask.
 *   4. Cast the value to the Packed type.
 *   5. Swap endian on the Packed value if Endian is not the native endian tag,
 *      then write the value into the packed array.
 *
 * @tparam Planar native integer type holding the planar data. Must be the same
 *         for all channels
 * @tparam Packed POD type for loading/storing the packed data. If not a native
 *         type, a specialization and conversion operator must be provided for
 *         {@ref numeric_type}, and an overload for {@ref endian_swap}
 * @tparam Endian endian tag
 * @tparam PelPerPack number of luma samples covered in one packed word
 * @tparam Subsampling log2 horizontal subsampling factor (e.g. 1 for 4:2:2)
 * @tparam ComponentMask order of color components in packed word
 * @tparam ShiftMask bit offset of each component in ComponentMask
 * @tparam DepthMask bit length of each component in ComponentMask
 */
template <
	class Planar,
	class Packed,
	class Endian,
	unsigned PelPerPack,
	unsigned Subsampling,
	uint32_t ComponentMask,
	uint32_t ShiftMask,
	uint32_t DepthMask>
struct pack_traits {
	static_assert(std::is_trivial<Planar>::value, "must be POD");
	static_assert(std::is_trivial<Packed>::value, "must be POD");

	typedef Planar planar_type;
	typedef Packed packed_type;
	typedef Endian endian;

	static const unsigned pel_per_pack = PelPerPack;
	static const unsigned subsampling = Subsampling;

	static const uint32_t component_mask = ComponentMask;
	static const uint32_t shift_mask = ShiftMask;
	static const uint32_t depth_mask = DepthMask;
};

namespace detail {
template <class T>
static constexpr size_t bit_size = sizeof(T) * CHAR_BIT;
}

/**
 * Helper template for defining 4:4:4 packing formats.
 *
 * The literature describes 4:4:4 formats such as RGB and RGBA as containing a
 * pixel per machine word. This implies that the first component (e.g. R in RGB)
 * occurs at the lower memory address in a big-endian format and the higher
 * address in little-endian formats.
 *
 * @tparam Planar native integer type holding the planar data
 * @tparam Packed POD type corresponding to one packed pixel
 * @tparam ComponentMask order of components, e.g.
 *   RGBA = mask(C_R, C_G, C_B, C_A)
 * @tparam Padding padding bits in LSB of each component when packed
 */
template <class Planar, class Packed, uint32_t ComponentMask, unsigned Padding = 0>
using byte_packed_444_be = pack_traits<
	Planar, Packed, big_endian_t, 1, 0, ComponentMask,
	mask(3, 2, 1, 0) * detail::bit_size<Planar> + mask(Padding),
	mask(detail::bit_size<Planar>) - mask(Padding)>;

/**
 * @see byte_packed_444_be
 */
template <class Planar, class Packed, uint32_t ComponentMask, unsigned Padding = 0>
using byte_packed_444_le = pack_traits<
	Planar, Packed, little_endian_t, 1, 0, ComponentMask,
	mask(3, 2, 1, 0) * detail::bit_size<Planar> + mask(Padding),
	mask(detail::bit_size<Planar>) - mask(Padding)>;

/**
 * Helper template for defining YUY2-like 4:2:2 packings.
 *
 * Unlike RGB formats, the literature describes YUY2 formats as being composed
 * of one machine word per component. Thus, the first component (e.g. Y in YUYV)
 * always occurs at the lower memory address.
 *
 * @tparam Planar native integer type holding the planar data
 * @tparam Packed POD type corresponding to two luma samples and one chroma pair
 * @tparam ComponentMask order of components, e.g.
 *   YUYV = mask(C_Y, C_U, C_Y, C_V)
 * @tparam Padding padding bits in LSB of each component when packed
 */
template <class Planar, class Packed, uint32_t ComponentMask, unsigned Padding = 0>
using byte_packed_422_be = pack_traits<
	Planar, Packed, big_endian_t, 2, 1, ComponentMask,
	mask(3, 2, 1, 0) * detail::bit_size<Planar> + mask(Padding),
	mask(detail::bit_size<Planar>) - mask(Padding)>;

/**
 * @see byte_packed_422_be
 */
template <class Planar, class Packed, uint32_t ComponentMask, unsigned Padding = 0>
using byte_packed_422_le = pack_traits<
	Planar, Packed, little_endian_t, 2, 1, ComponentMask,
	mask(0, 1, 2, 3) * detail::bit_size<Planar> + mask(Padding),
	mask(detail::bit_size<Planar>) - mask(Padding)>;

/**
 * Helper template for defining semi-planar (NV) 4:2:2 and 4:2:0 formats.
 *
 * The literature describes NV-like formats as containing both chroma
 * components in one machine word, with Cr in the most significant position.
 *
 * @tparam Planar native integer type holding the planar data
 * @tparam Packed POD type corresponding to one chroma pair
 * @tparam Padding padding bits in LSB of each component when packed
 */
template <class Planar, class Packed, unsigned Padding = 0>
using byte_packed_nv422_be = pack_traits<
	Planar, Packed, big_endian_t, 2, 1, mask(C__, C__, C_V, C_U),
	mask(0, 0, 1, 0) * detail::bit_size<Planar> + mask(Padding),
	mask(detail::bit_size<Planar>) - mask(Padding)>;

/**
 * @see byte_packed_nv_be
 */
template <class Planar, class Packed, unsigned Padding = 0>
using byte_packed_nv422_le = pack_traits<
	Planar, Packed, little_endian_t, 2, 1, mask(C__, C__, C_V, C_U),
	mask(0, 0, 1, 0) * detail::bit_size<Planar> + mask(Padding),
	mask(detail::bit_size<Planar>) - mask(Padding)>;


// vvv Predefined formats follow. vvv

// 24-bit RGB formats.
using packed_rgb24_be = byte_packed_444_be<uint8_t, uint24, mask(C__, C_R, C_G, C_B)>; /**< rgb24be, a.k.a. RGB */
using packed_rgb24_le = byte_packed_444_le<uint8_t, uint24, mask(C__, C_R, C_G, C_B)>; /**< rgb24le, a.k.a. BGR */
using packed_rgb24 = endian_select<packed_rgb24_be, packed_rgb24_le>;

// 32-bit RGBA formats.
using packed_argb32_be = byte_packed_444_be<uint8_t, uint32_t, mask(C_A, C_R, C_G, C_B)>; /** argb32be, a.k.a. ARGB */
using packed_argb32_le = byte_packed_444_le<uint8_t, uint32_t, mask(C_A, C_R, C_G, C_B)>; /** argb32le, a.k.a. BGRA */
using packed_argb32 = endian_select<packed_argb32_be, packed_argb32_le>;

using packed_rgba32_be = byte_packed_444_be<uint8_t, uint32_t, mask(C_R, C_G, C_B, C_A)>; /** rgba32be, a.k.a. RGBA */
using packed_rgba32_le = byte_packed_444_le<uint8_t, uint32_t, mask(C_R, C_G, C_B, C_A)>; /** rgba32le, a.k.a. ABGR */
using packed_rgba32 = endian_select<packed_rgba32_be, packed_rgba32_le>;

// 48-bit RGB formats.
using packed_rgb48_be = byte_packed_444_be<uint16_t, uint48, mask(C__, C_R, C_G, C_B)>; /**< rgb48be */
using packed_rgb48_le = byte_packed_444_le<uint16_t, uint48, mask(C__, C_R, C_G, C_B)>; /**< rgb48le */
using packed_rgb48 = endian_select<packed_rgb48_be, packed_rgb48_le>;

using packed_bgr48_be = byte_packed_444_be<uint16_t, uint48, mask(C__, C_B, C_G, C_R)>; /**< bgr48be */
using packed_bgr48_le = byte_packed_444_le<uint16_t, uint48, mask(C__, C_B, C_G, C_R)>; /**< bgr48le */
using packed_bgr48 = endian_select<packed_bgr48_be, packed_bgr48_le>;

// 64-bit RGBA formats.
using packed_argb64_be = byte_packed_444_be<uint16_t, uint64_t, mask(C_A, C_R, C_G, C_B)>; /**< argb64be */
using packed_argb64_le = byte_packed_444_le<uint16_t, uint64_t, mask(C_A, C_R, C_G, C_B)>; /**< argb64le */
using packed_argb64 = endian_select<packed_argb64_be, packed_argb64_le>;

using packed_rgba64_be = byte_packed_444_be<uint16_t, uint64_t, mask(C_R, C_G, C_B, C_A)>; /**< rgba64be */
using packed_rgba64_le = byte_packed_444_le<uint16_t, uint64_t, mask(C_R, C_G, C_B, C_A)>; /**< rgba64le */
using packed_rgba64 = endian_select<packed_rgba64_be, packed_rgba64_le>;

using packed_abgr64_be = byte_packed_444_be<uint16_t, uint64_t, mask(C_A, C_B, C_G, C_R)>; /**< abgr64be */
using packed_abgr64_le = byte_packed_444_le<uint16_t, uint64_t, mask(C_A, C_B, C_G, C_R)>; /**< abgr64le */
using packed_abgr64 = endian_select<packed_abgr64_be, packed_abgr64_le>;

using packed_bgra64_be = byte_packed_444_be<uint16_t, uint64_t, mask(C_B, C_G, C_R, C_A)>; /**< bgra64be */
using packed_bgra64_le = byte_packed_444_le<uint16_t, uint64_t, mask(C_B, C_G, C_R, C_A)>; /**< bgra64le */
using packed_bgra64 = endian_select<packed_bgra64_be, packed_bgra64_le>;

// YUVA formats.
using packed_ayuv_be = packed_argb32_be; /** ayuv32be, a.k.a. AYUV */
using packed_ayuv_le = packed_argb32_le; /** ayuv32le, a.k.a. VUYA */
using packed_ayuv = packed_argb32;

using packed_y412_be = byte_packed_444_be<uint16_t, uint64_t, mask(C_A, C_V, C_Y, C_U), 4>; /**< y412be */
using packed_y412_le = byte_packed_444_le<uint16_t, uint64_t, mask(C_A, C_V, C_Y, C_U), 4>; /**< y412le, a.k.a. Y412 */
using packed_y412 = endian_select<packed_y412_be, packed_y412_le>;

using packed_y416_be = byte_packed_444_be<uint16_t, uint64_t, mask(C_A, C_V, C_Y, C_U)>; /**< y416be */
using packed_y416_le = byte_packed_444_le<uint16_t, uint64_t, mask(C_A, C_V, C_Y, C_U)>; /**< y416le, a.k.a. Y416 */
using packed_y416 = endian_select<packed_y416_be, packed_y416_le>;

// RGB30 formats.
using packed_rgb30_be = pack_traits<
	uint16_t, uint32_t, big_endian_t, 1, 0, mask(C_A, C_R, C_G, C_B), mask(30, 20, 10, 0), mask(2, 10, 10, 10)>; /**< rgb30be */
using packed_rgb30_le = pack_traits<
	uint16_t, uint32_t, little_endian_t, 1, 0, mask(C_A, C_R, C_G, C_B), mask(30, 20, 10, 0), mask(2, 10, 10, 10)>; /**< rgb30le, a.k.a. A2R10G10B10 */
using packed_rgb30 = endian_select<packed_rgb30_be, packed_rgb30_le>;

using packed_y410_be = pack_traits<
	uint16_t, uint32_t, big_endian_t, 1, 0, mask(C_A, C_V, C_Y, C_U), mask(30, 20, 10, 0), mask(2, 10, 10, 10)>; /**< y410be */
using packed_y410_le = pack_traits<
	uint16_t, uint32_t, little_endian_t, 1, 0, mask(C_A, C_V, C_Y, C_U), mask(30, 20, 10, 0), mask(2, 10, 10, 10)>; /**< y410le, a.k.a. Y410 */
using packed_y410 = endian_select<packed_y410_be, packed_y410_le>;

// YUYV formats.
using packed_yuy2 = byte_packed_422_be<uint8_t, uint32_t, mask(C_Y, C_U, C_Y, C_V)>; /**< yuy2, a.k.a. yuyv */

using packed_y210_be = byte_packed_422_be<uint16_t, uint64_t, mask(C_Y, C_U, C_Y, C_V), 6>; /**< y210be */
using packed_y210_le = byte_packed_422_le<uint16_t, uint64_t, mask(C_Y, C_U, C_Y, C_V), 6>; /**< y210le, a.k.a. Y210 */
using packed_y210 = endian_select<packed_y210_be, packed_y210_le>;

using packed_y212_be = byte_packed_422_be<uint16_t, uint64_t, mask(C_Y, C_U, C_Y, C_V), 4>; /**< y212be */
using packed_y212_le = byte_packed_422_le<uint16_t, uint64_t, mask(C_Y, C_U, C_Y, C_V), 4>; /**< y212le, a.k.a. Y212 */
using packed_y212 = endian_select<packed_y210_be, packed_y210_le>;

using packed_y216_be = byte_packed_422_be<uint16_t, uint64_t, mask(C_Y, C_U, C_Y, C_V)>; /**< y216be */
using packed_y216_le = byte_packed_422_le<uint16_t, uint64_t, mask(C_Y, C_U, C_Y, C_V)>; /**< y216le, a.k.a. Y216 */
using packed_y216 = endian_select<packed_y216_be, packed_y216_le>;

// UYVY formats.
using packed_uyvy = byte_packed_422_be<uint8_t, uint32_t, mask(C_U, C_Y, C_V, C_Y)>; /**< uyvy */

using packed_v216_be = byte_packed_422_be<uint16_t, uint64_t, mask(C_U, C_Y, C_V, C_Y)>; /**< v216be */
using packed_v216_le = byte_packed_422_le<uint16_t, uint64_t, mask(C_U, C_Y, C_V, C_Y)>; /**< v216le, a.k.a. v216 */
using packed_v216 = endian_select<packed_v216_be, packed_v216_le>;

// 8-bit NV formats.
using packed_nv12_be = byte_packed_nv422_be<uint8_t, uint16_t>; /**< nv12be, a.k.a. NV21 */
using packed_nv12_le = byte_packed_nv422_le<uint8_t, uint16_t>; /**< nv12le, a.k.a. NV12 */
using packed_nv12 = endian_select<packed_nv12_be, packed_nv12_le>;

using packed_nv16_be = packed_nv12_be; /**< nv16be, a.k.a. NV61 */
using packed_nv16_le = packed_nv12_le; /**< nv16le, a.k.a. NV16 */
using packed_nv16 = packed_nv12;

// 10-bit NV formats.
using packed_p210_be = byte_packed_nv422_be<uint16_t, uint32_t, 6>; /**< p210be */
using packed_p210_le = byte_packed_nv422_le<uint16_t, uint32_t, 6>; /**< p210le, a.k.a. P210 */
using packed_p210 = endian_select<packed_p210_be, packed_p210_le>;

using packed_p010_be = packed_p210_be; /**< p010be */
using packed_p010_le = packed_p210_le; /**< p010le, a.k.a. P010 */
using packed_p010 = packed_p210;

// 12-bit NV formats.
using packed_p212_be = byte_packed_nv422_be<uint16_t, uint32_t, 4>; /**< p210be */
using packed_p212_le = byte_packed_nv422_le<uint16_t, uint32_t, 4>; /**< p210le, a.k.a. P212 */
using packed_p212 = endian_select<packed_p212_be, packed_p212_le>;

using packed_p012_be = packed_p212_be; /**< p010be */
using packed_p012_le = packed_p212_le; /**< p010le, a.k.a. P012 */
using packed_p012 = packed_p212;

// 16-bit NV formats.
using packed_p216_be = byte_packed_nv422_be<uint16_t, uint32_t>; /**< p216be */
using packed_p216_le = byte_packed_nv422_le<uint16_t, uint32_t>; /**< p216le, a.k.a. P216 */
using packed_p216 = endian_select<packed_p216_be, packed_p216_le>;

using packed_p016_be = packed_p216_be; /**< p016be */
using packed_p016_le = packed_p216_le; /**< p016le, a.k.a. P016 */
using packed_p016 = packed_p216;

// Special formats.
struct packed_v210_be {}; /**< v210be */;
struct packed_v210_le {}; /**< v210le, a.k.a. v210 */;
using packed_v210 = endian_select<packed_v210_be, packed_v210_le>;

// ^^^ End of predefined formats. ^^^


#ifdef P2P_SIMD
namespace detail {
typedef void (*unpack_func)(const void *, void * const *, unsigned, unsigned);
typedef void (*pack_func)(const void * const *, void *, unsigned, unsigned);
}
#endif // P2P_SIMD

/**
 * Convert packed to planar data.
 *
 * @tparam Traits packing format definition
 */
template <class Traits>
class packed_to_planar {
	typedef typename Traits::planar_type planar_type;
	typedef typename Traits::packed_type packed_type;
	typedef numeric_type_t<packed_type> numeric_type;

	typedef typename Traits::endian endian;

#ifdef P2P_SIMD
	static detail::unpack_func s_delegate;
#endif

	static planar_type extract_component(numeric_type x, unsigned c);

	static void unpack_impl(const void *src, void * const dst[4], unsigned left, unsigned right);
public:
	/**
	 * Unpack one scanline.
	 *
	 * @param src pointer to packed scanline
	 * @param dst pointer to planar scanlines, in R-G-B-A or Y-U-V-A order
	 * @param left first pixel to process
	 * @param right last pixel to process
	 */
	static void unpack(const void *src, void * const dst[4], unsigned left, unsigned right)
	{
#ifdef P2P_SIMD
		s_delegate(src, dst, left, right);
#else
		unpack_impl(src, dst, left, right);
#endif
	}
};

/**
 * Convert planar to packed data.
 *
 * @tparam Traits packing format definition
 * @tparam AlphaOneFill initialize alpha channel to all-ones if not provided
 */
template <class Traits, bool AlphaOneFill = true>
class planar_to_packed {
	typedef typename Traits::planar_type planar_type;
	typedef typename Traits::packed_type packed_type;
	typedef numeric_type_t<packed_type> numeric_type;

	typedef typename Traits::endian endian;

#ifdef P2P_SIMD
	static detail::pack_func s_delegate;
#endif

	static numeric_type align_component(planar_type x, unsigned c);

	static void pack_impl(const void * const src[4], void *dst, unsigned left, unsigned right);
public:
	/**
	 * Pack one scanline.
	 *
	 * @param src pointer to planar scanlines, in R-G-B-A or Y-U-V-A order
	 * @param dst pointer to packed scanline
	 * @param left first pixel to process
	 * @param right last pixel to process
	 */
	static void pack(const void * const src[4], void *dst, unsigned left, unsigned right)
	{
#ifdef P2P_SIMD
		s_delegate(src, dst, left, right);
#else
		pack_impl(src, dst, left, right);
#endif
	}
};

// Specializations for v210.
template <>
class packed_to_planar<packed_v210_be> {
public:
	static void unpack(const void *src, void * const dst[4], unsigned left, unsigned right);
};
template <>
class packed_to_planar<packed_v210_le> {
public:
	static void unpack(const void *src, void * const dst[4], unsigned left, unsigned right);
};
template <>
class planar_to_packed<packed_v210_be, false> {
public:
	static void pack(const void * const src[4], void *dst, unsigned left, unsigned right);
};
template <>
class planar_to_packed<packed_v210_be, true> {
public:
	static void pack(const void * const src[4], void *dst, unsigned left, unsigned right);
};
template <>
class planar_to_packed<packed_v210_le, false> {
public:
	static void pack(const void * const src[4], void *dst, unsigned left, unsigned right);
};
template <>
class planar_to_packed<packed_v210_le, true> {
public:
	static void pack(const void * const src[4], void *dst, unsigned left, unsigned right);
};

} // namespace p2p


// Implementation details follow.
namespace P2P_NAMESPACE {

namespace detail {

static constexpr bool is_be = std::is_same<big_endian_t, native_endian_t>::value;

// Extract byte by significance.
template <class T>
constexpr uint8_t get_u8(T x, unsigned idx)
{
	return static_cast<uint8_t>(x >> (idx * 8));
}

// Construct u32 from bytes by significance.
constexpr uint32_t make_u32(uint8_t a, uint8_t b, uint8_t c, uint8_t d)
{
	return static_cast<uint32_t>(a) | (static_cast<uint32_t>(b) << 8) | (static_cast<uint32_t>(c) << 16) | (static_cast<uint32_t>(d) << 24);
}

// Construct u64 from bytes by significance.
constexpr uint64_t make_u64(uint8_t a, uint8_t b, uint8_t c, uint8_t d, uint8_t e, uint8_t f, uint8_t g, uint8_t h)
{
	return static_cast<uint64_t>(make_u32(a, b, c, d)) | (static_cast<uint64_t>(make_u32(e, f, g, h)) << 32);
}

inline uint16_t endian_swap(uint16_t x)
{
#ifdef _WIN32
	return _byteswap_ushort(x);
#else
	return __builtin_bswap16(x);
#endif
}

inline uint32_t endian_swap(uint32_t x)
{
#ifdef _WIN32
	return _byteswap_ulong(x);
#else
	return __builtin_bswap32(x);
#endif
}

inline uint64_t endian_swap(uint64_t x)
{
#ifdef _WIN32
	return _byteswap_uint64(x);
#else
	return __builtin_bswap64(x);
#endif
}

constexpr uint24 endian_swap(uint24 x)
{
	return{ x._[2], x._[1], x._[0] };
}

constexpr uint48 endian_swap(uint48 x)
{
	return{ x._[5], x._[4], x._[3], x._[2], x._[1], x._[0] };
}

template <class Endian, class T, std::enable_if_t<std::is_same<Endian, native_endian_t>::value> * = nullptr>
T convert_endian(T x) { return x; }

template <class Endian, class T, std::enable_if_t<!std::is_same<Endian, native_endian_t>::value> * = nullptr>
T convert_endian(T x) { return endian_swap(x); }

} // namespace detail


constexpr uint24::uint24(uint8_t a, uint8_t b, uint8_t c) : _{ a, b, c } {}
constexpr uint24::uint24(uint32_t val) :
	_{
		detail::get_u8(val, detail::is_be ? 2 : 0),
		detail::get_u8(val, detail::is_be ? 1 : 1),
		detail::get_u8(val, detail::is_be ? 0 : 2)
	}
{}

constexpr uint24::operator uint32_t() const
{
	return detail::is_be ? detail::make_u32(_[2], _[1], _[0], 0) : detail::make_u32(_[0], _[1], _[2], 0);
}

constexpr uint48::uint48(uint8_t a, uint8_t b, uint8_t c, uint8_t d, uint8_t e, uint8_t f) : _{ a, b, c, d, e, f } {}
constexpr uint48::uint48(uint64_t val) :
	_{
		detail::get_u8(val, detail::is_be ? 5 : 0),
		detail::get_u8(val, detail::is_be ? 4 : 1),
		detail::get_u8(val, detail::is_be ? 3 : 2),
		detail::get_u8(val, detail::is_be ? 2 : 3),
		detail::get_u8(val, detail::is_be ? 1 : 4),
		detail::get_u8(val, detail::is_be ? 0 : 5)
	}
{}

constexpr uint48::operator uint64_t() const
{
	return detail::is_be
		? detail::make_u64(_[5], _[4], _[3], _[2], _[1], _[0], 0, 0)
		: detail::make_u64(_[0], _[1], _[2], _[3], _[4], _[5], 0, 0);
}

#ifdef P2P_SIMD
namespace detail {

unpack_func search_unpack_func(const std::type_info &ti);
pack_func search_pack_func(const std::type_info &ti, bool alpha_one_fill);

template <class Traits>
unpack_func search_unpack_func(unpack_func default_func)
{
	unpack_func func = search_unpack_func(typeid(Traits));
	return func ? func : default_func;
}

template <class Traits, bool AlphaOneFill>
pack_func search_pack_func(pack_func default_func)
{
	pack_func func = search_pack_func(typeid(Traits), AlphaOneFill);
	return func ? func : default_func;
}

} // namespace detail

template <class Traits>
detail::unpack_func packed_to_planar<Traits>::s_delegate = detail::search_unpack_func<Traits>(packed_to_planar::unpack_impl);
template <class Traits, bool AlphaOneFill>
detail::pack_func planar_to_packed<Traits, AlphaOneFill>::s_delegate = detail::search_pack_func<Traits, AlphaOneFill>(planar_to_packed::pack_impl);
#endif // P2P_SIMD

namespace detail {
constexpr uint8_t mask_get(uint32_t mask, unsigned idx) { return get_u8(mask, idx); }

constexpr bool mask_contains(uint32_t mask, unsigned val)
{
	return mask_get(mask, 0) == val || mask_get(mask, 1) == val || mask_get(mask, 2) == val || mask_get(mask, 3) == val;
}
} // namespace detail

template <class Traits>
typename packed_to_planar<Traits>::planar_type packed_to_planar<Traits>::extract_component(numeric_type x, unsigned c)
{
	numeric_type lsb_mask = ~static_cast<numeric_type>(0) >> (detail::bit_size<numeric_type> -detail::mask_get(Traits::depth_mask, c));
	return static_cast<planar_type>((x >> detail::mask_get(Traits::shift_mask, c)) & lsb_mask);
}

template <class Traits>
void packed_to_planar<Traits>::unpack_impl(const void *src, void * const dst[4], unsigned left, unsigned right)
{
	const packed_type *src_p = static_cast<const packed_type *>(src);
	planar_type *dst_p[4] = {
		static_cast<planar_type *>(dst[0]), static_cast<planar_type *>(dst[1]), static_cast<planar_type *>(dst[2]), static_cast<planar_type *>(dst[3])
	};
	bool have_alpha = dst[C_A] != nullptr;

	// Adjust pointers.
	src_p += left / Traits::pel_per_pack;
	dst_p[0] += detail::mask_contains(Traits::component_mask, 0) ? left : 0;
	dst_p[1] += detail::mask_contains(Traits::component_mask, 1) ? (left >> Traits::subsampling) : 0;
	dst_p[2] += detail::mask_contains(Traits::component_mask, 2) ? (left >> Traits::subsampling) : 0;
	dst_p[3] += (detail::mask_contains(Traits::component_mask, 3) && have_alpha) ? left : 0;

#define P2P_COMPONENT_ENABLED(c) ((detail::mask_get(Traits::component_mask, c) != C__) && (detail::mask_get(Traits::component_mask, c) != C_A || have_alpha))
	for (unsigned i = left; i < right; i += Traits::pel_per_pack) {
		numeric_type x = detail::convert_endian<endian>(*src_p++);

		if (P2P_COMPONENT_ENABLED(0))
			*dst_p[detail::mask_get(Traits::component_mask, 0)]++ = extract_component(x, 0);
		if (P2P_COMPONENT_ENABLED(1))
			*dst_p[detail::mask_get(Traits::component_mask, 1)]++ = extract_component(x, 1);
		if (P2P_COMPONENT_ENABLED(2))
			*dst_p[detail::mask_get(Traits::component_mask, 2)]++ = extract_component(x, 2);
		if (P2P_COMPONENT_ENABLED(3))
			*dst_p[detail::mask_get(Traits::component_mask, 3)]++ = extract_component(x, 3);
	}
#undef P2P_COMPONENT_ENABLED
}

template <class Traits, bool AlphaOneFill>
typename planar_to_packed<Traits, AlphaOneFill>::numeric_type planar_to_packed<Traits, AlphaOneFill>::align_component(planar_type x, unsigned c)
{
	numeric_type lsb_mask = ~static_cast<numeric_type>(0) >> (detail::bit_size<numeric_type> -detail::mask_get(Traits::depth_mask, c));
	return (static_cast<numeric_type>(x) & lsb_mask) << detail::mask_get(Traits::shift_mask, c);
}

template <class Traits, bool AlphaOneFill>
void planar_to_packed<Traits, AlphaOneFill>::pack_impl(const void * const src[4], void *dst, unsigned left, unsigned right)
{
	const planar_type *src_p[4] = {
		static_cast<const planar_type *>(src[0]), static_cast<const planar_type *>(src[1]), static_cast<const planar_type *>(src[2]), static_cast<const planar_type *>(src[3])
	};
	packed_type *dst_p = static_cast<packed_type *>(dst);
	bool have_alpha = src[C_A] != nullptr;

	// Adjust pointers.
	src_p[0] += detail::mask_contains(Traits::component_mask, 0) ? left : 0;
	src_p[1] += detail::mask_contains(Traits::component_mask, 1) ? (left >> Traits::subsampling) : 0;
	src_p[2] += detail::mask_contains(Traits::component_mask, 2) ? (left >> Traits::subsampling) : 0;
	src_p[3] += (detail::mask_contains(Traits::component_mask, 3) && have_alpha) ? left : 0;
	dst_p += left / Traits::pel_per_pack;

#define P2P_COMPONENT_ENABLED(c) ((detail::mask_get(Traits::component_mask, c) != C__) && (detail::mask_get(Traits::component_mask, c) != C_A || have_alpha))
	for (unsigned i = left; i < right; i += Traits::pel_per_pack) {
		numeric_type x = 0;

		if (AlphaOneFill && !have_alpha) {
			if (detail::mask_get(Traits::component_mask, 0) == C_A)
				x |= align_component(~static_cast<planar_type>(0), 0);
			if (detail::mask_get(Traits::component_mask, 1) == C_A)
				x |= align_component(~static_cast<planar_type>(0), 1);
			if (detail::mask_get(Traits::component_mask, 2) == C_A)
				x |= align_component(~static_cast<planar_type>(0), 2);
			if (detail::mask_get(Traits::component_mask, 3) == C_A)
				x |= align_component(~static_cast<planar_type>(0), 3);
		}

		if (P2P_COMPONENT_ENABLED(0))
			x |= align_component(*src_p[detail::mask_get(Traits::component_mask, 0)]++, 0);
		if (P2P_COMPONENT_ENABLED(1))
			x |= align_component(*src_p[detail::mask_get(Traits::component_mask, 1)]++, 1);
		if (P2P_COMPONENT_ENABLED(2))
			x |= align_component(*src_p[detail::mask_get(Traits::component_mask, 2)]++, 2);
		if (P2P_COMPONENT_ENABLED(3))
			x |= align_component(*src_p[detail::mask_get(Traits::component_mask, 3)]++, 3);

		*dst_p++ = detail::convert_endian<endian>(static_cast<packed_type>(x));
	}
}
#undef P2P_COMPONENT_ENABLED
} // namespace p2p

#endif // P2P_H_
