
/*
 * : main.c
	> Author: qianghaohao(Xqiang)
	> Program: url_parser
	> Readme: 本URL解析器代码来自https://github.com/jwerle/url.h
	>         在此基础上进行了大量的修改,修复了很多bug.
	>         总体思路没有改变,就是把很多细节改了下,现在可以正常使用了.
	>    -->可能还存在bug,如果网友发现了可以及时指正.
	> Platform: 可以跨平台使用
	> Created Time: 2016年06月11日 星期六 18时39分14秒
 ************************************************************************/
 
#include"url_parser.h"
url_data_t *url_info = NULL;
int main(int argc, char **argv) 
{
	if (argc != 2) 
	{
		fprintf(stderr, "usage:%s <url>", argv[0]);
		argv[1] = "http://www.baidu.com:80/cig-bin/index.html?sdkfj#283sjkdf";
	}
 
	url_info = url_parse(argv[1]);
	if (NULL == url_info) 
	{
		fprintf(stderr, "%s error...\n", argv[1]);
		return -1;
	}
	
	//url_data_inspect(url_info);
 
	//  测试用例:http://www.baidu.com:80/cig-bin/index.html?sdkfj#283sjkdf
	printf("\n================= API Demo ================\n");
 
 
	printf("herf:%s\n", argv[1]);
 /*
    char *hostname = url_get_host(argv[1]);
    printf("hostname:%s\n", hostname);
 
    char *path = url_get_path(argv[1]);
    printf("path:%s\n", path);
 
    char *host = url_get_host(argv[1]);
    printf("host:%s\n", host);
 
    char *proto = url_get_protocol(argv[1]);
    printf("protocol:%s\n", proto);
 
    char *auth = url_get_auth(argv[1]);
    printf("auth:%s\n", auth);
 
    char *search = url_get_search(argv[1]);
    printf("search:%s\n", search);
 
    char *hash = url_get_hash(argv[1]);
    printf("hash:%s\n", hash);
 
    char *query = url_get_query(argv[1]);
    printf("query:%s\n", query);
 
    char *port = url_get_port(argv[1]);
    printf("port:%s\n", port);
*/
	return 0;
}


