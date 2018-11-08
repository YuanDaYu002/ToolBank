#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#define MAXN 1024+10
//char url [MAXN] = "http://www.google.com:80/wiki/Search?search=train&go=Go#steammachine"; 

char url [MAXN] = "https://blog.csdn.net/baidu_36649389/article/details/63301047";
int main()
{
    const char *parseptr1;
    const char *parseptr2;
    int len;
    int i;
    parseptr2 = url;
    parseptr1 = strchr(parseptr2, ':');
    if ( NULL == parseptr1 ) 
    {
        printf("URL错误!\n");
        return 0;
    }
    
    len = parseptr1 - parseptr2;
    for ( i = 0; i < len; i++ ) 
    {
        if ( !isalpha(parseptr2[i]) ) 
        {
            printf("URL错误!\n");
            return 0;
        }
    }
    printf("protocol: ");
    for(i=0;i<len;i++)
        printf("%c",parseptr2[i]);
    printf("\n");//解析协议
    parseptr1++;
    parseptr2 = parseptr1;
    for ( i = 0; i < 2; i++ ) 
    {
        if ( '/' != *parseptr2 ) 
        {
            printf("URL错误!\n");
            return 0;
        }
        parseptr2++;
    }
    parseptr1 = strchr(parseptr2, ':');
    if ( NULL == parseptr1 )//判断有无端口号
    {
        parseptr1 = strchr(parseptr2, '/');
        if ( NULL == parseptr1 ) 
        {
            printf("URL错误!\n");
            return 0;
        }
        
        len = parseptr1 - parseptr2;
        printf("host: ");
        for(i=0;i<len;i++)
           printf("%c",parseptr2[i]);
        printf("\n");//解析主机
    }
    else
    {
        len = parseptr1 - parseptr2;
        printf("host: ");
        for(i=0;i<len;i++)
            printf("%c",parseptr2[i]);
        printf("\n");
        parseptr1++;
        parseptr2 = parseptr1;
        parseptr1 = strchr(parseptr2, '/');
        if ( NULL == parseptr1 ) 
        {
            printf("URL错误!\n");
            return 0;
        }
        len = parseptr1 - parseptr2;
        printf("port: ");
        for(i=0;i<len;i++)
            printf("%d",(parseptr2[i]-48));
        printf("\n");//解析端口
    }

    parseptr1++;
    parseptr2 = parseptr1;

    while ( '\0' != *parseptr1 && '?' != *parseptr1  && '#' != *parseptr1 ) 
    {
        parseptr1++;
    }
    
    len = parseptr1 - parseptr2;
    printf("path: ");

    for(i=0;i<len;i++)
           printf("%c",parseptr2[i]);
    printf("\n");//解析路径
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
        printf("query: ");
        for(i=0;i<len;i++)
            printf("%c",parseptr2[i]);//判断有无询问并解析
        printf("\n");
    }
    
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
        printf("fragment: ");
        for(i=0;i<len;i++)
            printf("%c",parseptr2[i]);
        printf("\n");//判断有无片段并解析
    }
}
