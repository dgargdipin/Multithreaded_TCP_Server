#include <arpa/inet.h>
#include <chrono>
#include <ctime>
#include <errno.h>
#include <functional>
#include <mutex>
#include <netdb.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <thread>
#include <unistd.h>
#include <vector>
#include<iostream>
#define PORT "3490"  // the port users will be connecting to

#define BACKLOG 10   // how many pending connections queue will hold




class TCP_SERVER{
        int sockfd;
        const char *port;
        struct addrinfo hints, *servinfo;
        bool error_state;
        std::mutex io_mtx;
        std::vector<std::thread> _threads;
        std::function<const char*(void) > callback;
        void fill_addrinfo(){
                memset(&hints,0,sizeof hints);
                hints.ai_family=AF_UNSPEC;
                hints.ai_socktype=SOCK_STREAM;
                hints.ai_flags=AI_PASSIVE;
                int rv;
                if ((rv = getaddrinfo(NULL,port, &hints, &servinfo)) != 0) {
                        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
                        exit(1);
                };

        }
        void bind_socket_and_listen(){
                addrinfo *p;
                for(p=servinfo;p!=NULL;p=p->ai_next){
                        if((sockfd=socket(p->ai_family,p->ai_socktype,p->ai_protocol))==-1){
                                perror("server: socket");
                                continue;
                        }
                        int yes=1;
                        if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes,
                                                sizeof(int)) == -1) {
                                perror("setsockopt");
                                exit(1);
                        }
                        if(bind(sockfd,p->ai_addr,p->ai_addrlen)==-1){
                                close(sockfd);
                                perror("server: bind");
                                continue;

                        }
                        break;

                }
                if(p==NULL){
                        fprintf(stderr, "server: failed to bind\n");
                        exit(1);

                }
                if (listen(sockfd, BACKLOG) == -1) {
                        perror("listen");
                        exit(1);
                }
                printf("Started listening \n");
        }

        void handle_sig_pipe(){//to not exit on disconnection of io network pipe
                struct sigaction new_actn, old_actn;
                new_actn.sa_handler = SIG_IGN;
                sigemptyset(&new_actn.sa_mask);
                new_actn.sa_flags = 0;
                sigaction(SIGPIPE, &new_actn, &old_actn);
        }

        void *get_in_addr(struct sockaddr *sa)
        {
                if (sa->sa_family == AF_INET) {
                        return &(((struct sockaddr_in*)sa)->sin_addr);
                }

                return &(((struct sockaddr_in6*)sa)->sin6_addr);
        }

        void handle_request(int io_fd,sockaddr their_addr,char *host){
                printf("Spawned new thread with fd:%d\n",io_fd);
                while(1){
                        {
                                char buffer[2000];
                                int bytesRead=recv(io_fd,buffer,sizeof buffer,0);
                                std::lock_guard<std::mutex> lg(io_mtx);
                                if(bytesRead<1){
                                        perror("recv error");
                                        printf("Connection closed with %s\n",
                                                        host);
                                        break;
                                }
                                printf("Received msg from %s: %s\nSending reply..\n",host,buffer);
                        }
                        const char* reply_string=callback();
                        int bytes_sent=send(io_fd,reply_string,strlen(reply_string),0);
                        std::lock_guard<std::mutex> lg(io_mtx);
                        if(bytes_sent==-1){
                                perror("send error");
                                printf("Connection closed with %s\n",host);
                                break;
                        }
                        else{
                                printf("Sent reply to %s\n",host);
                        }
                }
                close(io_fd);
        }
        void accept_loop(){
                while(1){
                        sockaddr_storage their_addr;
                        socklen_t sin_size=sizeof their_addr;
                        int new_fd=accept(sockfd,(sockaddr*)&their_addr,&sin_size);
                        if (new_fd == -1) {
                                perror("accept");
                                continue;
                        }
                        char s[INET6_ADDRSTRLEN];
                        inet_ntop(their_addr.ss_family,get_in_addr((sockaddr*)&their_addr),s,sizeof s);
                        printf("server: got connection from %s with fd : %d\n", s,new_fd);
                        _threads.emplace_back(std::thread(&TCP_SERVER::handle_request,this,new_fd,*((sockaddr*)&their_addr),s));

                }
        }
        public:
        TCP_SERVER(int port,
                        std::function<const char *()> calllback)
                : callback(calllback) {
                        this->port = std::to_string(port).c_str();
                        this->fill_addrinfo();
                }
        void listen_server(){
                this->bind_socket_and_listen();
                printf("server: waiting for connections...\n");
                handle_sig_pipe();
                accept_loop();

        }
        ~TCP_SERVER(){
                freeaddrinfo(servinfo);
                struct sigaction old_actn;
                sigaction(SIGPIPE, &old_actn, NULL);

                for(auto &t:_threads){
                        t.join();
                }
        }
};


class TCPtimeServer:public TCP_SERVER{
        static const char *get_current_time(){
                auto end = std::chrono::system_clock::now();

                std::time_t end_time = std::chrono::system_clock::to_time_t(end);


                return std::ctime(&end_time);
        }
        public:
        TCPtimeServer(int port):TCP_SERVER(port,get_current_time){};
};

int main(){
        TCPtimeServer server(3491);
        server.listen_server();
}
