#include<stdio.h>
#include<sys/socket.h>
#include<arpa/inet.h>
#include<sys/ioctl.h>
#include<net/if.h>
#include<sys/time.h>
#include<sys/types.h>
#include <unistd.h>
#include<unistd.h> 
#include<string.h>
#include<stdlib.h>
#include<sys/stat.h>
#include<vector>
#include<set>
#include<sstream>
#include<string>
#include <fcntl.h>
#include <filesystem>
#include "opencv2/opencv.hpp"
#include  <signal.h>
#include<iostream>

using namespace std;
using namespace cv;
namespace fs = std::filesystem;

#define BUFF_SIZE 1024
#define PORT 8787
#define ERR_EXIT(a){ perror(a); exit(1); }
#define END(ff){cout << "server: socket " << ff << "hung up" << endl; close(ff); FD_CLR(ff, &read_master);}
#define CLR(ff) if(write_byte == -1){ clear(ff);continue; }
// #define CLR(ff) {clear(ff); continue;}

int Min(int a, int b){
    if(a < b) return a;
    else return b;
}


int port = 8787;
const int maxn = BUFF_SIZE*2;
fd_set read_set, write_set;
fd_set read_master, write_master;
set<string> files;
vector<int> fdlist;
vector<string> bufs(maxn);
vector<VideoCapture> caps(maxn);
vector<Mat> server_imgs(maxn);
vector<int> width(maxn), height(maxn);
// VideoCapture cap;
int cnt = 0;
struct cli_data{
    // int ptr;
    string typ;
    string filename;
    int filesize;
    int stat; //
    int off;
    FILE *fp;
    string buf;

};
vector<cli_data> cli_datas(maxn);
bool queuing[maxn];
void clear(int i){
    if(errno == EAGAIN) return;
   cli_datas[i].typ = "";
   cli_datas[i].filename = "";
   cli_datas[i].filesize = 0;
   cli_datas[i].stat = 0;
   cli_datas[i].off = 0;
   cli_datas[i].buf = "";
   queuing[i] = 0;
   FD_CLR(i, &write_master);
   FD_CLR(i, &read_master);
//    printf("server: socket %d hung up\n", i);
   close(i);
}
void reset(int i){
   cli_datas[i].typ = "";
   cli_datas[i].filename = "";
   cli_datas[i].filesize = 0;
   cli_datas[i].stat = 0;
   cli_datas[i].off = 0;
   cli_datas[i].buf = "";
   queuing[i] = 0;
}
int fdcnt = 0;
char buf[2 * BUFF_SIZE] = {};
int main(int argc, char *argv[]){

    if(argv[1] == NULL){
        cout << "no port" << endl;
        return 0;
    }
    port = atoi(argv[1]);


    signal(SIGPIPE, SIG_IGN);
    Mat client_img;
    int server_sockfd, client_sockfd, write_byte, read_byte;
    struct sockaddr_in server_addr, client_addr;
    struct sockaddr_storage remoteaddr; // client address
    int newfd;
    int client_addr_len = sizeof(client_addr);
    int nbytes, filesize;

    socklen_t addrlen;
    
    FD_ZERO(&read_master);
    FD_ZERO(&read_set);
    FD_ZERO(&write_set);
    FD_ZERO(&write_master);

    struct timeval timeout;
    timeout.tv_sec = 15;
    timeout.tv_usec = 0;
    int retval;
    int write_maxfd, read_maxfd, maxfd;

    mkdir("./b08902012_server_folder", 0777);

    // Get socket file descriptor
    if((server_sockfd = socket(AF_INET , SOCK_STREAM , 0)) < 0){
        ERR_EXIT("socket failed\n")
    }

    // Set server address information
    bzero(&server_addr, sizeof(server_addr)); // erase the data
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(port);
    
    // Bind the server file descriptor to the server address
    if(bind(server_sockfd, (struct sockaddr *)&server_addr , sizeof(server_addr)) < 0){
        ERR_EXIT("bind failed\n");
    }      
    // Listen on the server file descriptor
    if(listen(server_sockfd , 3) < 0){
        ERR_EXIT("listen failed\n");
    }
    fcntl(server_sockfd, F_SETFL, O_NONBLOCK);
    maxfd = server_sockfd;
    FD_SET(server_sockfd, &read_set);
    FD_SET(server_sockfd, &read_master);



    // use select to get the request
    while(1){
        memset(buf, 0, sizeof(buf));
        read_set = read_master;
        write_set = write_master;
        if(select(maxfd+1, &read_set, &write_set, NULL, NULL) == 0){
            perror("select");
            exit(4);
        }
        for(int i = 0; i <= maxfd ; i++){
            if(FD_ISSET(i, &write_set)){
                if(cli_datas[i].typ == "ls"){
                    strcpy(buf, bufs[i].c_str());
                    if(cli_datas[i].off != sizeof(buf)){
                        write_byte = send(i, buf+cli_datas[i].off, sizeof(buf) - cli_datas[i].off, 0);
                        if(write_byte <= 0) {clear(i); continue;}
                        cli_datas[i].off += write_byte;
                    }
                    if(cli_datas[i].off == sizeof(buf)){
                        FD_CLR(i, &write_master);
                        FD_SET(i, &read_master);
                        queuing[i] = 0;
                    }
                    // queuing[i] = 0;

                }
                else if(cli_datas[i].typ == "get"){
                    if(cli_datas[i].stat <= 1){
                        if((write_byte = send(i, &cli_datas[i].filesize, 4, 0)) != 4){
                            if(write_byte <= 0) {clear(i); continue;}
                            continue;
                        }
                        cli_datas[i].off = 0;
                        if(cli_datas[i].filesize == -1){
                            cli_datas[i].stat = 0;
                            queuing[i] = 0;
                            FD_CLR(i, &write_master);
                            FD_SET(i, &read_master);
                            continue;
                        }
                        cli_datas[i].stat = 2;                        
                    }
                    if(cli_datas[i].stat <= 2){
                        
                        //start to send file
                        if(cli_datas[i].off != cli_datas[i].filesize){
                            fseek(cli_datas[i].fp, cli_datas[i].off, SEEK_SET);
                            read_byte = fread(buf, sizeof(char), sizeof(buf), cli_datas[i].fp);
                            write_byte = send(i, buf, read_byte, 0);
                            if(write_byte <= 0) {clear(i); continue;}
                            cli_datas[i].off += write_byte;
                            
                        }
                        if(cli_datas[i].off == cli_datas[i].filesize){
                            fclose(cli_datas[i].fp);
                            FD_CLR(i, &write_master);
                            queuing[i] = 0;
                        }
                    }
                }
                else if(cli_datas[i].typ == "play"){
                    if(cli_datas[i].stat == 0){
                        //send resolution
                        if((write_byte = send(i, &width[i], 4, 0)) != 4){
                            if(write_byte <= 0) {clear(i); continue;}
                            continue;
                        }
                        if(width[i] == -1){
                            queuing[i] = 0;
                            FD_CLR(i, &write_master);
                            FD_SET(i, &read_master);
                            continue;
                        }
                        cli_datas[i].stat = 1;
                    }
                    if(cli_datas[i].stat <= 1){
                        if((write_byte = send(i, &height[i], 4, 0)) != 4){
                            if(write_byte <= 0) {clear(i); continue;}
                            continue;
                        }
                        cli_datas[i].stat = 2;
                        server_imgs[i] = Mat::zeros(height[i], width[i], CV_8UC3);
                        client_img = Mat::zeros(540, 960, CV_8UC3);
                        if(!server_imgs[i].isContinuous()){
                            server_imgs[i] = server_imgs[i].clone();
                        }
                        if(!client_img.isContinuous()){
                            client_img = client_img.clone();
                        }
                        caps[i].read(server_imgs[i]);
                    }
                    if(cli_datas[i].stat <= 2){
                            
                        // Get the size of a frame in bytes 
                        int imgSize = server_imgs[i].total() * server_imgs[i].elemSize();
                        write_byte = send(i, &imgSize, 4, 0);
                        if(write_byte <= 0){clear(i); continue;}
                        if((write_byte = send(i, server_imgs[i].data+cli_datas[i].off, imgSize-cli_datas[i].off, 0)) != imgSize-cli_datas[i].off){
                            if(write_byte <= 0) {clear(i); continue;}
                            cli_datas[i].off += write_byte;
                            break;
                        }
                        cli_datas[i].off = 0;
                        caps[i] >> server_imgs[i];

                        FD_CLR(i, &write_master);
                        FD_SET(i, &read_master);                        
                    }
                        
                    
                }
            }
            else if(FD_ISSET(i, &read_set)){
                if(i == server_sockfd){
                    // new connection!!

                    addrlen = sizeof remoteaddr;
                    newfd = accept(server_sockfd,
                        (struct sockaddr *)&remoteaddr,
                        &addrlen);
                    if(newfd == -1){
                        perror("accept");
                    }
                    else{
                        FD_SET(newfd, &read_master);
                        fcntl(newfd, F_SETFL, O_NONBLOCK);
                        if(newfd > maxfd) maxfd = newfd;
                        cout << newfd << endl;
                    }

                }
                //queuing -> continuing dealing with the request
                else if(queuing[i]){
                    if(cli_datas[i].typ == "put"){
                        if(cli_datas[i].stat == 0){
                            //get filename
                            if((read_byte = recv(i, buf, sizeof(buf) - cli_datas[i].off, 0)) != sizeof(buf) - cli_datas[i].off){
                                if(read_byte <= 0){clear(i); continue;}
                                cli_datas[i].off += read_byte;
                                cli_datas[i].filename += buf;
                                continue;
                            }
                            cli_datas[i].off = 0; 
                            cli_datas[i].filename += buf;
                            memset(buf, 0, sizeof(buf));
                            cli_datas[i].filename = "./b08902012_server_folder/" + cli_datas[i].filename;

                            cli_datas[i].fp = fopen(cli_datas[i].filename.c_str(), "w");

                        }
                        if(cli_datas[i].stat <= 1){
                            if((read_byte = recv(i, &filesize, 4, 0)) != 4){
                                if(read_byte <= 0){clear(i); continue;}
                                cli_datas[i].stat = 1;
                                queuing[i] = 1;
                                continue;
                            }
                            cli_datas[i].filesize = filesize;
                            cli_datas[i].stat = 2;
                            cli_datas[i].off = 0;
                        }
                        if(cli_datas[i].stat <= 2){
                            if(cli_datas[i].off != cli_datas[i].filesize){
                                read_byte = recv(i, buf, Min(cli_datas[i].filesize - cli_datas[i].off, sizeof(buf)), 0);
                                if(read_byte <= 0){clear(i); continue;}
                                fseek(cli_datas[i].fp, cli_datas[i].off, SEEK_SET);
                                fwrite(buf, sizeof(char), read_byte, cli_datas[i].fp);
                                cli_datas[i].off += read_byte;
                                
                            }
                            if(cli_datas[i].off == cli_datas[i].filesize){
                                fclose(cli_datas[i].fp);
                                queuing[i] = 0;
                            }
                        }
                    }
                    else if(cli_datas[i].typ == "get"){
                        if(cli_datas[i].stat == 0){
                            memset(buf, 0, sizeof(buf));
                            if((read_byte = recv(i, buf, sizeof(buf) - cli_datas[i].off, 0)) != sizeof(buf)-cli_datas[i].off){
                                if(read_byte <= 0){clear(i); continue;}
                                cli_datas[i].filename += buf;
                                cli_datas[i].off += read_byte;
                                queuing[i] = 1;
                                continue;
                            }
                            cli_datas[i].filename += buf;
                            memset(buf, 0, sizeof(buf));
                            cli_datas[i].filename = "./b08902012_server_folder/" + cli_datas[i].filename;

                            cli_datas[i].fp = fopen(cli_datas[i].filename.c_str(), "r");
                            if(cli_datas[i].fp == NULL){
                                cli_datas[i].filesize = -1;                
                            }
                            else{
                                fseek(cli_datas[i].fp, 0, SEEK_END);
                                filesize = ftell(cli_datas[i].fp);
                                cli_datas[i].filesize = filesize;
                            }
                            cli_datas[i].stat = 1;
                            FD_SET(i, &write_master);
                        }
                        
                    }
                    else if(cli_datas[i].typ == "play"){
                        // get filename
                        if(cli_datas[i].stat == 0){
                            if((read_byte = recv(i, buf, sizeof(buf), 0)) != sizeof(buf)){
                                if(read_byte <= 0){clear(i); continue;}
                                continue;
                            }
                            cli_datas[i].filename = buf;
                            cli_datas[i].filename = "./b08902012_server_folder/" + cli_datas[i].filename;
                            if(caps[i].open(cli_datas[i].filename)){
                                width[i] = caps[i].get(CV_CAP_PROP_FRAME_WIDTH);
                                height[i] = caps[i].get(CV_CAP_PROP_FRAME_HEIGHT);
                            }
                            else{
                                width[i] = -1;
                                cout << "file doesn't exit" << endl;
                            }
                            FD_SET(i, &write_master);
                            FD_CLR(i, &read_master);
                        }
                        if(cli_datas[i].stat == 2){
                            read_byte = recv(i, buf, 1, 0);
                            if(read_byte <= 0){clear(i); continue;}
                            if(buf[0] == 'o'){
                                FD_SET(i, &write_master);
                                FD_CLR(i, &read_master);
                            }
                            else{
                                FD_CLR(i, &write_master);
                                FD_SET(i, &read_master);
                                queuing[i] = 0;
                                caps[i].release();
                            }
                        }   
                    }
                }
                else if((read_byte = recv(i, buf, 1, 0)) <= 0){
                    if(read_byte <= 0){clear(i); continue;}
                }                
                else if(buf[0] == 'l'){
                    queuing[i] = 1;
                    cli_datas[i].typ = "ls";
                    cli_datas[i].stat = 0;
                    cli_datas[i].filename = "";
                    cli_datas[i].filesize = 0;
                    cli_datas[i].off = 0;
                    bufs[i] = "";
                    string str;
                    for(const auto &entry : fs::directory_iterator("./b08902012_server_folder/")){
                        str = entry.path();
                        str = str.substr(26);
                        bufs[i] += str;
                        bufs[i] += '\n';
                    }
                    FD_SET(i, &write_master);
                }
                //initial stage
                else if(buf[0] == 'p'){
                    reset(i);
                    queuing[i] = 1;
                    cli_datas[i].typ = "put";                    
                    continue;
                }
                else if(buf[0] == 'g'){
                    reset(i);
                    queuing[i] = 1;
                    cli_datas[i].typ = "get";
                    continue;
                }
                else if(buf[0] == 'v'){
                    reset(i);
                    queuing[i] = 1;
                    cli_datas[i].typ = "play";
                    continue;
                }
            }
            
        }
    }
}
