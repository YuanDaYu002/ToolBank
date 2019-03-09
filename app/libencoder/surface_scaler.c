#include <stdlib.h>
#include <string.h>

#include "mpi_sys.h"

//#include "hal.h"
#include "surface_scaler.h"


//创建一个suface，调用前需指定sfc->u32Width和sfc->u32Height
//返回surface的对应的虚拟地址

void *create_surface(HLE_SURFACE *sfc)
{
	void *addr = malloc(sfc->u32Width * sfc->u32Height * 2);
	sfc->u32PhyAddr = (int) addr;
	return addr;
}

//销毁surface

void destroy_surface(HLE_SURFACE *sfc, void *addr)
{
	free(addr);
}

//双线性插值，在OSD这种场景下不比邻近插值好多少(ALPHA很难处理)，而且耗CPU很厉害
#if 0
#define __A(v) (((v) >> 15) & 0x1)
#define __R(v) (((v) >> 10) & 0x1F)
#define __G(v) (((v) >> 5) & 0x1F)
#define __B(v) ((v) & 0x1F)
#define __ARGB(a, r, g, b) ((((a)&0x1)<<15)|(((r)&0x1F)<<10)|(((g)&0x1F)<<5)|((b)&0x1F))

static int bilineer_scale(HLE_U16 *dst_bmp, HLE_U32 dst_w, HLE_U32 dst_h,
	HLE_U16 *src_bmp, HLE_U32 src_w, HLE_U32 src_h)
{
	if (src_bmp == NULL || dst_bmp == NULL)
		return -1;

	if (src_w == dst_w && src_h == dst_h) {
		memcpy(dst_bmp, src_bmp, src_w * src_h * 2);
		return 0;
	}

	float w_scale = (float) ((1.0 * src_w) / dst_w);
	float h_scale = (float) ((1.0 * src_h) / dst_h);

	int x, y;
	for (y = 0; y < dst_h; y++) //列
	{
		float src_fy = y * h_scale;
		if (src_fy <= 1e-8)
			src_fy = 0;
		if (src_fy > src_h - 2)
			src_fy = src_h - 2;

		int j = (int) src_fy;
		float v = src_fy - j;

		for (x = 0; x < dst_w; x++) //行
		{
			float src_fx = x * w_scale;
			if (src_fx <= 1e-8)
				src_fx = 0;
			if (src_fx > src_w - 2)
				src_fx = src_w - 2;

			int i = (int) src_fx;
			float u = src_fx - i;

			int v11 = *(src_bmp + src_w * j + i);
			int v12 = *(src_bmp + src_w * (j + 1) + i);
			int v21 = *(src_bmp + src_w * j + i + 1);
			int v22 = *(src_bmp + src_w * (j + 1) + i + 1);

			//f(i+u,j+v) = (1-u)(1-v)f(i,j) + (1-u)vf(i,j+1) + u(1-v)f(i+1,j) + uvf(i+1,j+1)
			int r = (int) ((1 - u)*(1 - v) * __R(v11) + (1 - u) * v * __R(v12)
				+ u * (1 - v) * __R(v21) + u * v * __R(v22));
			if (r > 0x1F)
				r = 0x1F;
			int g = (int) ((1 - u)*(1 - v) * __G(v11) + (1 - u) * v * __G(v12)
				+ u * (1 - v) * __G(v21) + u * v * __G(v22));
			if (g > 0x1F)
				g = 0x1F;

			int b = (int) ((1 - u)*(1 - v) * __B(v11) + (1 - u) * v * __B(v12)
				+ u * (1 - v) * __B(v21) + u * v * __B(v22));
			if (b > 0x1F)
				b = 0x1F;

			float alpha = (1 - u)*(1 - v) * __A(v11) + (1 - u) * v * __A(v12)
				+ u * (1 - v) * __A(v21) + u * v * __A(v22);
			int a = (alpha >= 0.5) ? 1 : 0;

			*(dst_bmp + dst_w * y + x) = __ARGB(a, r, g, b);
		}
	}

	return 0;
}
#undef __A
#undef __R
#undef __G
#undef __B
#undef __ARGB
#endif

#if 1

static int lineer_scale(HLE_U16 *dst_bmp, HLE_U32 dst_w, HLE_U32 dst_h,
	HLE_U16 *src_bmp, HLE_U32 src_w, HLE_U32 src_h)
{
	if (src_bmp == NULL || dst_bmp == NULL)
		return -1;

	if (src_w == dst_w && src_h == dst_h) {
		memcpy(dst_bmp, src_bmp, src_w * src_h * 2);
		return 0;
	}

	float w_scale = (float) ((1.0 * src_w) / dst_w);
	float h_scale = (float) ((1.0 * src_h) / dst_h);

	int x, y;
	for (y = 0; y < dst_h; y++) //列
	{
		int src_y = (int) (y * h_scale);
		if (src_y > src_h - 1) {
			DEBUG_LOG("IMPOSSIBLE src_y %d!\n", src_y);
			src_y = src_h - 1;
		}

		for (x = 0; x < dst_w; x++) //行
		{
			int src_x = (int) (x * w_scale);

			if (src_x > src_w - 1) {
				DEBUG_LOG("IMPOSSIBLE src_x %d!\n", src_x);
				src_x = src_w - 1;
			}

			*(dst_bmp + dst_w * y + x) = *(src_bmp + src_w * src_y + src_x);
		}
	}

	return 0;
}
#endif

//surface缩放操作，把src缩放到dst

int scale_surface(HLE_SURFACE *dst, HLE_SURFACE *src)
{
	return lineer_scale((HLE_U16 *) dst->u32PhyAddr, dst->u32Width, dst->u32Height,
		(HLE_U16 *) src->u32PhyAddr, src->u32Width, src->u32Height);

	//return bilineer_scale((HLE_U16 *)dst->u32PhyAddr, dst->u32Width, dst->u32Height,
	//    (HLE_U16 *)src->u32PhyAddr, src->u32Width, src->u32Height);
}

int scaler_init()
{
	return HLE_RET_OK;
}

