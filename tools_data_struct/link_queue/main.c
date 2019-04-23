/*****************************************************************************
  文 件 名   : main.c
  生成日期   : 2016年7月27日
   功能描述   : 实现链式存储的队列的创建，销毁，入队，出队，遍历，判断是否为
               空队
******************************************************************************/

#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include"LinkQueue.h"
 
int main()
{
    QHeader* phead = CreateLQueue();
    if ( NULL == phead)
    {
        return -1;
    }
    int i = 0;
    int ret = 0;
    for ( i = 0 ; i <9 ; i++ )
    {
        ret = InLQueue(phead, i);
        if ( LQUUE_ERR == ret )
        {
            break;
        }
    }
    ShowLQueue(phead);
    ret = LQueueIsEmpty(phead);
    printf("the queue is empty or not %d\n",ret);
    int num = -1;
    ret = DeLQueue(phead, &num);
    printf("the dequeue num is %d\n",num);
    ShowLQueue(phead);
    num = -1;
    ret = DeLQueue(phead, &num);
    printf("the dequeue num is %d\n",num);
    ShowLQueue(phead);
    DestroyLQueue(phead);
}

