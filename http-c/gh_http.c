#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include "gh_http.h"
#include "tcpclient.h"

/*******************************************************************************
*@ Description    :
*@ Input          :<pclient>:post 本地客户端的描述信息
					<page>: URL
					<message_json>要推送的字符数据（http头的数据部分）
					<filepath>文件的路径
					
*@ Output         :<response>服务器返回的响应包数据
*@ Return         :成功：0 失败: <0的数
*@ attention      :
*******************************************************************************/
static int http_post_file(tcpclient *pclient, const char *page,
     const char* message_json, const char *filepath,
     char **response)
{

    //check if the file is valid or not
    struct stat stat_buf;
    if(lstat(filepath,&stat_buf)<0)//获取文件的相关信息
	{
        GH_LOG_INFO("lstat %s fail", filepath);
        return -1;
    }
    if(!S_ISREG(stat_buf.st_mode)) //判断该文件是否是一个常规文件
	{
        GH_LOG_INFO("%s is not a regular file!",filepath);
        return  -1;
    }
 
    char *filename;
    filename=strrchr((char*)filepath,'/'); //查找filepath中最后一次出现字符‘/’的位置
    if(filename==NULL)
    {
 
    }
    filename+=1;
    if(filename>=filepath+strlen(filepath))
	{
        //'/' is the last character
        GH_LOG_INFO("%s is not a correct file!",filepath);
        return  -1;
    }
 
    //GH_LOG_INFO("filepath=%s,filename=%s",filepath,filename);
 
    char content_type[4096];  //用于构造 http post 的头部 content_type 信息
    memset(content_type, 0, 4096);
 
    char post[512]; //http 头 POST 字段
	char host[256]; //http 头 HOST 字段
    char *lpbuf;//指向服务器返回的数据
	char *ptmp;
    int len=0;
 
    lpbuf = NULL;
    const char *header2="User-Agent: Is Http 1.1\r\nCache-Control: no-cache\r\nAccept: */*\r\n";
 
    sprintf(post,"POST %s HTTP/1.1\r\n",page);
    sprintf(host,"HOST: %s:%d\r\n",pclient->remote_ip,pclient->remote_port);
    strcpy(content_type,post);
    strcat(content_type,host);
 
 
    char *boundary = (char *)"-----------------------8d9ab1c50066a";
 
    strcat(content_type, "Content-Type: multipart/form-data; boundary=");
    strcat(content_type, boundary);
    strcat(content_type, "\r\n");

    //--Construct request data {filePath, file}
    char content_before[8192];
    memset(content_before, 0, 8192);

    //GH_LOG_TEXT(message_json);
    //附加数据。
    strcat(content_before, "--");
    strcat(content_before, boundary);
    strcat(content_before, "\r\n");
    strcat(content_before, "Content-Disposition: form-data; name=\"consite_message\"\r\n\r\n");
    strcat(content_before, message_json);
    strcat(content_before, "\r\n");

    strcat(content_before, "--");
    strcat(content_before, boundary);
    strcat(content_before, "\r\n");
    strcat(content_before, "Content-Disposition: attachment; name=\"file\"; filename=\"");
    strcat(content_before, filename);
    strcat(content_before, "\"\r\n");
    strcat(content_before, "Content-Type: image/jpeg\r\n\r\n");

    char content_end[2048];
    memset(content_end, 0, 2048);
    strcat(content_end, "\r\n");
    strcat(content_end, "--");
    strcat(content_end, boundary);
    strcat(content_end, "--\r\n");

    int max_cont_len=5*1024*1024;  //用来缓存文件-----------太大了，后续可以修改
    char content[max_cont_len];
    int fd;
    fd=open(filepath,O_RDONLY,0666);
    if(!fd){
        GH_LOG_INFO("fail to open file : %s",filepath);
        return -1;
    }
    len=read(fd,content,max_cont_len);
    close(fd);

    char *lenstr;//字符串总长度
    lenstr = (char*)malloc(256);
    sprintf(lenstr, "%d", (int)(strlen(content_before)+len+strlen(content_end)));
 
    strcat(content_type, "Content-Length: ");
    strcat(content_type, lenstr);
    strcat(content_type, "\r\n\r\n");
    free(lenstr);

    //send
    if (!pclient->connected && tcpclient_conn(pclient) == -1)
    {
        return -1;
    }
 
    //content-type
    tcpclient_send(pclient,content_type,strlen(content_type));
 
    //content-before
    tcpclient_send(pclient,content_before,strlen(content_before));
 
    //content
    tcpclient_send(pclient,content,len);
 
    //content-end
    tcpclient_send(pclient,content_end,strlen(content_end));

/*---#进入服务器响应包解析过程------------------------------------------------------------*/
    /*it's time to recv from server*/
    if(tcpclient_recv(pclient,&lpbuf,0) <= 0)
	{
        if(lpbuf) free(lpbuf);
        return -2;
    }

	/*---#对返回数据进行合法检验------------------------------------------------------------*/
    //GH_LOG_TEXT(lpbuf);
 
        /*响应代码,|HTTP/1.0 200 OK|
         *从第10个字符开始,第3位
         * */
        //GH_LOG_INFO("responsed:\n%s",lpbuf);
        memset(post,0,sizeof(post));
        strncpy(post,lpbuf+9,3);
        if(atoi(post)!=200)
		{
			ERROR_LOG("atoi(post)!=200");
            if(lpbuf) free(lpbuf);
            return atoi(post);
        }
 
        ptmp = (char*)strstr(lpbuf,"\r\n\r\n");
        if(ptmp == NULL)
		{
            free(lpbuf);
            return -3;
        }
        ptmp += 4;//跳过\r\n
        
		/*---#前边为http的头部分------------------------------------------------------------*/
		/*---#拷贝真实数据给返回参数------------------------------------------------------------*/
        
        len = strlen(ptmp)+1;
        *response=(char*)malloc(len);
        if(*response == NULL)
		{
            if(lpbuf) free(lpbuf);
            return -1;
        }
        memset(*response,0,len);
        memcpy(*response,ptmp,len-1);

        //从头域找到内容长度,如果没有找到则不处理
        ptmp = (char*)strstr(lpbuf,"Content-Length:");
        if(ptmp != NULL)
		{
            char *ptmp2;
            ptmp += 15;
            ptmp2 = (char*)strstr(ptmp,"\r\n");
            if(ptmp2 != NULL)//将内容长度部分的字符串摘出来并转换成int型数据
			{
                memset(post,0,sizeof(post));
                strncpy(post,ptmp,ptmp2-ptmp);
                if(atoi(post)<len)
                    (*response)[atoi(post)] = '\0';
            }
        }

        if(lpbuf) free(lpbuf);
        return 0;
 
}

/*******************************************************************************
*@ Description    : http推送函数
*@ Input          :<pclient>本地客户端描述信息
					<page> 服务端的URL
					<message_json>http头要捎带的客户端数据（JSON字符串）
					<ret_code>
					<response>
*@ Output         :
*@ Return         :
*@ attention      :
*******************************************************************************/
static int http_post(tcpclient *pclient, const char *page,
 const const char* message_json, int* ret_code, char **response)
{
    char content_buffer[4096] = {0};
 
    char post[512]={0};
    char host[256]={0};
    char* lpbuf = NULL;
    char* ptmp  = NULL;

    const char *header2="Connection: keep-alive\r\nAccept-Encoding: gzip, deflate\r\nAccept: */*\r\nUser-Agent: python-requests/2.18.4\r\n";

    sprintf(post,"POST %s HTTP/1.1\r\n",page);
    strcpy(content_buffer, post);

    sprintf(host,"HOST: %s:%d\r\n",pclient->remote_ip,pclient->remote_port);
    strcat(content_buffer, host);
    strcat(content_buffer, header2);

    char *lenstr;
    lenstr = (char*)GH_MEM_MALLOC(256);
    sprintf(lenstr, "%d", (int)(strlen(message_json)));

    strcat(content_buffer, "Content-Length: ");
    strcat(content_buffer, lenstr);
    strcat(content_buffer, "\r\n");

    char content_type[4096] = {0};
    strcat(content_type, "Content-Type: application/json");
    strcat(content_type, "\r\n\r\n");
    strcat(content_type, message_json);

    strcat(content_buffer, content_type);
    free(lenstr);

    //send
    if (!pclient->connected)
    {
        if (tcpclient_conn(pclient) == -1)
        {
            return -1;
        }
    }

    //content-type
    tcpclient_send(pclient, content_buffer, strlen(content_buffer));
 
    /*it's time to recv from server*/
    if(tcpclient_recv(pclient,&lpbuf,0) <= 0){
        if(lpbuf) free(lpbuf);
        return -2;
    }

        /*响应代码,|HTTP/1.0 200 OK|
         *从第10个字符开始,第3位
         * */
        //GH_LOG_INFO("responsed:\n%s",lpbuf);
        memset(post,0,sizeof(post));
        strncpy(post,lpbuf+9,3);
        *ret_code = atoi(post);
        if(*ret_code!=200){
            if(lpbuf) free(lpbuf);
            return *ret_code;
        }
 
        ptmp = (char*)strstr(lpbuf,"\r\n\r\n");
        if(ptmp == NULL){
            free(lpbuf);
            return -3;
        }
        ptmp += 4;//跳过\r\n

        int len = strlen(ptmp)+1;
        *response=(char*)GH_MEM_MALLOC(len);
        if(*response == NULL){
            if(lpbuf) free(lpbuf);
            return -1;
        }
        memset(*response,0,len);
        memcpy(*response,ptmp,len-1);

        //从头域找到内容长度,如果没有找到则不处理
        ptmp = (char*)strstr(lpbuf,"Content-Length:");
        if(ptmp != NULL){
            char *ptmp2;
            ptmp += 15;
            ptmp2 = (char*)strstr(ptmp,"\r\n");
            if(ptmp2 != NULL){
                memset(post,0,sizeof(post));
                strncpy(post,ptmp,ptmp2-ptmp);
                if(atoi(post)<len)
                    (*response)[atoi(post)] = '\0';
            }
        }

        if(lpbuf) free(lpbuf);

        return 0;
}

/*******************************************************************************
*@ Description    :
*@ Input          :<pHost>服务端的域名
					<nPort> TCP通信的端口
					<pServerPath> 服务端的 URL
					<pMessageJson> 要推送的 字符数据
					
*@ Output         :
*@ Return         :
*@ attention      :
*******************************************************************************/
int http_post_data(const char* pHost, const int nPort, const char* pServerPath,
                     const char* pMessageJson, int* ret_code, char** ret_buffer)
{
 
    tcpclient client;
 
    //GH_LOG_INFO("开始组包%s", pFile);
    tcpclient_create(&client, pHost, nPort);
 
//    if(http_post(&client,"/recv_file.php","f1=hello",&response)){
//        GH_LOG_INFO("失败!");
//        exit(2);
//    }
    http_post(&client, pServerPath, pMessageJson, ret_code, ret_buffer);
 
    //GH_LOG_INFO("responsed %zu:%s", strlen(response), response);
 
    tcpclient_close(&client);
    return 0;
}

/*******************************************************************************
*@ Description    : HTTP上传文件
*@ Input          :<pHost>服务端的域名
					<nPort> TCP通信的端口
					<pServerPath> 服务端的 URL
					<pMessageJson> 要推送的 字符数据
					<pFile> 要上传的文件
*@ Output         :
*@ Return         :成功：0 
*@ attention      :
*******************************************************************************/
int http_upload_file(const char* pHost, const int nPort, const char* pServerPath,
                     const char* pMessageJson, const char* pFile)
{
 
    tcpclient client;
 
    char *response = NULL;
    //GH_LOG_INFO("开始组包%s", pFile);
    tcpclient_create(&client, pHost, nPort);
 
//    if(http_post(&client,"/recv_file.php","f1=hello",&response)){
//        GH_LOG_INFO("失败!");
//        exit(2);
//    }
    http_post_file(&client, pServerPath, pMessageJson, pFile, &response);
 
    //GH_LOG_INFO("responsed %zu:%s", strlen(response), response);
 
    free(response);

    tcpclient_close(&client);
    return 0;
}


