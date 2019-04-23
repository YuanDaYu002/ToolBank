 
/***************************************************************************
* @file: Link_Queue.c
* @author:   
* @date:  4,23,2019
* @brief:实现链式存储的队列的创建，销毁，入队，出队，遍历，判断是否为空队
* @attention:
***************************************************************************/ 

#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include"LinkQueue.h"
 
 

/*******************************************************************************
*@ Description    :创建链式队列头节点
*@ Input          :
*@ Output         :
*@ Return         :成功：qhead指针 ； 失败：NULL
*@ attention      :
*******************************************************************************/
QHeader* CreateLQueue(void)
{
    QHeader* qhead = NULL;
    qhead = (QHeader*)malloc(sizeof(QHeader));
    if(NULL == qhead)
    {
        printf("malloc memory failed\n");
        return qhead;
    }
    memset(qhead,0,sizeof(QHeader));
    return qhead;
}

/*******************************************************************************
*@ Description    : 销毁链式队列
*@ Input          :<qh>队头指针
*@ Output         :
*@ Return         :成功：0
					失败：小于零的错误码
*@ attention      :
*******************************************************************************/
int DestroyLQueue(QHeader* qh)
{
    if ( NULL == qh )
    {
        return LQUUE_ERR;
    }
    QNode* pDel = NULL;
    QNode* pTmp = NULL;
    pDel = qh->pHeader;
    while ( NULL != pDel)
    {
        pTmp = pDel->pNext;
        free(pDel);
        pDel = pTmp;
    }
    free(qh);
    qh = NULL;
    pDel = NULL;
    pTmp = NULL;
    return LQUEUE_OK;
}

/*******************************************************************************
*@ Description    :元素入队
*@ Input          :<qh>队列头指针
					<data>入队的数据
*@ Output         :
*@ Return         :成功：0
					失败：小于零的错误码
*@ attention      :
*******************************************************************************/
int InLQueue(QHeader* qh,data_t data)
{
    if((NULL == qh) || (LQUEUE_SIZE == qh->iNum))
    {
        return LQUUE_ERR;
    }
    QNode* pNode =(QNode*)malloc(sizeof(QNode));
    if ( NULL == pNode )
    {
        printf("malloc memory failed2\n");
        return LQUUE_ERR;
    }
    memset(pNode,0,sizeof(QNode));
    pNode->data = data;
    if ( NULL == qh->pHeader)
    {
        qh->pHeader = pNode;
        qh->pRear = pNode;
        qh->iNum++;
    }
    else           
    {
	    qh->pRear->pNext = pNode;
	    qh->pRear = pNode;
	    qh->iNum++;
    }
}
 
/*******************************************************************************
*@ Description    :元素出队列
*@ Input          :<qh>队列头指针
*@ Output         :<pdata>出队的节点里边的数据值
*@ Return         :成功：0
					失败：小于零的错误码
*@ attention      :
*******************************************************************************/
int DeLQueue(QHeader* qh,data_t *pdata)
{
    if ((NULL == qh) || (NULL == pdata)  || (0 == qh->iNum))
    {
        return LQUUE_ERR;
    }
    QNode* pDel = qh->pHeader;
    qh->pHeader = pDel->pNext;
    qh->iNum--;     
    *pdata = pDel->data;
    free(pDel);
    if ( NULL == qh->pHeader)
    {
        qh->pRear = NULL;
    }
    return LQUEUE_OK;
}
 
 

/*******************************************************************************
*@ Description    :判断链式队列是否为空
*@ Input          :<qh>队列头指针
*@ Output         :
*@ Return         :空：LQUEUE_EMPTY（-2）
					不空：LQUEUE_OK（0）
					出错：LQUUE_ERR  （-1）
*@ attention      :
*******************************************************************************/
int LQueueIsEmpty(QHeader*qh)
{
    if ( NULL == qh )
    {
        return LQUUE_ERR;
    }
    if ( 0 == qh->iNum)
    {
        return LQUEUE_EMPTY;
    }
    return LQUEUE_OK;
}
 
/*******************************************************************************
*@ Description    :遍历输出链式队列节点里边的值
*@ Input          :<qh>队列头指针
*@ Output         :
*@ Return         :
*@ attention      :
*******************************************************************************/
void ShowLQueue(QHeader *qh)
{
    if( ( NULL == qh ) || (0 == qh->iNum))
    {
        return;
    }
    QNode* pNode = qh->pHeader;
    while ( NULL != pNode)
    {
        printf("%5d",pNode->data);
        pNode = pNode->pNext;
    }
    printf("\n");
}

