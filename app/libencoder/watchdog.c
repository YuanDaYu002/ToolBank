
#include <fcntl.h>
#include <sys/ioctl.h>
#include <errno.h>

#include "watchdog.h"
//#include "ssp.h"
#include "mpi_sys.h"

//#define WDG_SET					(0x2010)
//#define WDG_ENABLE_BIT			15

#if 0
//HLE_U32 wdg_feed_disable;
void __suicide(void)
{
    //HLE_S32 wdg_reg_val = 0;
    //wdg_reg_val |= (1 << WDG_ENABLE_BIT);
    //wdg_feed_disable = 1;

    //ssp_register_write(WDG_SET, wdg_reg_val);
}
#endif

#define WATCHDOG_IOCTL_BASE		'W'
#define WDIOC_SETTIMEOUT		_IOWR(WATCHDOG_IOCTL_BASE, 6, int)
#define WDIOC_SETOPTIONS		_IOWR(WATCHDOG_IOCTL_BASE, 4, int)
#define WDIOC_KEEPALIVE			_IO(WATCHDOG_IOCTL_BASE, 5)
#define WDIOS_DISABLECARD		0x0001    /* Turn off the watchdog timer */
#define WDIOS_ENABLECARD		0x0002    /* Turn on the watchdog timer */

#define WDG_DEV_FILE	"/dev/watchdog"
int fdWdg = -1;

int watchdog_enable()
{
	if (fdWdg < 0) {
		fdWdg = open(WDG_DEV_FILE, O_RDWR);
		if (fdWdg < 0) {
			ERROR_LOG("open " WDG_DEV_FILE " fail: %d\n", errno);
			return HLE_RET_EIO;
		}
		
		int val = 10;
		int ret = ioctl(fdWdg, WDIOC_SETTIMEOUT, &val);
		if (0 != ret) {
			ERROR_LOG("ioctl WDIOC_SETTIMEOUT fail: %d\n", ret);
		}
		
		val = WDIOS_ENABLECARD;
		ret = ioctl(fdWdg, WDIOC_SETOPTIONS, &val);
		if (0 != ret) {
			ERROR_LOG("ioctl WDIOC_SETOPTIONS fail: %d\n", ret);
		}
		
		watchdog_feed();
	}

	return HLE_RET_OK;
}

int watchdog_disable()
{
	if (fdWdg < 0) {
		return HLE_RET_ENOTINIT;
	}
	
	int val = WDIOS_DISABLECARD;
	int ret = ioctl(fdWdg, WDIOC_SETOPTIONS, &val);
	if (0 != ret) {
		ERROR_LOG("ioctl WDIOC_SETOPTIONS fail: %d\n", ret);
		return HLE_RET_EIO;
	}

	return HLE_RET_OK;
}

int watchdog_feed()
{
	if (fdWdg < 0) {
		watchdog_enable();
		if (fdWdg < 0) return HLE_RET_EIO;
	}
	
	int ret = ioctl(fdWdg, WDIOC_KEEPALIVE);
	if (0 != ret) {
		ERROR_LOG("ioctl WDIOC_KEEPALIVE fail: %d\n", ret);
	}

	return HLE_RET_OK;
}

#if 0

#define RESET_SET					(0x2011)
#define GLOBAL_RESET_BIT			0
#define SENSOR_RESET_BIT			1

int global_reset(void)
{
	return ssp_register_write(RESET_SET, 1 << GLOBAL_RESET_BIT);
}

int sensor_reset(void)
{
	return ssp_register_write(RESET_SET, 1 << SENSOR_RESET_BIT);
}

int dsp_reset(void)
{
	ERROR_LOG("!!! dsp reset !!!\n");
#define DATA_GPIO7_7 0x201C0200

	int ret = HI_MPI_SYS_SetReg(DATA_GPIO7_7, 0);
	if (HI_SUCCESS != ret)
	{
		ERROR_LOG("HI_MPI_SYS_SetReg fail: %#x.\n", ret);
		return HLE_RET_ERROR;
	}

	usleep(50*1000);	//保持低电平 50ms 才能复位

	ret = HI_MPI_SYS_SetReg(DATA_GPIO7_7, 0xFF);
	if (HI_SUCCESS != ret)
	{
		ERROR_LOG("HI_MPI_SYS_SetReg fail: %#x.\n", ret);
		return HLE_RET_ERROR;
	}

	return HLE_RET_OK;
}

int phy_reset(void)
{
	DEBUG_LOG("phy 175 reset!\n");
#define DATA_GPIO6_5 0x201B0080

	int ret = HI_MPI_SYS_SetReg(DATA_GPIO6_5, 0);
	if (HI_SUCCESS != ret)
	{
		ERROR_LOG("HI_MPI_SYS_SetReg fail: %#x.\n", ret);
		return HLE_RET_ERROR;
	}

	usleep(50*1000);	//保持低电平 50ms 才能复位

	ret = HI_MPI_SYS_SetReg(DATA_GPIO6_5, 0xFF);
	if (HI_SUCCESS != ret)
	{
		ERROR_LOG("HI_MPI_SYS_SetReg fail: %#x.\n", ret);
		return HLE_RET_ERROR;
	}

	return HLE_RET_OK;
}
#endif

