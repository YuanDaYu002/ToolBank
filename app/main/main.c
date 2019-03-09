


#include "sys/types.h"

#include "sys/time.h"
#include "unistd.h"
#include "fcntl.h"
#include "sys/statfs.h"
#include "limits.h"

#include "los_event.h"
#include "los_printf.h"

#include "lwip/tcpip.h"
#include "lwip/netif.h"
#include "eth_drv.h"
#include "arch/perf.h"

#include "fcntl.h"
#include "fs/fs.h"

#include "stdio.h"

#include "shell.h"
#include "hisoc/uart.h"
#include "vfs_config.h"
#include "disk.h"

#include "los_cppsupport.h"

#include "linux/fb.h"

#include "securec.h"

#include "implementation/usb_init.h"

#include "proc_fs.h"
#include "hirandom.h"
#include "console.h"
#include "uart.h"
#include "nand.h"
#include "mtd_partition.h"
#include "spinor.h"
#include "dmac_ext.h"
#include "gpio.h"
#include "spi.h"
#include "tools_shell_cmd.h"
#include "sys/mount.h"
#include "write_file_to_flash.h"
#include "media_server_interface.h"
#include "itfEncoder.h"
#include "hls_main.h"



extern int mem_dev_register(void);
extern UINT32 osShellInit(char *);
extern void CatLogShell(void);
extern void hisi_eth_init(void);
extern void fuvc_frame_descriptors_set(void);
#if 0
extern void proc_fs_init(void);
extern int uart_dev_init(void);
extern int system_console_init(const char *);
extern int nand_init(void);
extern int add_mtd_partition( char *, UINT32 , UINT32 , UINT32 );
extern int spinor_init(void);
extern void spi_dev_init(void);
extern int i2c_dev_init(void);
extern int gpio_dev_init(void);
extern int dmac_init(void);
extern int ran_dev_register(void);
extern void tools_cmd_register(void);
extern UINT32 usb_init(controller_type ctype, device_type dtype);

extern int mount(const char   *device_name,
              const char   *path,
              const char   *filesystemtype,
              unsigned long rwflag,
              const void   *data);

#endif
extern void SDK_init(void);
extern int app_main(int argc, char* argv[]);

//define, max args ---20
#define ARGS_SIZE_T     20
#define ARG_BUF_LEN_T   256
static char *ptask_args[ARGS_SIZE_T];
static char *args_buf_t = NULL;
struct netif* pnetif;

static int taskid = -1;

int secure_func_register(void)
{
    int ret;
    STlwIPSecFuncSsp stlwIPSspCbk= {0};
    stlwIPSspCbk.pfMemset_s = memset_s;
    stlwIPSspCbk.pfMemcpy_s = memcpy_s;
    stlwIPSspCbk.pfStrNCpy_s = strncpy_s;
    stlwIPSspCbk.pfStrNCat_s = strncat_s;
    stlwIPSspCbk.pfStrCat_s = strcat_s;
    stlwIPSspCbk.pfMemMove_s = memmove_s;
    stlwIPSspCbk.pfSnprintf_s = snprintf_s;
    stlwIPSspCbk.pfRand = rand;
    ret = lwIPRegSecSspCbk(&stlwIPSspCbk);
    if (ret != 0)
    {
        PRINT_ERR("\n***lwIPRegSecSspCbk Failed***\n");
        return -1;
    }

    PRINTK("\nCalling lwIPRegSecSspCbk\n");
    return ret;
}

void com_app(unsigned int p0, unsigned int p1, unsigned int p2, unsigned int p3)
{
    int i = 0;
    unsigned int argc = p0;
    char **argv = (char **)p1;

    //Set_Interupt(0);

    dprintf("\ninput command:\n");
    for(i=0; i<argc; i++) {
        dprintf("%s ", argv[i]);
    }
    dprintf("\n");
    
    app_main(argc,argv);
    dprintf("\nmain out\n");

    dprintf("[END]:app_test finish!\n");
    free(args_buf_t);
    args_buf_t = NULL;
    taskid = -1;
}

void app_sample(int argc, char **argv )
{
    int i = 0, ret = 0;
    int len = 0;
    char *pch = NULL;
    TSK_INIT_PARAM_S stappTask;

    if(argc < 1) {
        dprintf("illegal parameter!\n");
    }

    if (taskid != -1) {
        dprintf("There's a app_main task existed.\n");
    }
    args_buf_t = zalloc(ARG_BUF_LEN_T);
    memset(&stappTask, 0, sizeof(TSK_INIT_PARAM_S));
    pch = args_buf_t;
    for(i=0; i<ARGS_SIZE_T; i++) {
        ptask_args[i] = NULL;
    }
    argc++;
    ptask_args[0] = "sample";

    for(i = 1; i < argc; i++)
    {
        len =  strlen(argv[i-1]);
        memcpy(pch , argv[i-1], len);
        ptask_args[i] = pch;
        //keep a '\0' at the end of a string.
        pch = pch + len + 1;
        if (pch >= args_buf_t +ARG_BUF_LEN_T) {
            dprintf("args out of range!\n");
            break;
        }
    }
    memset(&stappTask, 0, sizeof(TSK_INIT_PARAM_S));
    stappTask.pfnTaskEntry = (TSK_ENTRY_FUNC)com_app;
    stappTask.uwStackSize = 0x30000;
    stappTask.pcName = "sample";
    stappTask.usTaskPrio = 10;
    stappTask.uwResved   = LOS_TASK_STATUS_DETACHED;
    stappTask.auwArgs[0] = argc;
    stappTask.auwArgs[1] = (UINT32)ptask_args;
    ret = LOS_TaskCreate((UINT32 *)&taskid, &stappTask);
    if (LOS_OK != ret)
    {
        dprintf("LOS_TaskCreate err, ret:%d\n", ret);
    }
    else
    {
        dprintf("camera_Task %d\n", taskid);
    }

    //chdir("/sd0");
    chdir("/nfs");

}

void fmp4_record_(int argc, char **argv )
{
    int i = 0, ret = 0;
    int len = 0;
    char *pch = NULL;
    TSK_INIT_PARAM_S stappTask;


    if (taskid != -1) {
        dprintf("There's a app_main task existed.\n");
    }
    args_buf_t = zalloc(ARG_BUF_LEN_T);
    memset(&stappTask, 0, sizeof(TSK_INIT_PARAM_S));
    pch = args_buf_t;
    for(i=0; i<ARGS_SIZE_T; i++) {
        ptask_args[i] = NULL;
    }
    argc++;
    ptask_args[0] = "fmp4_record";

    for(i = 1; i < argc; i++)
    {
        len =  strlen(argv[i-1]);
        memcpy(pch , argv[i-1], len);
        ptask_args[i] = pch;
        //keep a '\0' at the end of a string.
        pch = pch + len + 1;
        if (pch >= args_buf_t +ARG_BUF_LEN_T) {
            dprintf("args out of range!\n");
            break;
        }
    }
    memset(&stappTask, 0, sizeof(TSK_INIT_PARAM_S));
    stappTask.pfnTaskEntry = (TSK_ENTRY_FUNC)fmp4_record;
    stappTask.uwStackSize = 0x30000;
    stappTask.pcName = "sample";
    stappTask.usTaskPrio = 8;//10;
    stappTask.uwResved   = LOS_TASK_STATUS_DETACHED;
    stappTask.auwArgs[0] = argc;
    stappTask.auwArgs[1] = (UINT32)ptask_args;
    ret = LOS_TaskCreate((UINT32 *)&taskid, &stappTask);
    if (LOS_OK != ret)
    {
        dprintf("LOS_TaskCreate err, ret:%d\n", ret);
    }
    else
    {
        dprintf("fmp4_record_Task %d\n", taskid);
    }

}

void hls_main_(int argc, char **argv )
{
    int i = 0, ret = 0;
    int len = 0;
    char *pch = NULL;
    TSK_INIT_PARAM_S stappTask;


    if (taskid != -1) {
        dprintf("There's a hls_main task existed.\n");
    }
    args_buf_t = zalloc(ARG_BUF_LEN_T);
    memset(&stappTask, 0, sizeof(TSK_INIT_PARAM_S));
    pch = args_buf_t;
    for(i=0; i<ARGS_SIZE_T; i++) {
        ptask_args[i] = NULL;
    }
    argc++;
    ptask_args[0] = "hls_main";

    for(i = 1; i < argc; i++)
    {
        len =  strlen(argv[i-1]);
        memcpy(pch , argv[i-1], len);
        ptask_args[i] = pch;
        //keep a '\0' at the end of a string.
        pch = pch + len + 1;
        if (pch >= args_buf_t +ARG_BUF_LEN_T) {
            dprintf("args out of range!\n");
            break;
        }
    }
    memset(&stappTask, 0, sizeof(TSK_INIT_PARAM_S));
    stappTask.pfnTaskEntry = (TSK_ENTRY_FUNC)hls_main;
    stappTask.uwStackSize = 0x60000;
    stappTask.pcName = "sample";
    stappTask.usTaskPrio = 8;//10;
    stappTask.uwResved   = LOS_TASK_STATUS_DETACHED;
    stappTask.auwArgs[0] = argc;
    stappTask.auwArgs[1] = (UINT32)ptask_args;
    ret = LOS_TaskCreate((UINT32 *)&taskid, &stappTask);
    if (LOS_OK != ret)
    {
        dprintf("LOS_TaskCreate err, ret:%d\n", ret);
    }
    else
    {
        dprintf("hls_main_Task %d\n", taskid);
        printf("hls_main_Task %d\n", taskid);
    }

}

extern int url_dowload_file(int argc, char * argv [ ]);
//extern int hls_main (int argc, char* argv[]);
void sample_command(void)
{
    osCmdReg(CMD_TYPE_EX, "sample", 0, (CMD_CBK_FUNC)app_sample);
    osCmdReg(CMD_TYPE_EX, "WriteNorFlash",1, (CMD_CBK_FUNC)write_file_to_nor_flash);
    osCmdReg(CMD_TYPE_EX, "RecvWriteNor",0, (CMD_CBK_FUNC)receive_and_write_file_to_nor_flash);
    osCmdReg(CMD_TYPE_EX, "my_tcp_send",2, (CMD_CBK_FUNC)tcp_send);
    
    //osCmdReg(CMD_TYPE_EX, "httpPost",2, (CMD_CBK_FUNC)http_post);
    //osCmdReg(CMD_TYPE_EX, "httpDowloadFile",1, (CMD_CBK_FUNC)http_dowload_file);
    osCmdReg(CMD_TYPE_EX, "fmp4_record_",0, (CMD_CBK_FUNC)fmp4_record_);    
    osCmdReg(CMD_TYPE_EX, "dow_upd_file",1, (CMD_CBK_FUNC)dowload_upadte_file);
    osCmdReg(CMD_TYPE_EX, "hls_main",0, (CMD_CBK_FUNC)hls_main);
    

}

void net_init(void)
{
#ifdef LOSCFG_DRIVERS_HIGMAC
    extern struct los_eth_driver higmac_drv_sc;
    pnetif = &(higmac_drv_sc.ac_if);
    higmac_init();
#endif

#ifdef LOSCFG_DRIVERS_HIETH_SF
    extern struct los_eth_driver hisi_eth_drv_sc;
    pnetif = &(hisi_eth_drv_sc.ac_if);
    hisi_eth_init();
#endif

    dprintf("cmd_startnetwork : DHCP_BOUND finished\n");
    netif_set_up(pnetif);
}

extern unsigned int g_uwFatSectorsPerBlock;
extern unsigned int g_uwFatBlockNums;
#define SUPPORT_FMASS_PARITION
#ifdef SUPPORT_FMASS_PARITION
extern int fmass_register_notify(void(*notify)(void* context, int status), void* context);
extern int fmass_partition_startup(char* path);
void fmass_app_notify(void* conext, int status)
{
    if(status == 1)/*usb device connect*/
    {
        char *path = "/dev/mmcblk0p0";
        //startup fmass access patition
        fmass_partition_startup(path);
    }
}
#endif

#include "board.h"
extern UINT32 g_sys_mem_addr_end;
extern unsigned int g_uart_fputc_en;
void board_config(void)
{
    g_sys_mem_addr_end = SYS_MEM_BASE + SYS_MEM_SIZE_DEFAULT;
    g_uwSysClock = OS_SYS_CLOCK;
    g_uart_fputc_en = 1;
    extern unsigned long g_usb_mem_addr_start;
    extern unsigned long g_usb_mem_size;
    g_usb_mem_addr_start = g_sys_mem_addr_end;
    //recommend 1M nonCache for usb, g_usb_mem_size should align with 1M
    g_usb_mem_size = 0x100000;

    g_uwFatSectorsPerBlock = CONFIG_FS_FAT_SECTOR_PER_BLOCK;
    g_uwFatBlockNums       = CONFIG_FS_FAT_BLOCK_NUMS;
}



void app_init(void)
{
    dprintf("random init ...\n");
    ran_dev_register();

    dprintf("uart init ...\n");
    uart_dev_init();

    dprintf("shell init ...\n");
    system_console_init(TTY_DEVICE);
    osShellInit(TTY_DEVICE);
    
    dprintf("hisi_wifi_shell_cmd_register init ...\n");
    hisi_wifi_shell_cmd_register();

    dprintf("spi nor flash init ...\n");
    if(!spinor_init()){
        add_mtd_partition("spinor", 0x800000, 8*0x100000, 0);
        //add_mtd_partition("spinor", 10*0x100000, 2*0x100000, 1);
        mount("/dev/spinorblk0", "/jffs0", "jffs", 0, NULL);
    }
    
    /*挂载ramfs 文件系统,实测不稳定，写的文件一大就报没空间，且延时上秒级别，华为官方也不建议用*/
    #if 1
    int swRet=0;
    swRet = mount((const char *)NULL, "/ramfs", "ramfs", 0, NULL);
    if (swRet) 
    {
        dprintf("mount ramfs err %d\n", swRet);
        return;
    }
    dprintf("Mount ramfs finished.\n");
    #endif
    

    dprintf("gpio init ...\n");
    gpio_dev_init();

    dprintf("net init ...\n");
    net_init();
    
    dprintf("sd/mmc host init ...\n");
    SD_MMC_Host_init();

    //dprintf("os vfs init ...\n");
    //proc_fs_init();

    dprintf("mem dev init ...\n");
    mem_dev_register();

#if 0
    dprintf("nand init ...\n");
    if(!nand_init()) {
        add_mtd_partition("nand", 0x200000, 32*0x100000, 0);
        add_mtd_partition("nand", 0x200000 + 32*0x100000, 32*0x100000, 1);
        mount("/dev/nandblk0", "/yaffs0", "yaffs", 0, NULL);
        //mount("/dev/nandblk1", "/yaffs1", "yaffs", 0, NULL);
    }
#endif
    
    dprintf("spi bus init ...\n");
    spi_dev_init();

    dprintf("i2c bus init ...\n");
    i2c_dev_init();

    dprintf("dmac init\n");
    dmac_init();

    dprintf("tcpip init ...\n");
    (void)secure_func_register();
    tcpip_init(NULL, NULL);

    #ifdef LOSCFG_DRIVERS_USB
        #ifdef LOSCFG_DRIVERS_USB_UVC_GADGET
            dprintf("usb fuvc frame set...\n");
            fuvc_frame_descriptors_set();
            usb_init(DEVICE, DEV_UVC);
        #else
            usb_init(HOST, DEV_MASS);
        #endif

        dprintf("usb init ...\n");
        //usb_init must behind SD_MMC_Host_init
    #endif

    hi1131s_init();

    dprintf("g_sys_mem_addr_end=0x%08x,\n",g_sys_mem_addr_end);

    CatLogShell();   //cat_logmpp

    dprintf("done init!\n");
    dprintf("Date:%s.\n", __DATE__);
    dprintf("Time:%s.\n", __TIME__);
    SDK_init();
    sample_command();

    tools_cmd_register();
    
   // app_sample(1,NULL);//自动调用APP
    
    return;
}

/* EOF kthread1.c */












