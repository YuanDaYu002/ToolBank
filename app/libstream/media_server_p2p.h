/*********************************************************************************
  *FileName: media_server_p2p.h
  *Create Date: 2018/09/06
  *Description: 本文件是p2p传输的头文件，基于shangyun SDK进行封装。 
  *Others:  
  *History:  
**********************************************************************************/
#ifndef MEDIA_SERVER_P2P_H
#define MEDIA_SERVER_P2P_H

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "typeport.h"
#include "in.h"





#define CH_CMD				0	//信令传输通道（P2P）
#define CH_STREAM			1	//编码流传输通道（P2P）
#define CH_DATA				1
#define SERVER_NUM			3	// WakeUp Server Number
#define SIZE_DID 			30	// Device ID Size
#define SIZE_APILICENSE 	24	// APILicense Size
#define SIZE_INITSTRING 	256	// InitString Size
#define SIZE_WAKEUP_KEY 	17	// WakeUp Key Size

#define MAX_CLIENT_NUM 		10  //最大能同时接入的客户端数量
#define P2P_TSK_PRIO        4	//P2P线程的创建优先级（LiteOS下）
//#define P2P_SUPORT_WAKEUP     //放在makefile中定义了

// show info 开关 -> 终端打印调试信息(尚云)
//#define ST_INFO_OPEN   1
#define ST_INFO_OPEN   0


#ifndef P2P_SUPORT_WAKEUP
//standard p2p 参数（不支持唤醒,适用Linux系统）
#define _DID_ 				"PPCS-016947-YFYNG"
#define APILICENSE 			"BBEHHJ"
#define CRCKEY 				"EasyView"
#define INITSTRING 			"EBGAEIBIKHJJGFJKEOGCFAEPHPMAHONDGJFPBKCPAJJMLFKBDBAGCJPBGOLKIKLKAJMJKFDOOFMOBECEJIMM"

#else
//advanced p2p 参数（支持唤醒.适用RTOS系统,LiteOS属于RTOS系统）
#define _DID_ 				"RTOS-000236-STWDB"
#define APILICENSE 			"ATTWPQ"
#define CRCKEY 				"EasyView"
#define INITSTRING 			"EFGBFFBJKEJKGGJJEEGFFHELHHNNHONHGLFNBHCCAEJDLNLPDDAGCIOFGDLGJMLAAOMOKCDLOONOBICJJIMM "
#define WAKEUPKEY 			"1234567890ABCDEF"			
#define IP_LENGTH			16
#define TCP_PORT			12306
#define UDP_PORT			12305
#define SERVER_IP1			"112.74.108.149"
#define SERVER_IP2			"112.74.108.149"
#define SERVER_IP3			"112.74.108.149"

#endif

//P2P_wait_for_wakeup函数返回状态
enum P2P_wait_for_wakeup_status
{
	SERVERS_OFFLINE = -4,		//服务器失去连接
	HAVE_CLIENT_CONNECTED = -3, //还有客户端连接，不能进入睡眠状态等待唤醒
	NOT_SUPPORT_WAKEUP = -2,	//（当前模式）不支持唤醒
	WAKEUP_ERR = -1,			//唤醒出错（失败）	
	WAKEUP_SUCC = 0				//唤醒成功
	
};

typedef struct _p2p_handle_t
{
	HLE_S8 *Did;			 					//DID: This is Device ID, such as: ABCD-123456-ABCDEF
	HLE_S8 APILicense[SIZE_APILICENSE];	 		//APILicense: If you set up CRCKey, this parameter must enter APILicense:CRCKey, if not, you only need to enter APILicense
	//HLE_S8 *CRCKey;		 					//[CRCKey] //和APILicense用“ ：”拼装在一块
	HLE_S8 *InitString;	 						//tInitString: The encrypted string of the server IP address or the domain name.
#ifdef P2P_SUPORT_WAKEUP
	HLE_S8 *WakeupKey; 	 						//[WakeupKey]: Wakeup Server String Key
	HLE_S8 IP[SERVER_NUM][IP_LENGTH];			//[IP1],[IP2],[IP3]: Three Wakeup Server IP or domain name, You need to enter at least one IP
	struct sockaddr_in serveraddr[SERVER_NUM];			//build the server's Internet address
	HLE_S32 server_index;						//当前正建立连接的唤醒服务器在serveraddr数组中的下标
	HLE_S32 skt;								//和唤醒服务器建立TCP连接的socket。
#endif
	
	HLE_S32 Session_num;						//当前已经连接的会话数量（客户端数量）,不能大于MAX_CLIENT_NUM
	pthread_mutex_t lock;
	HLE_S32 SessionID;							//与客户端的会话ID
}p2p_handle_t;

extern  unsigned long getTickCount(void);

/*以下函数为基于善云SDK ListenTester.cpp文件进行修改后封装的函数*/

//init  初始化配置参数 + P2P初始化。
HLE_S32 p2p_init(p2p_handle_t *P2P_handle);

//connect
HLE_S32 p2p_conect(p2p_handle_t *P2P_handle);

//wakeup 
HLE_S32 P2P_wait_for_wakeup(p2p_handle_t *P2P_handle);


//listen
HLE_S32 p2p_listen(p2p_handle_t *P2P_handle);

//客户端任务入口函数（每个客户端接入。都会创建一个线程，专程处理该客户端的业务）。
HLE_S32 P2P_client_task_create(p2p_handle_t *P2P_handle);

void * P2P_client_task_func(void*P2P_handle);

//recv
HLE_S32 p2p_recv(void *handle,void*buf,HLE_S32 length);

//send
HLE_S32 p2p_send(void *handle,const void*data,HLE_S32 length);

//close
HLE_S32 p2p_close(void *handle); 

const HLE_S8 *getP2PErrorCodeInfo(HLE_S32 err);
void st_info(const HLE_S8 *format, ...);





#endif



















