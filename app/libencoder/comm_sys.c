#include "hi_type.h"
#include "mpi_sys.h"
#include "comm_sys.h"
/*
等价于之前版本的 HI_MPI_SYS_SetReg
*/
HI_S32 SYS_SetReg(HI_U32 u32Addr, HI_U32 u32Value)
{
    HI_U32 *pu32Addr = NULL;
    HI_U32 u32MapLen = sizeof(u32Value);

    pu32Addr = (HI_U32 *)HI_MPI_SYS_Mmap(u32Addr, u32MapLen);
    if(NULL == pu32Addr)
    {
        return HI_FAILURE;
    }

    *pu32Addr = u32Value;

    return HI_MPI_SYS_Munmap(pu32Addr, u32MapLen);
}

/*
等价于之前版本的 HI_MPI_SYS_GetReg
*/
HI_S32 SYS_GetReg(HI_U32 u32Addr, HI_U32 *pu32Value)
{
    HI_U32 *pu32Addr = NULL;
    HI_U32 u32MapLen;

    if (NULL == pu32Value)
    {
        return HI_ERR_SYS_NULL_PTR;
    }

    u32MapLen = sizeof(*pu32Value);
    pu32Addr = (HI_U32 *)HI_MPI_SYS_Mmap(u32Addr, u32MapLen);
    if(NULL == pu32Addr)
    {
        return HI_FAILURE;
    }

    *pu32Value = *pu32Addr;

    return HI_MPI_SYS_Munmap(pu32Addr, u32MapLen);
}
