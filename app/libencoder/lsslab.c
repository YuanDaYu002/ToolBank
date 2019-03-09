#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <pthread.h>

#include "typeport.h"
#include "lsslab.h"


#define OFFSET_OF(TYPE, MEMBER) ((unsigned int) &((TYPE *)0)->MEMBER)

//#define SLB_DEBUG
#ifdef SLB_DEBUG
#define SLB_DEBUG_LOG(args...) \
do \
{\
	printf("<DEBUG file %s, line %d>: ", __FILE__, __LINE__);\
	printf(args);\
}while(0)
#else
#define SLB_DEBUG_LOG(args...)
#endif

typedef struct slb_item {
#define SLB_FLAG_USED (1<<0)
	int flag;
	struct slb_item *next; /*only used in free item list*/
	char data[1];
} slb_item;

typedef struct {
	int count;
	int used;
	int size;
	pthread_mutex_t lock;
	slb_item *free_list; /*used item list*/
	char *item_array;
} ls_slab;

LSSLAB_HANDLE lsslab_create(int size, int count)
{
	if (size < 0 || size > LSSLAB_MAX_SIZE)
		return NULL;

	if (count < 0 || count > LSSLAB_MAX_COUNT)
		return NULL;

	ls_slab *slb = (ls_slab *) malloc(sizeof (ls_slab));
	if (slb == NULL)
		return NULL;

	pthread_mutex_init(&slb->lock, NULL);

	slb->count = count;
	slb->used = 0;
	slb->size = size;
	slb->item_array = (char *) malloc((size + OFFSET_OF(slb_item, data)) * count);
	if (slb->item_array == NULL) {
		free(slb);
		return NULL;
	}
	slb->free_list = (slb_item *) slb->item_array;

	int i;
	slb_item *pitem = slb->free_list;
	for (i = 0; i < count - 1; i++) {
		pitem->flag = 0;
		pitem->next = (slb_item *) ((char *) pitem + (size + OFFSET_OF(slb_item, data)));
		pitem = pitem->next;
	}
	pitem->flag = 0;
	pitem->next = NULL;

	return slb;
}

int lsslab_destroy(LSSLAB_HANDLE hslab)
{
	assert(hslab != NULL);

	ls_slab *slab = (ls_slab *) hslab;

	pthread_mutex_lock(&slab->lock);

	/*we do it just in debug mode, because this
	 * should be guaranteed by the caller*/
#ifdef SLB_DEBUG
	if (slab->used != 0) {
		pthread_mutex_unlock(&slab->lock);
		SLB_DEBUG_LOG("slab still has %d unfreed item(s), can't be destroyed! ", slab->used);
		return -1;
	}
#endif

	free(slab->item_array);
	pthread_mutex_unlock(&slab->lock);
	free(slab);
	return 0;
}

void *lsslab_alloc(LSSLAB_HANDLE hslab)
{
	assert(hslab != NULL);

	ls_slab *slab = (ls_slab *) hslab;

	pthread_mutex_lock(&slab->lock);
	if (slab->free_list == NULL) {
		pthread_mutex_unlock(&slab->lock);
		SLB_DEBUG_LOG("slab doesn't have any free items\n");
		return NULL;
	}
	slab->used++;

	slb_item *pitem = slab->free_list;
	slab->free_list = pitem->next;
	pitem->next = NULL;
	pitem->flag |= SLB_FLAG_USED;
	pthread_mutex_unlock(&slab->lock);

	SLB_DEBUG_LOG("alloc address %p\n", pitem->data);
	return pitem->data;
}

int lsslab_free(LSSLAB_HANDLE hslab, void *obj)
{
	assert(hslab != NULL && obj != NULL);

	ls_slab *slab = (ls_slab *) hslab;

	pthread_mutex_lock(&slab->lock);
	slb_item *pitem = (slb_item *) ((char *) obj - OFFSET_OF(slb_item, data));
	SLB_DEBUG_LOG("slab item addr %p\n", pitem);
	if (!(pitem->flag & SLB_FLAG_USED)) {
		SLB_DEBUG_LOG("free twice, never do it again!\n");
		pthread_mutex_unlock(&slab->lock);
		return -1;
	}
	slab->used--;

	pitem->flag &= ~SLB_FLAG_USED;
	pitem->next = slab->free_list;
	slab->free_list = pitem;
	pthread_mutex_unlock(&slab->lock);

	return 0;
}

void lsslab_debug_info(LSSLAB_HANDLE hslab)
{
	assert(hslab != NULL);

	ls_slab *slab = (ls_slab *) hslab;

	DEBUG_LOG("----------------lsslab_debug_info %p---------------\n", slab);
	pthread_mutex_lock(&slab->lock);
	DEBUG_LOG("total %d, used %d\n", slab->count, slab->used);
	pthread_mutex_unlock(&slab->lock);
}

