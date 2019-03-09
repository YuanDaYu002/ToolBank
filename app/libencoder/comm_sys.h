#ifndef COMM_SYS_H
#define COMM_SYS_H

#include "typeport.h"

#define HI_MPI_SYS_GetReg(u32Addr,pu32Value)  SYS_GetReg(u32Addr,pu32Value)
#define HI_MPI_SYS_SetReg(u32Addr,pu32Value)  SYS_SetReg(u32Addr,pu32Value)


HI_S32 SYS_GetReg(HI_U32 u32Addr, HI_U32 *pu32Value);
HI_S32 SYS_SetReg(HI_U32 u32Addr, HI_U32 u32Value);



#endif


