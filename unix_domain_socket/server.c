#include <stdio.h>  
#include <sys/socket.h>  
#include <sys/un.h>  
#include <string.h>  
  
int main(void){  
        int fd,clientfd;  
        int client_addr_len = sizeof(struct sockaddr_un);  
        struct sockaddr_un un,clientun;  
        char path[32]="serversocketfile";  
        int addr_len = sizeof(struct sockaddr_un);  
        char rbuf[32];  
        char wbuf[32] = "i am server.";  
  
        //create a UNIX domain stream socket  
        if((fd = socket(AF_UNIX,SOCK_STREAM,0)) < 0){  
                perror("socket");  
                return -1;  
        }  
  
        //in case it already exists  
        unlink(path);  
  
        //fill in socket address structure  
        memset(&un, 0, sizeof(un));  
        un.sun_family = AF_UNIX;  
        strncpy(un.sun_path,path,32);  
  
        //bind the name to the descriptor  
        if(bind(fd, (struct sockaddr*)&un, addr_len) < 0){  
                perror("bind");  
                return -1;  
        }  
  
        if(listen(fd, 10) < 0){  
                perror("listen");  
                return -1;  
        }
	printf("listen..\n");  
  
        if((clientfd = accept(fd,(struct sockaddr*)&clientun,(socklen_t*)&client_addr_len)) < 0 ){  
                perror("accept");  
                return -1;  
        }  
  
        printf("client is:%s\n",clientun.sun_path);  
  
        if(read(clientfd,rbuf,32) < 0){  
                perror("read");  
                return -1;  
        }  
        printf("receive msg:%s\n",rbuf);  
  
        if(write(clientfd,wbuf,strlen(wbuf)+1) < 0){  
                perror("write");  
                return -1;  
        }  
  
        unlink(path);  
        return 0;  
}
