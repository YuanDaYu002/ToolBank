#ifndef LSSLAB_H
#define LSSLAB_H


#define LSSLAB_MAX_SIZE  512
#define LSSLAB_MAX_COUNT 65535

typedef void *LSSLAB_HANDLE;

/*
 * function: LSSLAB_HANDLE lsslab_create(int size, int count)
 * description:
 *		create a slab allocator which can hold @count item,
 *		each item has a fixed @size
 * return:
 *		non-NULL, handle to the slab created
 *		NULL, create failed
 * */
LSSLAB_HANDLE lsslab_create(int size, int count);

/*
 * function: int lsslab_destroy(LSSLAB_HANDLE slab)
 * description:
 *		destroy the specified @slab.
 *		make sure all the item of @slab have been freed before you destroy it!
 * return:
 *		0, success
 *		-1, fail
 * */
int lsslab_destroy(LSSLAB_HANDLE slab);

/*
 * function: void *lsslab_alloc(LSSLAB_HANDLE slab)
 * description:
 *		alloc a slab item from the specified @slab
 * return:
 *		non-NULL, address of the alloced slab item
 *		NULL, alloc failed
 * */
void *lsslab_alloc(LSSLAB_HANDLE slab);

/*
 * function: int lsslab_free(LSSLAB_HANDLE slab, void *obj)
 * description:
 *		free the slab item specified by @obj  from the specified @slab
 * return:
 *		0, success
 *		-1, fail
 * */
int lsslab_free(LSSLAB_HANDLE slab, void *obj);


#endif
