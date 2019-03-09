
#ifndef HAL_WATCHDOG_H
#define HAL_WATCHDOG_H

#include "typeport.h"


#ifdef __cplusplus
extern "C"
{
#endif


/*
    function:  watchdog_enable
    description:  启用看门狗接口
    args:
        无
    return:
        0, 成功
        <0, 失败，返回值为错误码，具体见错误码定义
 */
int watchdog_enable(void);

/*
    function:  watchdog_disable
    description:  禁用看门狗接口
    args:
        无
    return:
        0, 成功
        <0, 失败，返回值为错误码，具体见错误码定义
 */
int watchdog_disable(void);

/*
    function:  watchdog_feed
    description:  看门狗喂狗接口
    args:
        无
    return:
        0, 成功
        <0, 失败，返回值为错误码，具体见错误码定义
 */
int watchdog_feed(void);



#ifdef __cplusplus
}
#endif

#endif

