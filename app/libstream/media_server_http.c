#include <stdio.h>//printf
#include <string.h>//字符串处理
#include <sys/socket.h>//套接字
#include <arpa/inet.h>//ip地址处理
#include <fcntl.h>//open系统调用
#include <unistd.h>//write系统调用
#include <netdb.h>//查询DNS
#include <sys/stat.h>//stat系统调用获取文件大小
#include <sys/time.h>//获取下载时间
#include <errno.h>
#include <stdlib.h>


struct HTTP_RES_HEADER//保持相应头信息
{
    int status_code;//HTTP/1.1 '200' OK
    char content_type[128];//Content-Type: application/gzip
    long content_length;//Content-Length: 11683079
};

typedef struct _updateInfo_t
{
  char * downloadUrl;   //升级包下载连接
  int   isVerify ;      //暂时不用
  char *md5Value;       //升级包内容MD5加密值
  char *verion;         //最新固件包版本号
  char *size;           //升级包大小
}updateInfo_t;



void parse_url(const char *url, char *host, int *port, char *file_name)
{
    /*通过url解析出域名, 端口, 以及文件名*/
    int j = 0;
    int i;
    int start = 0;
    *port = 80;
    char *patterns[] = {"http://", "https://", NULL};

    for (i = 0; patterns[i]; i++)//分离下载地址中的http协议
        if (strncmp(url, patterns[i], strlen(patterns[i])) == 0)
            start = strlen(patterns[i]);

    //解析域名, 这里处理时域名后面的端口号会保留
    for (i = start; url[i] != '/' && url[i] != '\0'; i++, j++)
        host[j] = url[i];
    host[j] = '\0';

    //解析端口号, 如果没有, 那么设置端口为80
    char *pos = strstr(host, ":");
    if (pos)
        sscanf(pos, ":%d", port);

    //删除域名端口号
    for (i = 0; i < (int)strlen(host); i++)
    {
        if (host[i] == ':')
        {
            host[i] = '\0';
            break;
        }
    }

    //获取下载文件名
    j = 0;
    for (i = start; url[i] != '\0'; i++)
    {
        if (url[i] == '/')
        {
            if (i !=  strlen(url) - 1)
                j = 0;
            continue;
        }
        else
            file_name[j++] = url[i];
    }
    file_name[j] = '\0';
}

struct HTTP_RES_HEADER parse_header(const char *response)
{
    /*获取响应头的信息*/
    struct HTTP_RES_HEADER resp;

    char *pos = strstr(response, "HTTP/");
    if (pos)//获取返回代码
        sscanf(pos, "%*s %d", &resp.status_code);

    pos = strstr(response, "Content-Type:");
    if (pos)//获取返回文档类型
        sscanf(pos, "%*s %s", resp.content_type);

    pos = strstr(response, "Content-Length:");
    if (pos)//获取返回文档长度
        sscanf(pos, "%*s %ld", &resp.content_length);

    return resp;
}

void get_ip_addr(char *host_name, char *ip_addr)
{
    /*通过域名得到相应的ip地址*/
    int i;
    struct hostent *host = gethostbyname(host_name);//此函数将会访问DNS服务器
    if (!host)
    {
        ip_addr = NULL;
        return;
    }

    for (i = 0; host->h_addr_list[i]; i++)
    {
        strcpy(ip_addr, inet_ntoa( * (struct in_addr*) host->h_addr_list[i]));
        break;
    }
}


void progress_bar(long cur_size, long total_size, double speed)
{
    /*用于显示下载进度条*/
    float percent = (float) cur_size / total_size;
    const int numTotal = 50;
    int numShow = (int)(numTotal * percent);

    if (numShow == 0)
        numShow = 1;

    if (numShow > numTotal)
        numShow = numTotal;

    char sign[51] = {0};
    memset(sign, '=', numTotal);

    printf("\r%.2f%%[%-*.*s] %.2f/%.2fMB %4.0fkb/s", percent * 100, numTotal, numShow, sign, cur_size / 1024.0 / 1024.0, total_size / 1024.0 / 1024.0, speed);
    fflush(stdout);

    if (numShow == numTotal)
        printf("\n");
}

unsigned long get_file_size(const char *filename)
{
    //通过系统调用直接得到文件的大小
    struct stat buf;
    if (stat(filename, &buf) < 0)
        return 0;
    return (unsigned long) buf.st_size;
}

void download(int client_socket, char *file_name, long content_length)
{
    /*下载文件函数*/
    long hasrecieve = 0;//记录已经下载的长度
    struct timeval t_start, t_end;//记录一次读取的时间起点和终点, 计算速度
    int mem_size = 8192;//缓冲区大小8K
    int buf_len = mem_size;//理想状态每次读取8K大小的字节流
    int len;

    //创建文件描述符
    int fd = open(file_name, O_CREAT | O_WRONLY, S_IRWXG | S_IRWXO | S_IRWXU);
    if (fd < 0)
    {
        printf("create file error!\n");
        return;
    }

    char *buf = (char *) malloc(mem_size * sizeof(char));

    //从套接字流中读取文件流
    long diff = 0;
    int prelen = 0;
    double speed;

    while (hasrecieve < content_length)
    {
        gettimeofday(&t_start, NULL ); //获取开始时间
        len = read(client_socket, buf, buf_len);
        write(fd, buf, len);
        gettimeofday(&t_end, NULL ); //获取结束时间

        hasrecieve += len;//更新已经下载的长度

        //计算速度
        if (t_end.tv_usec - t_start.tv_usec >= 0 &&  t_end.tv_sec - t_start.tv_sec >= 0)
            diff += 1000000 * ( t_end.tv_sec - t_start.tv_sec ) + (t_end.tv_usec - t_start.tv_usec);//us

        if (diff >= 1000000)//当一个时间段大于1s=1000000us时, 计算一次速度
        {
            speed = (double)(hasrecieve - prelen) / (double)diff * (1000000.0 / 1024.0);
            prelen = hasrecieve;//清零下载量
            diff = 0;//清零时间段长度
        }

        progress_bar(hasrecieve, content_length, speed);

        if (hasrecieve == content_length)
            break;
    }
}

/*****************************************************************
*函数名 ：     http_dowload_file
*功能描述 ：系统升级下载升级包文件
*参数 ：   argc:输入参数个数
*        argv[0]:升级文件的所在远端服务器的 url   
*返回值 ：     成功：下载的文件绝对路径 字符串指针 
*          失败：NULL
*注意，返回的指针使用后需要free
*****************************************************************/
char * http_dowload_file(int argc, char *argv[])
{
    char url[2048] = "127.0.0.1";//设置默认地址为本机,
    char host[64] = {0};//远程主机地址
    char ip_addr[16] = {0};//远程主机IP地址
    int port = 80;//远程主机端口, http默认80端口
    char file_name[256] = {0};//下载文件名

    if (argc < 1)
    {
        printf("You must give an HTTP address to get started! \n");
        return NULL;
    }
    else
        strcpy(url, argv[0]);
    printf("argv[0] : %s\n",argv[0]);
    
    strcpy(file_name,"/jffs0/");
    
    puts("1:Parsing the download address...");
    parse_url(url, host, &port, &file_name[7]);//从url中分析出主机名, 端口号, 文件名
    
    #if 0
    if (argc == 2)
    {
        printf("\tYou have specified the download file name as: %s\n", argv[2]);
        strcat(file_name,argv[1]);
    }
    #endif

    puts("2: Getting the remote server IP address...");
    get_ip_addr(host, ip_addr);//调用函数同访问DNS服务器获取远程主机的IP
    if (strlen(ip_addr) == 0)
    {
        printf("Error: could not get IP address of remote server. Please check the validity of download address\n");
        return NULL;
    }

    puts("\n>>>>Download address resolution succeeded<<<<");
    printf("\tdownload link: %s\n", url);
    printf("\tremote server: %s\n", host);
    printf("\tIP address: %s\n", ip_addr);
    printf("\thost PORT: %d\n", port);
    printf("\tfile name : %s\n\n", file_name);

    //设置http请求头信息
    char header[2048] = {0};
    sprintf(header, \
            "GET %s HTTP/1.1\r\n"\
            "Accept: text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,*/*;q=0.8\r\n"\
            "User-Agent: Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537(KHTML, like Gecko) Chrome/47.0.2526Safari/537.36\r\n"\
            "Host: %s\r\n"\
            "Connection: keep-alive\r\n"\
            "\r\n"\
        ,url, host);


    puts("3: Create a network socket...");
    int client_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (client_socket < 0)
    {
        printf("Create a network socket failed: %d\n", client_socket);
        return NULL;
    }

    //创建IP地址结构体 IPV4
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr(ip_addr);
    addr.sin_port = htons(port);

    //连接远程主机
    puts("4: Connecting to remote host...");
    int res = connect(client_socket, (struct sockaddr *) &addr, sizeof(addr));
    if (res == -1)
    {
        printf("Connecting to remote host failed, error: %d\n", res);
        shutdown(client_socket, 2);
        return NULL;
    }

    puts("5: Sending HTTP download request...");
    write(client_socket, header, strlen(header));//write系统调用, 将请求header发送给服务器


    int mem_size = 4096;
    int length = 0;
    int len,i;
    char *buf = (char *) malloc(mem_size * sizeof(char));
    char *response = (char *) malloc(mem_size * sizeof(char));

    //每次单个字符读取响应头信息
    puts("6: Parsing the HTTP response header...");
    while ((len = read(client_socket, buf, 1)) != 0)
    {
        if (length + len > mem_size)
        {
            //动态内存申请, 因为无法确定响应头内容长度
            mem_size *= 2;
            char * temp = (char *) realloc(response, sizeof(char) * mem_size);
            if (temp == NULL)
            {
                printf("realloc failed!\n");
                free(buf);
                free(response);
                shutdown(client_socket, 2);
                return NULL;
            }
            response = temp;
        }

        buf[len] = '\0';
        strcat(response, buf);

        //找到响应头的头部信息
        int flag = 0;
        for (i = strlen(response) - 1; response[i] == '\n' || response[i] == '\r'; i--, flag++);
        if (flag == 4)//连续两个换行和回车表示已经到达响应头的头尾, 即将出现的就是需要下载的内容
            break;

        length += len;
    }

    struct HTTP_RES_HEADER resp = parse_header(response);

    printf("\n>>>>HTTP response header parsing was successful:<<<<\n");

    printf("\tHTTP response code: %d\n", resp.status_code);
    if (resp.status_code != 200)
    {
        printf("The file cannot be downloaded and the remote host returns: %d\n", resp.status_code);
        free(buf);
        free(response);
        shutdown(client_socket, 2);
        return NULL;
    }
    printf("\tHTTP file type: %s\n", resp.content_type);
    printf("\tHTTP body length: %ldBytes\n\n", resp.content_length);


    printf("7:Start file download...\n");
    download(client_socket, file_name, resp.content_length);
    printf("8: close socket\n");

    if (resp.content_length == get_file_size(file_name))
        printf("\nFile %s downloaded successfully!^_^\n\n", file_name);
    else
    {
        remove(file_name);
        printf("\nMissing bytes in file download. Download failed. Please try again!\n\n");
        free(buf);
        free(response);
        shutdown(client_socket, 2);
        return NULL;
    }
    
    shutdown(client_socket, 2);//关闭套接字的接收和发送
    
    char *doeload_file = strdup(file_name);
    return doeload_file;
}


/*****************************************************************
*函数名 ：     http_post
*功能描述 ：http发送post请求
*参数 ：   host_url:远端服务器的 设备固件升级请求URL

*返回值 ：     成功：char* 的数据指针（指向服务器响应头的头部信息） 失败：NULL
*注意：返回的指针使用后需要free
*****************************************************************/
#define PORT 80  
#define BUFSIZE 2048

//请求url:
#define DEVELOP_URL "http://hle1879.picp.net:9020/device/get-info/downloadFirmware"             //开发环境
#define TEST_URL    "http://www.pethin.com:9080/device/get-info/downloadFirmware"               //测试环境
#define PRODUCT_URL "http://service.home-linking.com:9010/device/get-info/downloadFirmware"     //生产环境
char* http_post(char *host_url)
{
    if(NULL == host_url)
        return NULL;
    
    int sockfd, ret , i, h;
    struct sockaddr_in servaddr;
    char str[4],str1[2048], str2[256];
    char url[1024] = "127.0.0.1";//设置默认地址为本机,
    char host[64] = {0};//远程主机地址
    char ip_addr[16] = {0};//远程主机IP地址
    int port = 80;//远程主机端口, http默认80端口
    char file_name[256] = {0};//下载文件名
    char *recv_buf = malloc(BUFSIZE);
    if(NULL == recv_buf)
    {
        printf("<http> Error: malloc failed!\n");
        return NULL;
    }
            
    memset(recv_buf,0,sizeof(BUFSIZE));
    
    socklen_t len;
    fd_set   t_set1;
    struct timeval  tv;


    strcpy(url, host_url);

    puts("1:Parsing the POST address...");
    parse_url(url,host,&port,file_name);

    puts("2: Getting the remote server IP address...");
    get_ip_addr(host, ip_addr);//调用函数同访问DNS服务器获取远程主机的IP
    if (strlen(ip_addr) == 0)
    {
        printf("<http> Error: could not get IP address of remote server. Please check the validity of download address\n");
        return NULL;
    }
    puts(">>>>POST address resolution succeeded<<<<");
    printf("\tPOST link: %s\n", url);
    printf("\tremote server: %s\n", host);
    printf("\tIP address: %s\n", ip_addr);
    printf("\thost PORT: %d\n", port);
    printf("\tfile name : %s\n\n", file_name);

    puts("3: Create a network socket...");
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0 ) 
    {
            printf("<http> socket error!\n");
            return NULL;
    }

    //bzero(&servaddr, sizeof(servaddr));
    memset(&servaddr,0,sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(port);
    if (inet_pton(AF_INET, ip_addr, &servaddr.sin_addr) <= 0 )
    {
            printf("<http> inet_pton error!\n");
            return NULL;
    }

    puts("4: Connecting to remote host...");
    if (connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0)
    {
            printf("<http> Connecting to remote host failed! \n");
            return NULL;
    }
    puts(">>>>Connecting to remote host succeeded<<<<\n");

    memset(str2, 0, 256);
    //构建post到服务器端的数据    
    strcat(str2, "{\n\"verion\": \"1.1\",\n\"firmwareType\":\"AQD17S20WW50\",\n\"firmwareNum\":\"ipc-ov2718\"\n}");//str2的值为post的数据
    //strcat(str2, "{\nverion: \"1.1\",\nfirmwareType:\"AQD17S20WW5018080005\",\nfirmwareNum:\"ipc-ov2718\"\n}");

    printf("str2 = %s\n",str2);

    len = strlen(str2);
    sprintf(str, "%d", len);
   
    
    char tmp_buf[1024] = {0};
    memset(str1, 0, 2048);
    
    //"Content-Type: application/x-www-form-urlencoded\r\n"\
    
    sprintf(str1, \
    "POST %s HTTP/1.1\r\n"\
    "Host: %s\r\n"\
    "Accept-Language: en-us"\
    "Content-Type: application/json\r\n"\
    "Content-Length: %s\r\n"\
    "\r\n"\
    "%s\r\n"\
    "\r\n"\
,url, host,str,str2); //str2的值为post的数据

    //printf("<http> POST string: %s\n",str1);
    
     puts("5: Sending HTTP POST request...");
    ret = write(sockfd,str1,strlen(str1));
    if (ret < 0) 
    {
            printf("<http> post--->write failed! errno(%d),'%s'\n",errno, strerror(errno));
            return NULL;
    }
    else
    {
            printf(">>>>post(%d)bytes succeeded<<<<\n",ret);

    }
    
  
    while(1)
    {
            FD_ZERO(&t_set1);
            FD_SET(sockfd, &t_set1);
       
            tv.tv_sec= 1;
            tv.tv_usec= 0;
            h= 0;
        
            h= select(sockfd +1, &t_set1, NULL, NULL, &tv);

            if (0 == h)//timeout
            {
                continue;
            }
            else if (h < 0) 
            {
                    close(sockfd);
                    printf("<http> select error，abort！\n");
                    return NULL;
            }
            else
            {
                    puts("6: Receive the HTTP response header...");
                    memset(recv_buf, 0, BUFSIZE);
                    i= read(sockfd, recv_buf, BUFSIZE-1);
                    if (i <= 0)
                    {
                            close(sockfd);
                            printf("<http> read error!\n");
                            return NULL;
                    }

                    printf("<http> Max buffer size(%d)bytes \nreceive (%d) bytes :\n%s\n",BUFSIZE,i,recv_buf);
                    printf("\n>>>>complete!<<<<\n");
                    break;
            }
    }



    close(sockfd);
    
    return recv_buf;
}

/*****************************************************************
*函数名 ：     prase_POST_BAK
*功能描述 ：解析http发送post请求后收到的回应包
*参数 ：  response : 回应包首地址
*返回值 ：      成功：0 
            失败：-1
*****************************************************************/
#include "media_server_cJSON.h"
int prase_POST_BAK(const char* response,updateInfo_t*upgradeInfo)
{
    if(NULL == response)
    {
        printf("Illegal parameter !\n");
        return -1;
    }
    char * ret =NULL;

    struct HTTP_RES_HEADER resp = parse_header(response);

    printf("\n>>>>HTTP response header parsing was successful:<<<<\n");
    printf("\tHTTP response code: %d\n", resp.status_code);
    printf("\tHTTP file type: %s\n", resp.content_type);
    printf("\tHTTP body length: %ldBytes\n\n", resp.content_length);
    
    //提取“{}”部分并返回
    char * string_start = strstr(response,"{");
    char * string_end = strstr(response,"}");
    int diff = string_end - string_start;
    
    int string_length = strlen(string_start);
    printf("body_string = %s\n",string_start);
    printf("diff = %d\n",diff);

    printf("string_length = %d\n",string_length);
    char*json_string = (char*)calloc(string_length,1);
    strncpy(json_string,string_start,diff+1);//提取“{}”部分
    printf("ret_string = %s\n",json_string);

    cJSON*json = cJSON_Parse(json_string);
    if (!json) 
    {
        printf("Error before: [%s]\n",cJSON_GetErrorPtr());
        return -1;
    }
    else
    {
        char *out=cJSON_Print(json);//json结构体转换成字符串
        printf("cJSON_Print: %s\n",out);
        
    #if 1
        /*
            将解析出来的字符串写入到文件中
        */
        FILE *writefile = fopen("/jffs0/JSON.txt","wb+");//读写打开文件
        if(NULL != writefile)
        {
              printf("write file strlen(out) = %d \n",strlen(out)); 
              int size = fwrite(out,1,strlen(out)+1,writefile);
              fflush(writefile);
              if(size > 0)
                printf("fwrite success!\n");
        }
    #endif


    //提取出升级包的url md5 version等信息
    cJSON* model = cJSON_GetObjectItem(json,"model");
    if(NULL == model)//提取失败，没有对应元素
    {
        printf("HTTP POST back header not have JSON item \"model\" ,prase failed! \n");
        cJSON_Delete(json);
        free(out);
        return -1;
    }
    else
    {
        cJSON *updateInfo = cJSON_GetObjectItem(model,"updateInfo");
        if(NULL == updateInfo)
        {
            printf("HTTP POST back header not have JSON item \"updateInfo\" ,prase failed! \n");
            cJSON_Delete(json);
            free(out);
            return -1;
        }
        else
        {
            char *downloadUrl = cJSON_GetObjectItem(updateInfo,"downloadUrl")->valuestring;
            char *md5Value = cJSON_GetObjectItem(updateInfo,"md5Value")->valuestring;
            char *verion = cJSON_GetObjectItem(updateInfo,"verion")->valuestring;
            char *size = cJSON_GetObjectItem(updateInfo,"size")->valuestring;

            if(NULL == downloadUrl || NULL == md5Value||\
               NULL == verion || NULL == size)
            {
                printf("prase item \"updateInfo\" error!\n");
                cJSON_Delete(updateInfo);
                cJSON_Delete(json);
                free(out);
                return -1;
            }
            else
            {
                upgradeInfo->downloadUrl = strdup(downloadUrl);
                upgradeInfo->md5Value = strdup(md5Value);
                upgradeInfo->verion = strdup(verion);
                upgradeInfo->size = strdup(size);

                cJSON_Delete(updateInfo);
                free(downloadUrl);
                free(md5Value);
                free(verion);
                free(size);
            
                cJSON_Delete(model);
                cJSON_Delete(json);
                free(out);
                return 0;
                
            }
            
        }
        
    }

   }

return -1;
}



/*****************************************************************
*函数名 ：     dowload_upadte_file
*功能描述 ：下载升级文件
*
*参数 ：  argc:入参个数
*       argv[0]: http post请求的url 
*返回值 ：      成功：0 
*           失败：-1
*****************************************************************/
//int dowload_upadte_file(char *host_url)
int dowload_upadte_file(int argc,char**argv)
{

    char * host_url = NULL;
    if(argc >= 1)
    {
        host_url = strdup(argv[0]); 
    }
    else
    {
        host_url = DEVELOP_URL;
    }
    
    printf("host_url = %s\n",host_url);
    
    //发送固件升级的 post 请求,接收响应头
    char *response = http_post(host_url);
    if(NULL == response)return -1;

    //解析响应头，得出升级文件的url、MD5、version等信息
    updateInfo_t updateInfo = {0};
    int ret = prase_POST_BAK(response,&updateInfo);
    if(ret < 0)return -1;


    //下载升级文件
    char *argv_tmp[] = {updateInfo.downloadUrl};
    char * dowloadfile = http_dowload_file(1,argv_tmp);
    if(NULL == dowloadfile)return -1;
    printf("dowloadfile = %s",dowloadfile);
    
    //升级包的数据校验，版本校验等业务...............待定

    free(dowloadfile);
    free(response);
    free(host_url);
    return 0;
    
}










