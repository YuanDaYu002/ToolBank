
#ifndef URL_PRASE_H
#define URL_PRASE_H

typedef struct _url_info_t
{
	char*protocol;	//协议
	char*host;		//域名
	char*port;		//端口
	char*path;		//路径
	char*query;		//询问
	char*fragment; //片段
	
}url_info_t;

// char hle_ip[20];
// char hle_port[10];

/**************************************************************************
URL解析函数，精简版
参数：
	url（入）：url字符串指针
	url_info_ret（返回）：解析后的url结构体指针
注意，使用完后需要释放（free）url_info_ret的每一个成员，不然会产生内存泄漏。
//url示例： http://www.baidu.com:80/cig-bin/index.html?sdkfj#283sjkdf
***************************************************************************/
int url_prase(const char*url,url_info_t*url_info_ret);

/**************************************************************************
 Get ip from domain name
依据域名解析ip地址。 
参数：
	hostname（入）：域名字符串指针
	ip（返回）：返回ip字符串指针 （需提前申请空间，再传地址）

***************************************************************************/
int hostname_to_ip(char * hostname , char* ip); 

/**************************************************************************
 hle Get ip from url
依据域名解析ip地址。 
参数：
    url（入）：域名字符串指针
    ip（返回）：返回ip字符串指针 （需提前申请空间，再传地址）
        有端口号会附带端口号（eg：112.112.112.112:80）
		char ip [20];
***************************************************************************/
int hle_url_to_ip_port(char * url,char*ip);


#endif
