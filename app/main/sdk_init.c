/******************************************************************************
  Some simple Hisilicon Hi3516A system functions.

  Copyright (C), 2010-2015, Hisilicon Tech. Co., Ltd.
 ******************************************************************************
    Modification:  2015-6 Created
******************************************************************************/
#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */

#include <stdio.h>
#include <asm/io.h>
#include <string.h>
#include "hi_type.h"
#include "hi_comm_sys.h"
#include "hi_comm_vi.h"
#include "hi_comm_vpss.h"
#include "hi_comm_vo.h"
#include "hi_comm_venc.h"
#include "hi_comm_vgs.h"
#include "hi_comm_isp.h"

#include "hi_module_param.h"
#include "hi_isp_param.h"
#include "osal_mmz.h"



//#define ENABLE_HIFB
#ifdef ENABLE_HIFB
#include "hifb.h"
#endif

//#include "adv7179.h"
//#include "hi_interdrv_param.h"

#define himm(address, value)    writel(value, address)
#define M_SIZE (1024*1024)
#define MEM_ALIGN(x) (((x)+ M_SIZE - 1)&(~(M_SIZE - 1)))


HI_CHAR* hi_chip = "hi3516ev100";
HI_U32 g_u32vi_vpss_online = 1;		/* 0, 1 */
HI_CHAR* sensor_type = "sc2235";	/* imx290 imx323 ov2718 ar0237 ov2718_2a jxf22 sc2235 imx385 imx307 */
HI_CHAR* vo_type = "CVBS";			/* BT656 LCD CVBS */
HI_CHAR* pin_mux_select = "vo";		/* vo net */

#define HI_ACODEC_TYPE_INNER        1
        
static HI_CHAR sensor_bus_type[5] = "i2c";
/**********************hi3516cv300 pinmux********************/
//I2C0 -> sensor
static HI_VOID i2c0_pin_mux(void)
{
    himm(0x1204002C, 0x3);  	//0:GPIO0_4; 1:SPI0_SDO; 2:SPI_3LINE_SDATA ;3:I2C0_SDA
    himm(0x12040030, 0x3);  	//0:GPIO8_3; 1:SPI0_SCLK; 2: SPI_3LINE_SCLK; 3:I2C0_SCL0

    //pin drive_capability
    himm(0x1204082c, 0x130);
    himm(0x12040830, 0x170);
}

//i2c1 -> aic31/adv7179
static HI_VOID i2c1_pin_mux(void)
{
	himm(0x12040020, 0x1);		//0:GPIO3_6; 1:I2C1_SDA
	himm(0x12040024, 0x1);

    himm(0x12040820, 0x130);    //I2C1_SDA
    himm(0x12040824, 0x130);    //I2C1_SCL
}

static HI_VOID spi0_pin_mux(void)
{
	himm(0x12040028, 0x1);
	himm(0x1204002c, 0x1);
	himm(0x12040030, 0x1);
	himm(0x12040034, 0x1);

    himm(0x12040830, 0x150);
    himm(0x1204082c, 0x120);
    himm(0x12040828, 0x120);
    himm(0x12040834, 0x130);
}

static HI_VOID sensor_pin_mux(void)
{
	himm(0x12040038, 0x1);
	himm(0x1204003c, 0x1);

    himm(0x12040838, 0x131);
    himm(0x1204083c, 0x120);
}

static HI_VOID vi_dc_bt1120_pin_mux(void)
{
	//himm(0x12040040, 0x1);      //VI_DATA13
	himm(0x12040044, 0x1);      //VI_DATA10
	//himm(0x12040048, 0x1);      //VI_DATA12
	himm(0x1204004c, 0x1);      //VI_DATA11
	himm(0x12040050, 0x1);      //VI_DATA9
	//himm(0x12040054, 0x1);      //VI_DATA14
	//himm(0x12040058, 0x1);      //VI_DATA15
	himm(0x1204005c, 0x1);      //VI_VS
	himm(0x12040060, 0x1);      //VI_HS

    //himm(0x12040840, 0x170);    //VI_DATA13
    himm(0x12040844, 0x130);    //VI_DATA10
    //himm(0x12040848, 0x130);    //VI_DATA12
    himm(0x1204084c, 0x130);    //VI_DATA11
    himm(0x12040850, 0x130);    //VI_DATA9
    //himm(0x12040854, 0x130);    //VI_DATA14
    //himm(0x12040858, 0x130);    //VI_DATA15
    himm(0x1204085c, 0x130);    //VI_VS
    himm(0x12040860, 0x130);    //VI_HS
}

static HI_VOID vicap_pin_mux(HI_CHAR* sensor_bus_type)
{
    if(!strcmp(sensor_bus_type, "i2c"))
    {
        i2c0_pin_mux();
    }
    else if(!strcmp(sensor_bus_type, "ssp"))
    {
        spi0_pin_mux();
    }

    sensor_pin_mux();

	himm(0x12040010, 0x1);
	himm(0x12040014, 0x1);
}

/*VDP*/
static HI_VOID vo_bt656_pin_mux(void)
{
	himm(0x12040040, 0x2);
	himm(0x12040044, 0x2);
	himm(0x12040048, 0x2);
	himm(0x1204004c, 0x2);
	himm(0x12040050, 0x2);
	himm(0x12040054, 0x2);
	himm(0x12040058, 0x2);
	himm(0x1204005c, 0x2);
	himm(0x12040060, 0x2);

    himm(0x12040840, 0x140);
    himm(0x12040844, 0x120);
    himm(0x12040848, 0x120);
    himm(0x1204084c, 0x120);
    himm(0x12040850, 0x120);
    himm(0x12040854, 0x120);
    himm(0x12040858, 0x120);
    himm(0x1204085c, 0x120);
    himm(0x12040860, 0x120);
}

static HI_VOID vo_lcd_pin_mux(void)
{
	himm(0x12040040, 0x3);
	himm(0x12040044, 0x3);
	himm(0x12040048, 0x3);
	himm(0x1204004c, 0x3);
	himm(0x12040050, 0x3);
	himm(0x12040054, 0x3);
	himm(0x12040058, 0x3);
	himm(0x1204005c, 0x3);
	himm(0x12040060, 0x3);
    himm(0x12040064, 0x3);

    himm(0x12040840, 0x140);
    himm(0x12040844, 0x110);
    himm(0x12040848, 0x110);
    himm(0x1204084c, 0x110);
    himm(0x12040850, 0x110);
    himm(0x12040854, 0x110);
    himm(0x12040858, 0x110);
    himm(0x1204085c, 0x110);
    himm(0x12040860, 0x110);
    himm(0x12040864, 0x110);
}

static HI_VOID spi1_pin_mux(void)
{
	himm(0x120400c4, 0x1);
	himm(0x120400c8, 0x1);
	himm(0x120400cc, 0x1);
	himm(0x120400d0, 0x1);
    himm(0x120400d4, 0x1);

    himm(0x120408e4, 0x130);
    himm(0x120408e8, 0x130);
    himm(0x120408ec, 0x120);
    himm(0x120408f0, 0x120);
    himm(0x120408f4, 0x100);
}

#if 0
static HI_VOID i2s_with_jtag_pin_mux(void)
{
	himm(0x120400c4, 0x4);
	himm(0x120400c8, 0x4);
	himm(0x120400cc, 0x4);
	himm(0x120400d0, 0x4);
    himm(0x120400d4, 0x4);

    himm(0x120408e4, 0x130);
    himm(0x120408e8, 0x130);
    himm(0x120408ec, 0x130);
    himm(0x120408f0, 0x130);
    himm(0x120408fc, 0x130);
}

static HI_VOID i2s_with_vi_pin_mux(void)
{
	himm(0x12040040, 0x4);
	himm(0x12040044, 0x4);
	himm(0x12040048, 0x4);
	himm(0x1204004c, 0x4);
    himm(0x12040058, 0x4);

    himm(0x12040840, 0x170);
    himm(0x12040844, 0x130);
    himm(0x12040848, 0x130);
    himm(0x1204084c, 0x130);
    himm(0x12040858, 0x130);
}

static HI_VOID spi_3line_pin_mux(void)
{
	himm(0x1204002c, 0x2);
	himm(0x12040030, 0x2);
	himm(0x12040034, 0x2);

    himm(0x1204082c, 0x130);
    himm(0x12040828, 0x170);
    himm(0x12040834, 0x130);
}
#endif

/*disable audio mute*/
static HI_VOID audio_mutex_loop_mux(void)
{
    //demo board
	himm(0x1204008C, 0x0);
	himm(0x12143400, 0x4);
	himm(0x12143010, 0x4);

    //socket board
	//himm(0x12040008, 0x0);
	//himm(0x12146400, 0x40);
	//himm(0x12146100, 0x40);
}

static HI_VOID pinmux_hi3516cv300(void)
{
    if (!strcmp(pin_mux_select, "vo"))
    {
        if (!strcmp(vo_type, "BT656"))
        {
            vo_bt656_pin_mux();
            i2c1_pin_mux();

            /* load peripheral equipment  */
        }
        else if (!strcmp(vo_type, "LCD"))
        {
			vo_lcd_pin_mux();
            spi1_pin_mux();

            /* load peripheral equipment */
        }
        else if (!strcmp(vo_type, "CVBS"))
        {

        }
        else
        {}
    }
    else
    {
        printf("pin_mux_select error....\n");
    }

    i2c1_pin_mux();
    //i2s_with_vi_pin_mux();
    //i2s_with_jtag_pin_mux();
    audio_mutex_loop_mux();
}

static HI_VOID clkcfg_hi3516cv300(void)
{
    himm (0x1201002c, 0x0486800d);		//VICAP & MIPI & SENSOR
    himm (0x12010034, 0x00400ff4);		//VDP unreset
    himm (0x12010040, 0x402);			//VEDU0 unreset
    himm (0x12010048, 0x802);			//VPSS0 unreset
    himm (0x1201005c, 0x802);			//VGS unreset
    himm (0x12010060, 0x2);				//JPGE unreset
    himm (0x1201006c, 0x2a);			//IVE unreset
    //himm (0x12010074, 0x0);			//GDC unreset
    himm (0x1201007C, 0x2);				//HASH&CIPHER unreset
    himm (0x1201008C, 0x2);				//AIO unreset
}

static HI_VOID sysctl_hi3516cv300(void)
{
    // mddrc pri&timeout setting
    // write cmd priority
    himm(0x120600c0,0x76543210);     // ports0
    himm(0x120600c4,0x76543210);     // ports1
    himm(0x120600c8,0x76543210);     // ports2
    himm(0x120600cc,0x76543210);     // ports3
    himm(0x120600d0,0x76543210);     // ports4
    himm(0x120600d4,0x76543210);     // ports5
    //himm(0x120600d8,0x76543210);     // ports6

    // read cmd priority
    himm(0x12060100,0x76543210);     // ports0
    himm(0x12060104,0x76543210);     // ports1
    himm(0x12060108,0x76543210);     // ports2
    himm(0x1206010c,0x76543210);     // ports3
    himm(0x12060110,0x76543210);     // ports4
    himm(0x12060114,0x76543210);     // ports5
    //himm(0x12060118,0x76543210);     // ports6

    // write cmd timeout
    himm(0x12060140,0x00000000);     // ports0 0:???1timeout
    himm(0x12060144,0x00000000);     // ports1
    himm(0x12060148,0x00000000);     // ports2
    himm(0x1206014c,0x00000000);     // ports3
    himm(0x12060150,0x00000000);     // ports4
    himm(0x12060154,0x00000000);     // ports5
    //himm(0x12060158,0x08040200);     // ports6

    // read cmd timeout
    himm(0x12060180,0x00004000);     // ports0
    himm(0x12060184,0x00004000);     // ports1
    himm(0x12060188,0x00000000);     // ports2
    himm(0x1206018c,0x00000000);     // ports3
    himm(0x12060190,0x00000100);     // ports4
    himm(0x12060194,0x00000000);     // ports5
    //himm(0x12060198,0x08040200);     // ports6

    //mddrc's Green port           # cjhhust 20160919, modyfied at 1009
    himm(0x12060200, 0x1f);
    himm(0x12060208, 0x2);
    himm(0x1206020c, 0x2);
    //himm(0x12060210, 0x2);
    himm(0x12060214, 0x3);
    himm(0x12060240, 0xb);
    himm(0x12060244, 0x0);

    // map mode
    himm(0x12060040, 0x81001000);   // ports0
    himm(0x12060044, 0x81001000);   // ports1
    himm(0x12060048, 0x81001000);   // ports2
    himm(0x1206004c, 0x81001000);   // ports3
    himm(0x12060050, 0x81001000);   // ports4
    himm(0x12060054, 0x81001000);   // ports5
    //himm(0x12060058 ,0x01001000);   // ports6

    himm(0x120614bc, 0x101);

    himm(0x113200E0,0xd);           // internal codec: AIO MCLK out, CODEC AIO TX MCLK
    //himm(0x113200E0,0xe);         // external codec: AIC31 AIO MCLK out, CODEC AIO TX MCLK

    if (g_u32vi_vpss_online)
    {
		//echo "==============vi_vpss_online==============";
        himm(0x12030000, 0x00000080);

        // priority select
        himm(0x12030044, 0x66664111);	//each module 4bit reserve   reserve     gzip     hash     jpge    aio     vicap   vdp
        himm(0x12030048, 0x66666013);	//each module 4bit reserve   reserve     fmc      sdio1    sdio0   arm9    vpss    vgs
        himm(0x1203004c, 0x65266666);	//each module 4bit reserve   ive         vedu     usb      cipher  dma_m1  dma_m0  eth
        // timeout select
        himm(0x12030050, 0x00000011);	//each module 4bit reserve   reserve     gzip     hash     jpge    aio     vicap   vdp
        himm(0x12030054, 0x00000110);	//each module 4bit reserve   reserve     fmc      sdio1    sdio0   arm9    vpss    vgs
        himm(0x12030058, 0x00000000);	//each module 4bit reserve   ive         vedu     usb      cipher  dma_m1  dma_m0  eth

        // mddrc vpss port outstanding
        himm(0x12060204, 0x1f);
    }
    else
    {
        //echo "==============vi_vpss_offline==============";
        himm (0x12030000, 0x00000000);

        // priority select
        himm (0x12030044, 0x66666111);	//each module 4bit reserve   reserve     gzip     hash     jpge    aio     vicap   vdp
        himm (0x12030048, 0x66666023);	//each module 4bit reserve   reserve     fmc      sdio1    sdio0   arm9    vpss    vgs
        himm (0x1203004c, 0x65266666);	//each module 4bit reserve   ive         vedu     usb      cipher  dma_m1  dma_m0  eth
        // timeout select
        himm (0x12030050, 0x00000011);	//each module 4bit reserve   reserve     gzip     hash     jpge    aio     vicap   vdp
        himm (0x12030054, 0x00000100);	//each module 4bit reserve   reserve     fmc      sdio1    sdio0   arm9    vpss    vgs
        himm (0x12030058, 0x00000000);	//each module 4bit reserve   ive         vedu     usb      cipher  dma_m1  dma_m0  eth

        // mddrc vpss port outstanding
        himm(0x12060204, 0x2);
    }
}

static HI_VOID insert_sns(void)
{
    if (!strcmp(sensor_type, "ar0237"))
    {
    	strncpy(sensor_bus_type, "i2c", 5);
		himm(0x1201002c, 0x0584800d);	// sensor unreset, clk 27MHz, VI 198MHz
    }
    else if (!strcmp(sensor_type, "ov2718"))
    {
    	strncpy(sensor_bus_type, "i2c", 5);
		himm(0x1201002c, 0x0604800d);	// sensor unreset, clk 24MHz, VI 198MHz
    }
    else if (!strcmp(sensor_type, "ov2718_2a"))
    {
    	strncpy(sensor_bus_type, "i2c", 5);
		himm(0x1201002c, 0x06048011);	// sensor unreset, clk 24MHz, VI 250MHz
    }
    else if (!strcmp(sensor_type, "imx290"))
    {
    	strncpy(sensor_bus_type, "i2c", 5);
        himm(0x1201002c, 0x0486800d);	// sensor unreset, clk 37.125MHz, VI 198MHz
    }
	else if (!strcmp(sensor_type, "imx385"))
    {
    	strncpy(sensor_bus_type, "i2c", 5);
        himm(0x1201002c, 0x0486800d);	// sensor unreset, clk 37.125MHz, VI 198MHz
    }
	else if (!strcmp(sensor_type, "imx307"))
	{
		strncpy(sensor_bus_type, "i2c", 5);
		himm(0x1201002c, 0x0486800d);	// sensor unreset, clk 37.125MHz, VI 198MHz
	}
    else if (!strcmp(sensor_type, "imx323"))
    {
    	strncpy(sensor_bus_type, "ssp", 5);
        vi_dc_bt1120_pin_mux();
		himm(0x1201002c, 0x0486800d);	// sensor unreset, clk 37.125MHz, VI 198MHz
    }
    else if (!strcmp(sensor_type, "jxf22"))
    {
    	strncpy(sensor_bus_type, "i2c", 5);
        himm(0x1201002c, 0x0604800d);	// sensor unreset, clk 37.125MHz, VI 198MHz

        himm(0x12040104, 0x0); //0:GPIO8_2;1: SAR_ADC_CH2
        himm(0x12148404, 0x1); //direction:out
        himm(0x12148010, 0x0); //data:0
    }
    else if (!strcmp(sensor_type, "sc2235"))
    {
    	strncpy(sensor_bus_type, "i2c", 5);
        vi_dc_bt1120_pin_mux();
        himm(0x1201002c, 0x0584800d);	// sensor unreset, clk 37.125MHz, VI 198MHz
    }
    else if (!strcmp(sensor_type, "bt1120"))
    {
        vi_dc_bt1120_pin_mux();
    }
    else
    {
        printf("sensor_type '%s' is error!!!\n", sensor_type);
    }

    vicap_pin_mux(sensor_bus_type);
}

static HI_VOID SYS_cfg(void)
{
    pinmux_hi3516cv300();
    clkcfg_hi3516cv300();
    sysctl_hi3516cv300();
}

static HI_S32 BASE_init(void)
{
    extern int base_mod_init(void);
    return base_mod_init();
}

/* calculate the MMZ info */
extern unsigned long g_usb_mem_size;
extern unsigned int g_sys_mem_addr_end;

static HI_S32 MMZ_init(void)
{
    extern int media_mem_init(void *);
    MMZ_MODULE_PARAMS_S stMMZ_Param;
    HI_U32 u32MmzStart, u32MmzSize;

    u32MmzStart = g_sys_mem_addr_end + g_usb_mem_size;
    u32MmzSize = (SYS_MEM_BASE + DDR_MEM_SIZE - u32MmzStart)/M_SIZE;

    /* default : 0x84100000 191M */
    snprintf(stMMZ_Param.mmz, MMZ_SETUP_CMDLINE_LEN, "anonymous,0,0x%x,%dM", u32MmzStart, u32MmzSize);

    stMMZ_Param.anony = 1;

    dprintf("mem_start=0x%x, MEM_OS_SIZE=%dM, MEM_USB_SIZE=%dM, mmz_start=0x%x, mmz_size=%dM\n",
        SYS_MEM_BASE, (g_sys_mem_addr_end-SYS_MEM_BASE)/M_SIZE, MEM_ALIGN(g_usb_mem_size)/M_SIZE, u32MmzStart, u32MmzSize);
    dprintf("mmz param= %s\n", stMMZ_Param.mmz);
    return media_mem_init(&stMMZ_Param);
}

static HI_S32 SYS_init(void)
{
    extern int sys_mod_init(void *);
    SYS_MODULE_PARAMS_S stSYS_Param;

    stSYS_Param.u32VI_VPSS_online = g_u32vi_vpss_online;
    stSYS_Param.u32EnOnlineDelayInt = 0;
    stSYS_Param.u32SensorNum = 1;
    snprintf(stSYS_Param.cSensor[0], sizeof(stSYS_Param.cSensor[0]), sensor_type);
    stSYS_Param.u32MemTotal = DDR_MEM_SIZE/M_SIZE;

    return sys_mod_init(&stSYS_Param);
}

static HI_S32 REGION_init(void)
{
    extern int rgn_mod_init(void *);

    RGN_MODULE_PARAMS_S stRgn_Param;
    stRgn_Param.bCanvasNumValid = 0;

    return rgn_mod_init(&stRgn_Param);
}

static HI_S32 VGS_init(void)
{
    extern int vgs_mod_init(void *);

    //VGS_MODULE_PARAMS_S stVGS_Param;
    return vgs_mod_init(NULL);
}

static HI_S32 ISP_init(void)
{
    extern int isp_mod_init(void *);

    ISP_MODULE_PARAMS_S stIsp_Param;

    stIsp_Param.u32PwmNum = 2;
    stIsp_Param.u32ProcParam = 0;
    stIsp_Param.u32UpdatePos = 0;

    return isp_mod_init(&stIsp_Param);
}

static HI_S32 VI_init(void)
{
    extern int viu_mod_init(void *);

    VI_MODULE_PARAMS_S stVI_Param;

    stVI_Param.s32DetectErrFrame = 10;
    stVI_Param.u32DropErrFrame = 0;
    stVI_Param.u32DiscardInt = 0;
    stVI_Param.u32IntdetInterval = 10;
    stVI_Param.bCscTvEnable = HI_FALSE;
    stVI_Param.bCscCtMode = HI_FALSE;
	stVI_Param.bExtCscEn = HI_FALSE;
    stVI_Param.bYuvSkip = HI_FALSE;
    stVI_Param.u32DelayLine = 16;

    return viu_mod_init(&stVI_Param);
}

static HI_S32 VPSS_init(void)
{
    extern int vpss_mod_init(void *);

    VPSS_MODULE_PARAMS_S stVPSS_Param;

    stVPSS_Param.bOneBufferforLowDelay = 0;
    stVPSS_Param.u32RfrFrameCmp = 1;

    return vpss_mod_init(&stVPSS_Param);
}

static HI_S32 VO_init(void)
{
    extern int vou_mod_init(void *);

    //VO_MODULE_PARAMS_S stVO_Param;
    return vou_mod_init(NULL);
}

#ifdef ENABLE_HIFB
static HI_S32 HIFB_init(void)
{
    extern int hifb_init(void*);
    HIFB_MODULE_PARAMS_S stHIFB_Param;

    snprintf(stHIFB_Param.video, 64, "hifb:vram0_size:1620");
    snprintf(stHIFB_Param.softcursor, 8, "off");

    return hifb_init(&stHIFB_Param);
}
#endif

static HI_S32 RC_init(void)
{
    extern int rc_mod_init(void *);

    return rc_mod_init(NULL);
}

static HI_S32 VENC_init(void)
{
    extern int venc_mod_init(void *);

    //VENC_PARAM_MOD_VENC_S stVencModuleParam;
    return venc_mod_init(NULL);
}

static HI_S32 CHNL_init(void)
{
    extern int chnl_mod_init(void);
    return chnl_mod_init();
}

static HI_S32 VEDU_init(void)
{
    extern int vedu_mod_init(void *);
    return vedu_mod_init(NULL);
}

static HI_S32 H264E_init(void)
{
    extern int h264e_mod_init(void *);

    //VENC_PARAM_MOD_H264E_S stH264eModuleParam;
    return h264e_mod_init(NULL);
}

static HI_S32 H265E_init(void)
{
    extern int h265e_mod_init(void *);

    //VENC_PARAM_MOD_H265E_S stH265eModuleParam;
    return h265e_mod_init(NULL);
}

static HI_S32 JPEGE_init(void)
{
    extern int jpege_mod_init(void *);

    //VENC_PARAM_MOD_JPEGE_S stJpegeModuleParam;
    return jpege_mod_init(NULL);
}

static HI_S32 SENSOR_init(void)
{
    extern int sensor_mod_init(void *);

    SENSOR_MODULE_PARAMS_S stSensorModuleParams;
    stSensorModuleParams.s32SensorIndex = 0;
    stSensorModuleParams.s32IspBindDev = 0;
    stSensorModuleParams.u32BusDev = 0;

    snprintf((void *)&stSensorModuleParams.u8BusType, 5, sensor_bus_type);

    return sensor_mod_init(&stSensorModuleParams);
}


static HI_S32 PWM_init(void)
{
    extern int pwm_mod_init(void);

    return pwm_mod_init();
}

static HI_S32 PIRIS_init(void)
{
    extern int piris_init(void);

    return piris_init();
}


static HI_S32 MIPI_init(void)
{
    extern int mipi_init(void);
    return mipi_init();
}

static HI_S32 CIPHER_init(void)
{
    extern int CIPHER_DRV_ModInit(void);

    return CIPHER_DRV_ModInit();
}

static HI_S32 AiaoMod_init(void)
{
    extern int aiao_mod_init(void);
    return aiao_mod_init();
}

static HI_S32 AiMod_init(void)
{
    extern int ai_mod_init(void);
    return ai_mod_init();
}

static HI_S32 AoMod_init(void)
{
    extern int ao_mod_init(void);
    return ao_mod_init();
}

static HI_S32 AencMod_init(void)
{
    extern int aenc_mod_init(void);
    return aenc_mod_init();
}

static HI_S32 AdecMod_init(void)
{
    extern int adec_mod_init(void);
    return adec_mod_init();
}

#ifdef HI_ACODEC_TYPE_INNER
static HI_S32 AcodecMod_init(void)
{
	extern int acodec_mod_init(void *);
    return acodec_mod_init(NULL);
}
#endif

#ifdef HI_ACODEC_TYPE_TLV320AIC31
static HI_S32 tlv320aic31Mod_init(void)
{
    extern int tlv320aic31_init(void);

    return tlv320aic31_init();
}
#endif

static HI_S32 IveMod_init(void)
{
    extern int ive_mod_init(void *);

    IVE_MODULE_PARAMS_S stParam = {0};
    stParam.bSavePowerEn = HI_TRUE;
    stParam.u16IveNodeNum = 512;

    return ive_mod_init(&stParam);
}

#if 0
static HI_S32 Adv7179Mod_init(void)
{
    ADV7179_MODULE_PARAMS_S stAdv7179Param;
    stAdv7179Param.Norm_mode = 0;

    return adv7179_init(&stAdv7179Param);
}
#endif

#if 0
static HI_S32 LCD_init(void)
{
    extern int hi_ssp_lcd_init(void);
    hi_ssp_lcd_init();
}
#endif

extern void osal_proc_init(void);

HI_VOID SDK_init(void)
{
    HI_S32 ret = 0;

    SYS_cfg();
	insert_sns();

    ret = MMZ_init();
    if (ret != 0)
    {
        printf("mmz init error.\n");
        goto Failed;
    }

    osal_proc_init();

    ret = BASE_init();
    if (ret != 0)
    {
        printf("base init error.\n");
        goto Failed;
    }

    ret = SYS_init();
    if (ret != 0)
    {
        printf("sys init error.\n");
        goto Failed;
    }

    ret = REGION_init();
    if (ret != 0)
    {
        printf("region init error.\n");
        goto Failed;
    }

    ret = VGS_init();
    if (ret != 0)
    {
        printf("vgs init error.\n");
        goto Failed;
    }

    ret = VI_init();
    if (ret != 0)
    {
        printf("vi init error.\n");
        goto Failed;
    }

    ret = ISP_init();
    if (ret != 0)
    {
        printf("isp init error.\n");
        goto Failed;
    }

    ret = VPSS_init();
    if (ret != 0)
    {
        printf("vpss init error.\n");
        goto Failed;
    }

    ret = VO_init();
    if (ret != 0)
    {
        printf("vo init error.\n");
        goto Failed;
    }

#ifdef ENABLE_HIFB
    ret = HIFB_init();
    if (ret != 0)
    {
        printf("hifb init error.\n");
        goto Failed;
    }
#endif

    ret = RC_init();
    if (ret != 0)
    {
        printf("rc init error.\n");
        goto Failed;
    }

    ret = VENC_init();
    if (ret != 0)
    {
        printf("venc init error.\n");
        goto Failed;
    }

    ret = CHNL_init();
    if (ret != 0)
    {
        printf("chnl init error.\n");
        goto Failed;
    }

	ret = VEDU_init();
    if (ret != 0)
    {
        printf("vedu init error.\n");
        goto Failed;
    }

    ret = H264E_init();
    if (ret != 0)
    {
        printf("h264e init error.\n");
        goto Failed;
    }

	ret = H265E_init();
    if (ret != 0)
    {
        printf("h265e init error.\n");
        goto Failed;
    }

    ret = JPEGE_init();
    if (ret != 0)
    {
        printf("jpege init error.\n");
        goto Failed;
    }

    ret = SENSOR_init();
    if (ret != 0)
    {
        printf("sensor'i2c init error.\n");
        goto Failed;
    }

    ret = PWM_init();
    if (ret != 0)
    {
        printf("pwm init error.\n");
        goto Failed;
    }

	ret = PIRIS_init();
    if (ret != 0)
    {
        printf("piris init error.\n");
        goto Failed;
    }

    ret = MIPI_init();
    if (ret != 0)
    {
        printf("mipi init error.\n");
        goto Failed;
    }

    ret = CIPHER_init();
    if (ret != 0)
    {
        printf("cipher init error.\n");
        goto Failed;
    }

    ret = AiaoMod_init();
    if (ret != 0)
    {
        printf("aiao init error.\n");
        goto Failed;
    }
    ret = AiMod_init();
    if (ret != 0)
    {
        printf("ai init error.\n");
        goto Failed;
    }
    ret = AoMod_init();
    if (ret != 0)
    {
        printf("ao init error.\n");
        goto Failed;
    }
    ret = AencMod_init();
    if (ret != 0)
    {
        printf("aenc init error.\n");
        goto Failed;
    }
    ret = AdecMod_init();
    if (ret != 0)
    {
        printf("adec init error.\n");
        goto Failed;
    }

#ifdef HI_ACODEC_TYPE_INNER
    ret = AcodecMod_init();
    if (ret != 0)
    {
        printf("acodec init error.\n");
        goto Failed;
    }
#endif

#ifdef HI_ACODEC_TYPE_TLV320AIC31
    ret = tlv320aic31Mod_init();
       if (ret != 0)
       {
           printf("tlv320aic31 init error.\n");
           goto Failed;
       }
#endif

    ret = IveMod_init();
    if (ret != 0)
    {
    	printf("Ive init error.\n");
        goto Failed;
    }

	//sample config as demo board, don't need this ko.

#if 0
    ret = Adv7179Mod_init();
    if (ret != 0)
    {
        printf("adv7179 init error.\n");
    }
#endif

    printf("SDK init ok...\n");
	return;
Failed:
    printf("SDK init failed...\n");
    return;

}


#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */

