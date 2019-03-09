#ifndef SFC_SCALER_H
#define SFC_SCALER_H


#include "typeport.h"

#ifdef __cplusplus
extern "C"
{
#endif

typedef struct
{
    HLE_U32 u32PhyAddr;
    HLE_U32 u32Width;
    HLE_U32 u32Height;
} HLE_SURFACE;

//创建一个suface，调用前需指定sfc->u32Width和sfc->u32Height
//返回surface的对应的虚拟地址
void *create_surface(HLE_SURFACE *sfc);

//销毁surface
void destroy_surface(HLE_SURFACE *sfc, void *addr);


//surface缩放操作，把src缩放到dst
int scale_surface(HLE_SURFACE *dst, HLE_SURFACE *src);


int scaler_init(void);



#ifdef __cplusplus
}
#endif

#endif


