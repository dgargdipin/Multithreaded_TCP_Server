#include <cstddef>
#include <unistd.h>
#include <cstdio>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include<iostream>
#include <arpa/inet.h>
#include <signal.h>

#define PORT "3490" // the port client will be connecting to 

#define MAXDATASIZE 100 // max number of bytes we can get at once 
volatile sig_atomic_t stop;

void
inthand(int signum)
{
    stop = 1;
}



void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}



int main(int argc,char **argv){

        if (argc != 3) {
                fprintf(stderr,"usage: client hostname port\n");
                exit(1);
        }
        int sockfd;
        struct addrinfo hints,*servinfo,*addr_iter;
        memset(&hints,0,sizeof hints);
        hints.ai_family=AF_UNSPEC;
        hints.ai_socktype=SOCK_STREAM;
        int addrinfo_retvalue;
        if((addrinfo_retvalue=getaddrinfo(argv[1],argv[2],&hints,&servinfo))!=0){
                fprintf(stderr,"getaddrinfo: %s\n",gai_strerror(addrinfo_retvalue));
                return 1;
        }
        for(addr_iter=servinfo;addr_iter!=NULL;addr_iter=addr_iter->ai_next){
                if((sockfd=socket(addr_iter->ai_family,addr_iter->ai_socktype,addr_iter->ai_protocol))==-1){
                        perror("client:socket");
                        continue;
                }
                if(connect(sockfd,addr_iter->ai_addr,addr_iter->ai_addrlen)==-1){
                        close(sockfd);
                        perror("client: connect");
                        continue;
                }
                break;

        }
        if(addr_iter==NULL){
                fprintf(stderr, "client: failed to connect\n");
                return 2;

        }
        char s[INET6_ADDRSTRLEN];
        inet_ntop(addr_iter->ai_family,get_in_addr((sockaddr*)addr_iter->ai_addr),s,sizeof s);
        printf("client: connecting to %s\n", s);
        freeaddrinfo(servinfo); // all done with this structure

        signal(SIGINT, inthand);

        while(!stop){

                printf("Enter line: ");
                char *str = NULL;
                size_t buffer_size = 0;

                int len = getline(&str, &buffer_size, stdin);
                if (send(sockfd,str,strlen(str), 0) == -1)
                        perror("send");

                char buffer[MAXDATASIZE];
                int numbytes;
                if((numbytes=recv(sockfd,buffer,MAXDATASIZE-1,0))==-1){
                        perror("recv");
                        exit(1);
                }
                buffer[numbytes]='\0';
                printf("client: received '%s'\n",buffer);
        }
        printf("\nExiting safely\n");
        close(sockfd);
        return 0;

}

