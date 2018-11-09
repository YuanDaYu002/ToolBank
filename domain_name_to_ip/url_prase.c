

#include <stdio.h> //printf 
#include <sys/socket.h> 
#include <netdb.h> //hostent 
#include <arpa/inet.h> 
#include <string.h>
#include <stdlib.h>

//#include "hle_url_prase.h"


typedef struct _url_info_t
{
    char*protocol;  //协议
    char*host;      //域名
    char*port;      //端口
    char*path;      //路径
    char*query;     //询问
    char*fragment; //片段
    
}url_info_t;

/**************************************************************************
URL info 结构的释放
参数：
    url_info：url_prase函数 解析后的url结构体指针
***************************************************************************/
int free_url_info(url_info_t*url_info)
{
    if(NULL == url_info)
        return -1;

    free((void*)url_info->protocol);
    free((void*)url_info->host);
    free((void*)url_info->port);
    free((void*)url_info->path);
    free((void*)url_info->query);
    free((void*)url_info->fragment);

    return 0;
}

/**************************************************************************
URL解析函数，精简版
参数：
    url（入）：url字符串指针
    url_info_ret（返回）：解析后的url结构体指针
注意，使用完后需要释放（free）url_info_ret的每一个成员，不然会产生内存泄漏。
return:
        success:0
        error:-1
//url示例： http://www.baidu.com:80/cig-bin/index.html?sdkfj#283sjkdf
***************************************************************************/
int url_prase(const char*url,url_info_t*url_info_ret)
{
    if(NULL == url)
        return -1;
    
    char *parseptr1;
    char *parseptr2;
    int len;
    int i;
  
 
    /*************解析协议*********************************/
    parseptr2 = strdup(url);
    char *free_pst = parseptr2;
    parseptr1 = strchr(parseptr2, ':');//在parseptr2中找“ ：”的位置，由parseptr1指向它开始的位置
    if ( NULL == parseptr1 ) 
    {
        printf("URL error!\n");
        free(parseptr2);
        return -1;
    }
  

    len = parseptr1 - parseptr2; //地址做差，得出 “ ：”之前字符串长度（字节数），即协议的长度
    for ( i = 0; i < len; i++ ) 
    {
        if ( !isalpha(parseptr2[i]) ) //判断字符ch是否为英文字母a-z A-Z ,是则返回非零值
        {
            printf("URL error!\n");
            free(parseptr2);
            return -1;
        }
    }
   /* printf("protocol: ");
    for(i=0;i<len;i++)
        printf("%c",parseptr2[i]);
    */
    url_info_ret->protocol = (char*)calloc(len,sizeof(char));
    strncpy(url_info_ret->protocol,parseptr2,len);
    //url_info_ret->protocol = strndup(parseptr2,len); 
    

    /*************解析host*********************************/
    printf("\n");
    parseptr1++; //指向 “ ：”的下一个字符，跳过“//”
    parseptr2 = parseptr1;
    for ( i = 0; i < 2; i++ ) 
    {
        if ( '/' != *parseptr2 ) 
        {
            printf("URL error!\n");
            free(parseptr2);
            free(url_info_ret->protocol);
            return -1;
        }
        parseptr2++;
    }
    parseptr1 = strchr(parseptr2, ':');
    if ( NULL == parseptr1 )//判断有无端口号
    {
        parseptr1 = strchr(parseptr2, '/'); //无端口号的 host解析
        if ( NULL == parseptr1 ) 
        {
            printf("URL error!\n");
            free(parseptr2);
            free(url_info_ret->protocol);
            return -1;
        }
        
        len = parseptr1 - parseptr2;//host长度
        /*printf("host: ");
        for(i=0;i<len;i++)
           printf("%c",parseptr2[i]);
        printf("\n");//解析主机
        */
        //url_info_ret->host = strndup(parseptr2,len);

        url_info_ret->host = (char*)calloc(len,sizeof(char));
        strncpy(url_info_ret->host,parseptr2,len);
    }
    else//有端口号
    {
        len = parseptr1 - parseptr2;
        
        /*printf("host: ");
        for(i=0;i<len;i++)
            printf("%c",parseptr2[i]);
        printf("\n");
        */
       // url_info_ret->host = strndup(parseptr2,len);
        url_info_ret->host = (char*)calloc(len,sizeof(char));
        strncpy(url_info_ret->host,parseptr2,len);
        
        /*************解析port*********************************/
        parseptr1++; //跳过端口前的 “ ：”
        parseptr2 = parseptr1;//基准位置从端口开始
        parseptr1 = strchr(parseptr2, '/');
        if ( NULL == parseptr1 ) 
        {
            printf("URL error!\n");
            free(parseptr2);
            free(url_info_ret->protocol);
            free(url_info_ret->host);
            return -1;
        }
        len = parseptr1 - parseptr2;
        /*
        printf("port: ");
        for(i=0;i<len;i++)
            printf("%d",(parseptr2[i]-48));
        printf("\n");//解析端口
        */
        //url_info_ret->port = strndup(parseptr2,len);
        url_info_ret->port = (char*)calloc(len,sizeof(char));
        strncpy(url_info_ret->port,parseptr2,len);
    }


    /*************解析path*********************************/
    parseptr1++; //跳过端口后边的“/”
    parseptr2 = parseptr1;//基准位置重新调整
    while ( '\0' != *parseptr1 && '?' != *parseptr1  && '#' != *parseptr1 ) 
    {
        parseptr1++;
    }
    
    len = parseptr1 - parseptr2;
    /*
    printf("path: ");
    for(i=0;i<len;i++)
           printf("%c",parseptr2[i]);
    printf("\n");//解析路径
    */
    //url_info_ret->path = strndup(parseptr2,len);
    url_info_ret->path = (char*)calloc(len,sizeof(char));
    strncpy(url_info_ret->path,parseptr2,len);
  

    /*************解析query*********************************/
    parseptr2=parseptr1;
    if ( '?' == *parseptr1 ) 
    {
        parseptr2++;
        parseptr1 = parseptr2;
        while ( '\0' != *parseptr1 && '#' != *parseptr1 ) 
        {
            parseptr1++;
        }
        len = parseptr1 - parseptr2;
        /*
        printf("query: ");
        for(i=0;i<len;i++)
            printf("%c",parseptr2[i]);//判断有无询问并解析
        printf("\n");
        */
        //url_info_ret->query = strndup(parseptr2,len);
        url_info_ret->query = (char*)calloc(len,sizeof(char));
        strncpy(url_info_ret->query,parseptr2,len);
    }
   
    /*************解析fragment*********************************/
    parseptr2=parseptr1;
    if ( '#' == *parseptr1 ) 
    {
        parseptr2++;
        parseptr1 = parseptr2;
        while ( '\0' != *parseptr1 ) 
        {
            parseptr1++;
        }
        len = parseptr1 - parseptr2;
        /*
        printf("fragment: ");
        for(i=0;i<len;i++)
            printf("%c",parseptr2[i]);
        printf("\n");//判断有无片段并解析
        */
        //url_info_ret->fragment = strndup(parseptr2,len);
        url_info_ret->fragment = (char*)calloc(len,sizeof(char));
        strncpy(url_info_ret->fragment,parseptr2,len);
    }


    free(free_pst);//释放备份url

    return 0;
}


/**************************************************************************
 Get ip from domain name
依据域名解析ip地址。 
参数：
    hostname（入）：域名字符串指针
    ip（返回）：返回ip字符串指针 （需提前申请空间，再传地址）

***************************************************************************/
#include<netdb.h> //hostent 
#include<arpa/inet.h> 
int hostname_to_ip(char * hostname , char* ip) 
{ 
    struct hostent *he; 
    struct in_addr **addr_list; 
    int i; 
  
    if ( (he = gethostbyname( hostname ) ) == NULL)  
    { 
        // get the host info 
        perror("gethostbyname"); 
        return 1; 
    } 

    addr_list = (struct in_addr **) he->h_addr_list; 
    for(i = 0; addr_list[i] != NULL; i++)  
    { 
        //Return the first one; 
        strcpy(ip , inet_ntoa(*addr_list[i]) ); 
        return 0; 
    } 

    return 1; 
}


/**************************************************************************
 hle Get ip from url
依据域名解析ip地址。 
参数：
    url（入）：域名字符串指针
    ip（返回）：返回ip字符串指针 （需提前申请空间，再传地址）
    port:port

***************************************************************************/
char hle_ip[20];
char hle_port[10];
int hle_url_to_ip_port(char * url,char*ip,char*port)
{
    if(NULL == url||NULL == ip)
        return -1;
    url_info_t url_info_ret = {0};

    int ret = url_prase(url,&url_info_ret);
    if(ret < 0)
        perror("url_prase failed!\n");

#if 1
    hostname_to_ip(url_info_ret.host,ip);

    if(url_info_ret.port != NULL)
    {
        strcpy(port,url_info_ret.port);
    }

    printf("the url = %s \n",url);

    printf("protocol = %s\n",url_info_ret.protocol);
    printf("host = %s\n",url_info_ret.host);
    printf("port = %s\n",url_info_ret.port);
    printf("path = %s\n",url_info_ret.path);
    printf("query = %s\n",url_info_ret.query);
    printf("fragment = %s\n",url_info_ret.fragment);

    strcpy(hle_ip,ip);
    strcpy(hle_port,port);
#endif


    return 0;

}


int main (int argc,char**argv)
{

    char *url = "http://www.baidu.com/index.html";
    //printf("argv[0] = %s\n",argv[0]);
    char ip [20] = {0};
    char port[10] = {0};
    hle_url_to_ip_port(url,ip,port);



    return 0;

}


