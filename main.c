#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include "p2p_api.h"

static void test_api()
{
	int i;

	puts(__func__);

	for (i = 0; i < p2p_packing_max; ++i) {
		p2p_pack_func pack_ptr = p2p_select_pack_func(i);
		p2p_unpack_func unpack_ptr = p2p_select_unpack_func(i);

		if (!pack_ptr || !unpack_ptr)
			printf("%d pack: %p unpack: %p", i, (void *)pack_ptr, (void *)unpack_ptr);
	}
}

static void test_rgb24_be()
{
	uint8_t src[3][1] = { { 0xA0 }, { 0xB0 }, { 0xC0 } };
	uint8_t dst[3];
	void *src_p[4] = { &src[0], &src[1], &src[2], NULL };

	puts(__func__);

	p2p_select_pack_func(p2p_rgb24_be)((const void * const *)src_p, dst, 0, 1);
	printf("packed: %x %x %x\n", dst[0], dst[1], dst[2]);

	p2p_select_unpack_func(p2p_rgb24_be)(dst, src_p, 0, 1);
	printf("planar: %x %x %x\n", src[0][0], src[1][0], src[2][0]);
}

static void test_rgb24_le()
{
	uint8_t src[3][1] = { { 0xA0 }, { 0xB0 }, { 0xC0 } };
	uint8_t dst[3];
	void *src_p[4] = { &src[0], &src[1], &src[2], NULL };

	puts(__func__);

	p2p_select_pack_func(p2p_rgb24_le)((const void * const *)src_p, dst, 0, 1);
	printf("packed: %x %x %x\n", dst[0], dst[1], dst[2]);

	p2p_select_unpack_func(p2p_rgb24_le)(dst, src_p, 0, 1);
	printf("planar: %x %x %x\n", src[0][0], src[1][0], src[2][0]);
}

static void test_rgbx_be()
{
	uint8_t src[3][2] = { { 0xA0, 0xA1 }, { 0xB0, 0xB1 }, { 0xC0, 0xC1 } };
	uint8_t dst[8];
	void *src_p[4] = { &src[0], &src[1], &src[2], NULL };

	puts(__func__);

	p2p_select_pack_func(p2p_argb32_be)((const void * const *)src_p, dst, 0, 2);
	printf("packed: [%x] %x %x %x | [%x] %x %x %x\n", dst[0], dst[1], dst[2], dst[3], dst[4], dst[5], dst[6], dst[7]);

	p2p_select_unpack_func(p2p_argb32_be)(dst, src_p, 0, 2);
	printf("planar: %x %x %x | %x %x %x\n", src[0][0], src[1][0], src[2][0], src[0][1], src[1][1], src[2][1]);
}

static void test_rgb48_be()
{
	uint16_t src[3][1] = { { 0xA0A1 }, { 0xB0B1 }, { 0xC0C1 } };
	uint8_t dst[6];
	void *src_p[4] = { &src[0], &src[1], &src[2], NULL };

	puts(__func__);

	p2p_select_pack_func(p2p_rgb48_be)((const void * const *)src_p, dst, 0, 1);
	printf("packed: %x%02x %x%02x %x%02x\n", dst[0], dst[1], dst[2], dst[3], dst[4], dst[5]);

	p2p_select_unpack_func(p2p_rgb48_be)(dst, src_p, 0, 1);
	printf("planar: %x %x %x\n", src[0][0], src[1][0], src[2][0]);
}

static void test_y410()
{
	uint16_t src[4][1] = { { 0x1A0 }, { 0x1B0 }, { 0x1C0 }, { 0x02 } };
	uint32_t dst[1];
	void *src_p[4] = { &src[0], &src[1], &src[2], &src[3] };

	puts(__func__);

	p2p_select_pack_func(p2p_y410)((const void * const *)src_p, dst, 0, 1);
	printf("packed: %x %x %x %x\n", (dst[0] >> 30) & 0x03, (dst[0] >> 20) & 0x3FF, (dst[0] >> 10) & 0x3FF, dst[0] & 0x3FF);

	p2p_select_unpack_func(p2p_y410)(dst, src_p, 0, 1);
	printf("planar: %x %x %x %x\n", src[0][0], src[1][0], src[2][0], src[3][0]);
}

static void test_uyvy()
{
	uint8_t src[4][2] = {
		{ 0xA0, 0xB0 },
		{ 0x40 },
		{ 0x50 },
	};
	uint8_t dst[4];
	void *src_p[4] = { &src[0], &src[1], &src[2], &src[3] };

	puts(__func__);

	p2p_select_pack_func(p2p_uyvy)((const void * const *)src_p, dst, 0, 2);
	printf("packed: %x %x %x %x\n", dst[0], dst[1], dst[2], dst[3]);

	p2p_select_unpack_func(p2p_uyvy)(dst, src_p, 0, 2);
	printf("planar: %x %x %x %x\n", src[0][0], src[0][1], src[1][0], src[2][0]);
}

int main(int argc, char **argv)
{
	(void)argc;
	(void)argv;

	test_api();

	test_rgb24_be();
	test_rgb24_le();
	test_rgbx_be();
	test_rgb48_be();
	test_y410();
	test_uyvy();

	puts("press any key to continue");
	getc(stdin);

	return 0;
}
