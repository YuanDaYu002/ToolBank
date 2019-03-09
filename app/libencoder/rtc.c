#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <linux/rtc.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <stdlib.h>

#include "mpi_sys.h"

#include "typeport.h"
#include "hi_rtc.h"

#define RTC_PATH "/dev/hi_rtc"

static int fd_rtc;

//NOTICE: use GMT(UTC) time

int set_rtc_time(struct tm *tm_time)
{
    rtc_time_t rtt;
    rtt.year = tm_time->tm_year + 1900;
    rtt.month = tm_time->tm_mon + 1;
    rtt.date = tm_time->tm_mday;
    rtt.hour = tm_time->tm_hour;
    rtt.minute = tm_time->tm_min;
    rtt.second = tm_time->tm_sec;
    rtt.weekday = tm_time->tm_wday;

    if (rtt.year < 2018) {
        return HLE_RET_ERROR;
    }

    if (ioctl(fd_rtc, HI_RTC_SET_TIME, &rtt) < 0) {
        ERROR_LOG("ioctl HI_RTC_SET_TIME fail!\n");
        return HLE_RET_ERROR;
    }

    return HLE_RET_OK;
}

//NOTICE: return GMT(UTC) time

int get_rtc_time(struct tm *tm_time)
{
    rtc_time_t rtt;
    if (ioctl(fd_rtc, HI_RTC_RD_TIME, &rtt) < 0) {
        ERROR_LOG("ioctl RTC_RD_TIME fail!\n");
        return HLE_RET_ERROR;
    }
    if (rtt.year < 2018) {
        return HLE_RET_ERROR;
    }

    tm_time->tm_year = rtt.year - 1900;
    tm_time->tm_mon = rtt.month - 1;
    tm_time->tm_mday = rtt.date;
    tm_time->tm_hour = rtt.hour;
    tm_time->tm_min = rtt.minute;
    tm_time->tm_sec = rtt.second;
    tm_time->tm_isdst = -1;

    /*
            if (70 == tm_time->tm_year
                    && 0 == tm_time->tm_mon
                    && 1 == tm_time->tm_mday
                    && 0 == tm_time->tm_hour
                    && 0 == tm_time->tm_min
                    && 0 == tm_time->tm_sec)
                    return HLE_RET_ERROR;
     */

    return HLE_RET_OK;
}

#if 0

int sys_to_rtc(void)
{
    return system("hwclock -u -w") ? HLE_RET_ERROR : HLE_RET_OK;
}

int rtc_to_sys(void)
{
    return system("hwclock -u -s") ? HLE_RET_ERROR : HLE_RET_OK;
}
#endif

int rtc_init(void)
{
    fd_rtc = open(RTC_PATH, O_RDWR);
    if (-1 == fd_rtc) {
        ERROR_LOG("Open rtc file fail!\n");
        return HLE_RET_ERROR;
    }

    struct timeval tv;
    struct tm tm_time;

    gettimeofday(&tv, NULL);
    int ret = get_rtc_time(&tm_time);
    if (HLE_RET_ERROR == ret) {
        //printf("10000000005\n");
        //从RTC 里面获取时间失败，那么把系统时间设置到RTC
        gmtime_r(&tv.tv_sec, &tm_time);
        ret = set_rtc_time(&tm_time);
        if (HLE_RET_OK != ret) return HLE_RET_ERROR;
    }
    else {
        //如果系统时间和RTC 时间不一致，那么就把RTC 时间设置到系统时间
        char* oldtz = getenv("TZ");
        //setenv("TZ", "UTC0", 1);
        tv.tv_sec = mktime(&tm_time);
        if (oldtz) {
            //setenv("TZ", oldtz, 1);
        }
        else {
            //unsetenv("TZ");
        }
        settimeofday(&tv, NULL);
    }

    DEBUG_LOG("Cur Sys time :%s", ctime(&tv.tv_sec));

    return HLE_RET_OK;
}

void rtc_exit(void)
{
    close(fd_rtc);
}


#if 0

int main(int argc, char** argv)
{
    struct tm t;
    rtc_init();
    get_rtc_time(&t);
    printf("read from rtc: %d-%d-%d %d:%d:%d %d\n", t.tm_year, t.tm_mon, t.tm_mday, t.tm_hour, t.tm_min, t.tm_sec, t.tm_wday);


    struct timeval tmv;
    gettimeofday(&tmv, NULL);
    printf("tv_sec=%d, %d, %d\n", tmv.tv_sec, mktime(&t), time(NULL));

    time_t tmt;
    struct tm gmt, loc;
    printf("TZ=%s\n", getenv("TZ"));
    time(&tmt);
    gmtime_r(&tmt, &gmt);
    localtime_r(&tmt, &loc);
    printf("time()=%d, mktime(gmtime())=%d, mktime(localtime())=%d, ctime()=%s\n", tmt, mktime(&gmt), mktime(&loc), ctime(&tmt));
    putenv("TZ='UTC0'");
    printf("TZ=%s\n", getenv("TZ"));
    time(&tmt);
    gmtime_r(&tmt, &gmt);
    localtime_r(&tmt, &loc);
    printf("time()=%d, mktime(gmtime())=%d, mktime(localtime())=%d, ctime()=%s\n", tmt, mktime(&gmt), mktime(&loc), ctime(&tmt));
}
#endif

