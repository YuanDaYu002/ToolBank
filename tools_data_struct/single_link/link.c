 
/***************************************************************************
* @file: link.c
* @author:   
* @date:  4,22,2019
* @brief:  单链表相关操作函数(heade节点数据域为空)
* @attention:
***************************************************************************/


#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "link.h"
 
 
/*******************************************************************************
*@ Description    :创建链表头节点
*@ Input          :
*@ Output         :
*@ Return         :返回头结点指针
*@ attention      :
*******************************************************************************/
link_t* CreateLink(void)
{
    link_t* phead = (link_t* )malloc(sizeof(link_t));
    if (NULL == phead)
    {
        printf("create link fault !\n");
        return NULL;
    }
    memset(phead,0,sizeof(link_t));
    return phead;
	
}
 
 
/*******************************************************************************
*@ Description    :销毁单链表
*@ Input          :<phead>链表指针
					<free_data_flag> 是否对data数据指针进行free操作（0：不free ; 1：free）
*@ Output         :
*@ Return         :
*@ attention      :
*******************************************************************************/
int DestroyLink(link_t* phead,int free_data_flag)
{
    if ( NULL == phead )
    {
        return LINK_ERR;
    }
    link_t* pDel = NULL;
    link_t* pTmp = NULL;
    pDel= phead->pNext;
    while(NULL != pDel)
    {
        pTmp = pDel->pNext;
		if(1 == free_data_flag)
		{
			if(pDel->data) {free(pDel->data);pDel->data = NULL;}
		}
        free(pDel);
        pDel = pTmp;
    }
    free(phead);
    phead = NULL;
    pDel = NULL;
    pTmp = NULL;
	
    return LINK_OK;
}

/*******************************************************************************
*@ Description    :创建一个链表节点
*@ Input          :
*@ Output         :
*@ Return         :创建好的节点指针
*@ attention      :
*******************************************************************************/
link_t* CreateNode( data_t* tdata )
{
    link_t* pNode = NULL;
    pNode = (link_t*)malloc(sizeof(link_t));
    if ( NULL == pNode)
    {
        return NULL;
    }
	
    memset(pNode,0,sizeof(link_t));
    pNode->data = tdata;
	
    return pNode;
}
 
/*******************************************************************************
*@ Description    :链表中插入一个链表节点
*@ Input          :<phead> 链表头指针
					<idata>节点中的数据指针
					<Offset>插入的位置：HEAD(头插法) TAIL(尾插法)
*@ Output         :
*@ Return         :成功：0
					失败：-1
*@ attention      :
*******************************************************************************/
int InsertLinkItem(link_t* phead,data_t* idata,int Offset)
{
    if ( NULL == phead )
    {
        return LINK_ERR;
    }
    link_t* pN = CreateNode(idata);
    link_t* pTmp = phead;
    switch ( Offset )
    {
        case HEAD:
            pN->pNext = phead->pNext;
            phead->pNext = pN;
            break;
			
        case TAIL:
            while ( NULL != pTmp->pNext)
            {
                pTmp = pTmp->pNext;
            }
            pN->pNext = pTmp->pNext;
            pTmp->pNext = pN;
            break;
			
        default:   //尾插法
	        while( NULL != pTmp->pNext)
	        {
	           pTmp = pTmp->pNext; 
	        }
	        pN->pNext = pTmp->pNext;
	        pTmp->pNext = pN;
    }
	
    return LINK_OK;
}
 
 
/*******************************************************************************
*@ Description    :删除链表中一个节点
*@ Input          :<phead>链表头指针
					<Offset>删除的位置：HEAD(头删法) TAIL(尾删法)
*@ Output         :<pdata>返回删除节点中的数据
*@ Return         :成功：0
					失败：-1
*@ attention      :
*******************************************************************************/
int DelLinkItem(link_t* phead,data_t** pdata,int Offset)
{
    if ( (NULL == pdata) || (NULL == phead) || (NULL == phead->pNext))
    {
        return LINK_ERR;
    }
    link_t* pT = phead->pNext;
    link_t* pTmp = phead;
    link_t* pDel = NULL;
    switch ( Offset )
    {
        case HEAD:
            *pdata = pT->data;
            phead->pNext = pT->pNext;
            free(pT);
            pT = NULL;
            break;
			
        default:  //默认尾删法
            while (NULL != pTmp->pNext->pNext)
            {
				 pTmp = pTmp->pNext;
            }
            pDel = pTmp->pNext;
            *pdata = pDel->data;
            pTmp->pNext = pDel->pNext;
            free(pDel);
            pDel = NULL;
 
    }
    return LINK_OK;
}
 
/*******************************************************************************
*@ Description    :将链表中含有数据 OldData 的节点数据替换成 NewData
*@ Input          :<phead>链表头指针
					<NewData>要替换的新数据
*@ Output         :<OldData>返回之前的老数据
*@ Return         :失败：-1 ； 成功：0
*@ attention      :
*******************************************************************************/
/*
int UpdateLink(link_t* phead,data_t* OldData,data_t* NewData)
{
    if ( NULL == phead)
    {
        return LINK_ERR;
    }
	
    link_t* pTmp = phead->pNext;
    while ( NULL != pTmp)
    {
        if ( OldData == pTmp->data)
        {
            pTmp->data = NewData;
        }
        pTmp = pTmp->pNext;
    }
    return LINK_OK;
}
*/

/*******************************************************************************
*@ Description    :遍历输出链表元素的值
*@ Input          :<phead>链表头指针
*@ Output         :
*@ Return         :
*@ attention      :
*******************************************************************************/
/*
void ShowLink(link_t* phead)
{
    if ( NULL == phead)
    {
        return;
    }
	
    link_t* pTmp = phead->pNext;
    while ( NULL != pTmp)
    {
        printf("%#x ",pTmp->data);
        pTmp = pTmp->pNext;
    }

	printf("\n");
 
}

*/






