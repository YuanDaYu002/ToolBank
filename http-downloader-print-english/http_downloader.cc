#include <stdio.h>//printf
#include <string.h>//字符串处理
#include <sys/socket.h>//套接字
#include <arpa/inet.h>//ip地址处理
#include <fcntl.h>//open系统调用
#include <unistd.h>//write系统调用
#include <netdb.h>//查询DNS
#include <stdlib.h>//exit函数
#include <sys/stat.h>//stat系统调用获取文件大小
#include <sys/time.h>//获取下载时间

struct HTTP_RES_HEADER//保持相应头信息
{
    int status_code;//HTTP/1.1 '200' OK
    char content_type[128];//Content-Type: application/gzip
    long content_length;//Content-Length: 11683079
};

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
        printf("文件创建失败!\n");
        exit(0);
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

int main(int argc, char const *argv[])
{
    /* 命令行参数: 接收两个参数, 第一个是下载地址, 第二个是文件的保存位置和名字, 下载地址是必须的, 默认下载到当前目录
     * 示例: ./download http://www.baidu.com baidu.html
     */
    char url[2048] = "127.0.0.1";//设置默认地址为本机,
    char host[64] = {0};//远程主机地址
    char ip_addr[16] = {0};//远程主机IP地址
    int port = 80;//远程主机端口, http默认80端口
    char file_name[256] = {0};//下载文件名

    if (argc == 1)
    {
        printf("You must give an HTTP address to get started! \n");
        exit(0);
    }
    else
        strcpy(url, argv[1]);

    puts("1:Parsing the download address...");
    parse_url(url, host, &port, file_name);//从url中分析出主机名, 端口号, 文件名

    if (argc == 3)
    {
        printf("\tYou have specified the download file name as: %s\n", argv[2]);
        strcpy(file_name, argv[2]);
    }

    puts("2: Getting the remote server IP address...");
    get_ip_addr(host, ip_addr);//调用函数同访问DNS服务器获取远程主机的IP
    if (strlen(ip_addr) == 0)
    {
        printf("Error: could not get IP address of remote server. Please check the validity of download address\n");
        return 0;
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
        exit(-1);
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
        exit(-1);
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
                printf("malloc failed!\n");
                exit(-1);
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
        return 0;
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
    }
    shutdown(client_socket, 2);//关闭套接字的接收和发送
    return 0;
}

