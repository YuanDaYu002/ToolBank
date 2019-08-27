#ifndef BASE_H
#define BASE_H

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_SHORT 0x7FFF
#define MAX_INT 0x7FFFFFFF
#define MAX_LONG_LONG 0x7fffffff

#if defined(WIN32) && !defined(__cplusplus)
#define inline __inline
#endif

#define NO_INLINE

#ifndef WIN32
//linux gcc
#define inline extern inline// extern inline
//ios xcode
//#define inline static inline
#endif

#define _new(_struct, _pfinit, ...) (_pfinit?_pfinit((_struct *)mymemset(mymalloc(sizeof(_struct)), 0, sizeof(_struct)), ##__VA_ARGS__):((_struct *)mymemset(mymalloc(sizeof(_struct)), 0, sizeof(_struct))))
#define _delete(_o, _pffinalize) {if(_pffinalize)_pffinalize(_o);myfree(_o);}
#define _delete2(_o) {_o->finalize(_o);myfree(_o);}

#ifdef _DEBUG
#define ARRAY_CHECK
#endif

#define asize0_1(_array1Name, _sizeofT1)
#define asize1(_array1, _size1)
#define asize1_1(_array1, _size1, _array2Name, _sizeofT2)

#define asize1_2(_array1, _size1, _array2Name, _sizeofT2, _array3Name, _sizeofT3)
#define asize2(_array1, _size1, _array2, _size2)
#define asize2_1(_array1, _size1, _array2, _size2, _array3Name, _sizeofT3)
#define asize2_2(_array1, _size1, _array2, _size2, _array3Name, _sizeofT3, _array4Name, _sizeofT4)
#define asize3(_array1, _size1, _array2, _size2, _array3, _size3)
#define asize3_1(_array1, _size1, _array2, _size2, _array3, _size3, _array4Name, _sizeofT4)
#define asize4(_array1, _size1, _array2, _size2, _array3, _size3, _array4, _size4)
#define asize4_1(_array1, _size1, _array2, _size2, _array3, _size3, _array4, _size4, _array5Name, _sizeofT5)
#define asize5(_array1, _size1, _array2, _size2, _array3, _size3, _array4, _size4, _array5, _size5)
#define asize5_1(_array1, _size1, _array2, _size2, _array3, _size3, _array4, _size4, _array5, _size5, _array6Name, _sizeofT6)
#define asize6_2(_array1, _size1, _array2, _size2, _array3, _size3, _array4, _size4, _array5, _size5, _array6, _size6, _array7Name, _sizeofT7, _array8Name, _sizeofT8)
#define asize6_3(_array1, _size1, _array2, _size2, _array3, _size3, _array4, _size4, _array5, _size5, _array6, _size6, _array7Name, _sizeofT7, _array8Name, _sizeofT8, _array9Name, _sizeofT9)
#define asize7_3(_array1, _size1, _array2, _size2, _array3, _size3, _array4, _size4, _array5, _size5, _array6, _size6, _array7, _size7, _array8Name, _sizeofT8, _array9Name, _sizeofT9, _array10Name, _sizeofT10)
#define areset(_array, _size)
#define areset2(_arrayName, _array, _size)
#define adata(_array, _index) _array[_index]

#include "log.h"
#define throwError(errorMsg) {error("throwError", "%s\n", errorMsg);exit(101);}

#if !defined(__cplusplus)
#define bool char
#define true 1
#define false 0
#endif

#ifdef __cplusplus
}
#endif

#endif

