#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/prctl.h>
#include <pthread.h>
#include <linux/input.h>

#include <imp/imp_isp.h>
#include <imp/imp_osd.h>

#include <calibration.h>

#include "bgramapinfo_OSD.h"

extern unsigned char *osd_data;

int drawLineRectShow(IMPRgnHandle handler, int x0, int y0, int x1, int y1, int color, IMPOsdRgnType type, unsigned long long ts)
{
	IMPOSDRgnTimestamp rTs;
	IMPOSDRgnAttr rAttr;
	if (IMP_OSD_GetRgnAttr(handler, &rAttr) < 0) {
		printf("%s(%d):IMP_OSD_GetRgnAttr failed\n", __func__, __LINE__);
		return -1;
	}

	rTs.ts = ts;
	rAttr.type = type;
	rAttr.rect.p0.x = x0 > 1919 ? 1919 : x0;
	rAttr.rect.p0.y = y0 > 1079 ? 1079 : y0;
	rAttr.rect.p1.x = x1 > 1919 ? 1919 : x1;
	rAttr.rect.p1.y = y1 > 1079 ? 1079 : y1;
	rAttr.data.lineRectData.color = color;

	if (IMP_OSD_SetRgnAttrWithTimestamp(handler, &rAttr, &rTs) < 0) {
	/*if (IMP_OSD_SetRgnAttr(handler, &rAttr) < 0) {*/
		printf("%s(%d):IMP_OSD_SetRgnAttrWithTimestamp failed\n", __func__, __LINE__);
		return -1;
	}

	if (IMP_OSD_ShowRgn(handler, 0, 1) < 0) {
		printf("%s(%d): IMP_OSD_ShowRgn failed\n", __func__, __LINE__);
		return -1;
	}

	return 0;
}

int drawPicRectShow(IMPRgnHandle handler, int id, int x0, int y0, unsigned long long ts)
{
	IMPOSDRgnTimestamp rTs;
	IMPOSDRgnAttr rAttr;
	char DataStr[40];
	int len = 0;
	int i = 0, j = 0;
	void *dataData = NULL;
	int fontNum = 10;
	int fontWidth = 16;
	int fontHeight = 34;
	bitmapinfo_t_2 *gBgradata = gBgramap;
	int fontadv = 0;
	int penpos_t = 0;

	if (osd_data == NULL) {
		osd_data = malloc(fontNum * fontWidth * fontHeight * 4);
		if (!osd_data) {
			printf("%s(%d):malloc osd_data error!\n", __func__, __LINE__);
			return -1;
		}
	}
	memset(osd_data, 0, fontNum * fontWidth * fontHeight * 4);

	if (IMP_OSD_GetRgnAttr(handler, &rAttr) < 0) {
		printf( "%s(%d):IMP_OSD_GetRgnAttr failed\n", __func__, __LINE__);
		return -1;
	}
	rTs.ts = ts;
	rAttr.type = OSD_REG_PIC;
	rAttr.rect.p0.x = x0;
	rAttr.rect.p0.y = y0;
	rAttr.rect.p1.x = x0 + fontWidth * fontNum - 1;
	rAttr.rect.p1.y = y0 + fontHeight - 1;

	if ((rAttr.rect.p0.x > 1919) || (rAttr.rect.p0.y > 1079) || (rAttr.rect.p1.x > 1919) || (rAttr.rect.p1.y > 1079))
		return 0;

	rAttr.fmt = PIX_FMT_BGR555LE;

	memset(DataStr, 0, 40);
	sprintf(DataStr, "%d", id);
	len = strlen(DataStr);

	for (i = 0; i < len; i++) {
		switch (DataStr[i]) {
		case '0' ... '9':
			dataData = (void *)gBgradata[DataStr[i] - '0'].pdata;
			fontadv = gBgradata[DataStr[i] - '0'].width;
			penpos_t += gBgradata[DataStr[i] - '0'].width;
			break;
		case 'A' ... 'Z':
			dataData = (void *)gBgradata[DataStr[i] - '7'].pdata;
			fontadv = gBgradata[DataStr[i] - '7'].width;
			penpos_t += gBgradata[DataStr[i] - '7'].width;
			break;
		case ':':
			dataData = (void *)gBgradata[64].pdata;
			fontadv = gBgradata[64].width;
			penpos_t += gBgradata[64].width;
			break;
		default:
			break;
		}
		for (j = 0; j < fontHeight; j++) {
			memcpy((void *)((uint16_t *)osd_data + j * fontNum * fontWidth + penpos_t),
					(void *)((uint16_t *)dataData + j * fontadv), fontadv * 2);
		}
	}
	rAttr.data.picData.pData = osd_data;

	/*if (IMP_OSD_SetRgnAttr(handler, &rAttr) < 0) {*/
	if (IMP_OSD_SetRgnAttrWithTimestamp(handler, &rAttr, &rTs) < 0) {
		printf("%s(%d):IMP_OSD_SetRgnAttr failed\n", __func__, __LINE__);
		return -1;
	}
	if (IMP_OSD_ShowRgn(handler, 0, 1) < 0) {
		printf("%s(%d): IMP_OSD_ShowRgn failed\n", __func__, __LINE__);
		return -1;
	}

	return 0;
}

const unsigned int _pow2_lut[33]={
	1073741824,1097253708,1121280436,1145833280,1170923762,1196563654,1222764986,1249540052,
	1276901417,1304861917,1333434672,1362633090,1392470869,1422962010,1454120821,1485961921,
	1518500250,1551751076,1585730000,1620452965,1655936265,1692196547,1729250827,1767116489,
	1805811301,1845353420,1885761398,1927054196,1969251188,2012372174,2056437387,2101467502,
	2147483648U
};

/**
 * IMP get the ev attr.gain is format [27.5], unit: dB, transform it to Au, format is [24.8].
 * for example, the value is 170, is '101.01010', is 5 + (10/32), is 5.3125.
 * Au = math_exp2(dB, shift_in, shift_out).
 * NOTE: [27.5] means that the high 27 bits represent an integer and the lower 5 bit represent decimal.
 **/
unsigned int math_exp2(unsigned int val, const unsigned char shift_in, const unsigned char shift_out)
{
	unsigned int fract_part = (val & ((1<<shift_in)-1));
	unsigned int int_part = val >> shift_in;
	if (shift_in <= 5)
	{
		unsigned int lut_index = fract_part << (5-shift_in);
		return _pow2_lut[lut_index] >> (30 - shift_out - int_part);
	}
	else
	{
		unsigned int lut_index = fract_part >> (shift_in - 5);
		unsigned int lut_fract = fract_part & ((1<<(shift_in-5))-1);
		unsigned int a = _pow2_lut[lut_index];
		unsigned int b =  _pow2_lut[lut_index+1];
		unsigned int res = ((unsigned long long)(b - a)*lut_fract) >> (shift_in-5);
		res = (res + a) >> (30-shift_out-int_part);

		return res;
	}
}

int host_get_isp_EV_attr(ISP_EV_Attr *ev_attr)
{
	int ret;
	unsigned int gain;
	IMPISPEVAttr attr;
	IMPISPITAttr itattr;

	ret = IMP_ISP_Tuning_GetEVAttr(&attr);
	if (ret) {
		printf("failed to get isp EV attr!\n");
		return -1;
	}

	ret = IMP_ISP_Tuning_GetIntegrationTime(&itattr);
	if (ret) {
		printf("failed to get isp integration time!\n");
		return -1;
	}

	/**
	 * The function get the value data format is [24.8],
	 * unit: Au.
	**/
	ret = IMP_ISP_Tuning_GetTotalGain(&gain);
	if (ret) {
		printf("failed to get isp get total gain!\n");
		return -1;
	}

	ev_attr->ev = attr.ev;
	ev_attr->expr_us = attr.expr_us;

	/**
	  * The attr.again data format is [27.5], unit: dB,
	  * and transform it to data format [22.10].unit:Au.
	  * Au = math_exp2(dB, shift_in, shift_out)
	 **/
	ev_attr->sensor_again = math_exp2(attr.again, 5, 10);

	/* the attr.dgain data format is [22.10], 1024 actual value is 1(Au), dB is 0 */
	ev_attr->sensor_dgain = 1024;

	/**
	 * The attr.dgain data format is [27.5], unit:dB,
	 * and transform it to data format [22.10], unit:Au.
	**/
	ev_attr->isp_dgain = math_exp2(attr.dgain, 5, 10);

	ev_attr->integration_time = itattr.integration_time;

	return 0;
}
