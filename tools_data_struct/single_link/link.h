 
/***************************************************************************
* @file: link.h 
* @author:   
* @date:  4,22,2019
* @brief:  单链表
* @attention:
***************************************************************************/

#ifndef  _LINK_H_
#define  _LINK_H_

#define HEAD 0
#define TAIL -1
typedef enum LINK_OP
{
    LINK_ERR = -1,
    LINK_OK
}LINK_OP_ENUM;
	
 
typedef  char  data_t;
typedef  struct  _link_t
{
    data_t * 			data; 		//负载数据的指针
    struct _link_t*		pNext;		//后继节点的指针
}link_t;
  
 
link_t* CreateLink(void);
int DestroyLink(link_t* phead,int free_data_flag);
link_t* CreateNode( data_t*  tdata );
int InsertLinkItem(link_t * phead ,  data_t*  idata, int  Offset);
int DelLinkItem(link_t * phead,  data_t **pdata, int  Offset);
//int UpdateLink(link_t * phead,  data_t  OldData, data_t  NewData);
//void ShowLink(link_t * phead);


#endif /* _LINK_H_ */
