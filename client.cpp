
#include<iostream>
#include<sys/socket.h> 
#include<arpa/inet.h>
#include<sys/ioctl.h>
#include<net/if.h>
#include<unistd.h> 
#include<string.h>
#include<stdio.h>
#include<stdlib.h>
#include <sys/stat.h>
#include<string>
#include<vector>
#include <fcntl.h>
#include<sstream>
#include "opencv2/opencv.hpp"
using namespace std;
using namespace cv;

#define BUFF_SIZE 1024
#define PORT 8787
#define ERR_EXIT(a){ perror(a); exit(1); }
FILE* pfile;
int main(int argc , char *argv[]){

    if(argv[1] == NULL){
        cout << "no id " << endl;
        return 0;
    }
    if(argv[2] == NULL){
        cout << "no ip, port" << endl;
        return 0;
    }
    string ip, port_st;
    ip = argv[2];
    auto pos = ip.find(":");
    port_st = ip.substr(pos+1);
    ip = ip.substr(0, pos);

    // VideoCapture cap;
    Mat server_img,client_img;
    // Get the resolution of the video

    int ser_addr;
    int width, height, right;
    int off;
    int sockfd, read_byte, write_byte, num;
    int totfile, filesize;
    int totbyte, nowbyte;
    struct sockaddr_in addr;
    char buf[2 * BUFF_SIZE] = {};
    string ans;
    string req;
    char str[100] = "b08902012_", str2[100];
    strcat(str, argv[1]);
    strcat(str, "_client_folder");
    mkdir(str, 0777);
    strcat(str, "/");
    FILE *fp;
    // Get socket file descriptor
    if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
        ERR_EXIT("socket failed\n");
    }

    // Set server address
    bzero(&addr,sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr(ip.c_str());
    addr.sin_port = htons(stoi(port_st));

    // Connect to the server
    if(connect(sockfd, (struct sockaddr*)&addr, sizeof(addr)) < 0){
        ERR_EXIT("connect failed\n");
    }
    string message;
    while(1){
        printf("$ "); fflush(stdout);
        getline(cin, req);
        
        if(req == "ls"){
            strcpy(buf, "l");
            write_byte = send(sockfd, buf, 1, 0);
            memset(buf, 0, sizeof(buf));
            off = 0;
            read_byte = recv(sockfd, buf, sizeof(buf), MSG_WAITALL);
            cout << buf;
            
        }
        else if(req[0] == 'p' && req[1] == 'u' && req[2] == 't' && req[3] == ' '){
            stringstream ss;
            ss << req;
            string st;
            vector<string> putfile;
            ss >> st;
            while(ss >> st){
                putfile.push_back(st);
            }
            for(auto name : putfile){  
                strcpy(str2, str); strcat(str2, name.c_str());
                pfile = fopen(str2, "r");
                if(pfile == NULL){
                    cout << "The " << name << " doesn't exist." << endl;
                    continue;
                }
                cout << "puting " << name << "......" << endl;
                //tell put 
                strcpy(buf, "p");
                while((write_byte = send(sockfd, buf, 1, 0)) != 1);
                //tell filename
                memset(buf, 0, sizeof(buf));
                strcpy(buf, name.c_str());
                while((write_byte = send(sockfd, buf, sizeof(buf), 0)) != sizeof(buf));

                fseek(pfile, 0, SEEK_END);
                filesize = ftell(pfile);
                while((write_byte = send(sockfd, &filesize, 4, 0)) != 4);


                //start to send data
                write_byte = 0;
                int numbytes, progessing = 0;
                int off = 0;
                while(off != filesize){
                    fseek(pfile, off, SEEK_SET);
                    memset(buf, 0, sizeof(buf));
                    read_byte = fread(buf, sizeof(char), sizeof(buf), pfile);
                    write_byte = send(sockfd, buf, read_byte, 0);
                    off += write_byte;
                }
                fclose(pfile);
            }
        }
        else if(req[0] == 'g' && req[1] == 'e' && req[2] == 't' && req[3] == ' '){
            stringstream ss;
            ss << req;
            string st;
            vector<string> putfile;
            ss >> st;
            while(ss >> st){
                putfile.push_back(st);
            }
            for(auto name : putfile){     
                strcpy(str2, str); strcat(str2, name.c_str());
                //tell get 
                strcpy(buf, "g");
                while((write_byte = send(sockfd, buf, 1, 0)) != 1);
                //tell filename
                strcpy(buf, name.c_str());
                while((write_byte = send(sockfd, buf, sizeof(buf), 0)) != sizeof(buf));
                //get filesize    
                read_byte = recv(sockfd, &filesize, 4, MSG_WAITALL);  
                if(filesize == -1){
                    cout << "The " << name << " doesn't exist." << endl;
                    continue;
                }
                cout << "getting " << name << "......" << endl;
                pfile = fopen(str2, "w");
                off = 0;
                while(off != filesize){
                    read_byte = recv(sockfd, buf, sizeof(buf), 0);
                    fseek(pfile, off, SEEK_SET);
                    write_byte = fwrite(buf, sizeof(char), read_byte, pfile);
                    off += read_byte;
                }
                
                fclose(pfile);
            }
        }
        
        else if(req[0] == 'p' && req[1] == 'l' && req[2] == 'a' && req[3] == 'y' && req[4] == ' '){

            //send filename
            stringstream ss;
            ss << req;
            string st;
            ss >> st; 
            ss >> st;
            int tt = st.size();
            if(st[tt-1] != 'g' || st[tt-2] != 'p' || st[tt-3] != 'm' || st[tt-4] != '.'){
                cout << "The " << st << " is not a mpg file." << endl;
                continue;
            }
            strcpy(buf, "v");
            while((write_byte = send(sockfd, buf, 1, 0)) != 1);
            memset(buf, 0, sizeof(buf));
            strcpy(buf, st.c_str());
            while((write_byte = send(sockfd, buf, sizeof(buf), 0)) != sizeof(buf));
            
            //get width
            read_byte = recv(sockfd, &width, 4, MSG_WAITALL);
            if(width == -1){
                cout << "The " << st << " doesn't exist." << endl;
                continue;
            }
            //get height
            read_byte = recv(sockfd, &height, 4, MSG_WAITALL);
            client_img = Mat::zeros(height, width, CV_8UC3);
            if(!client_img.isContinuous()){
                client_img = client_img.clone();
            }
            int imgSize = client_img.total() * client_img.elemSize();
            cout << "playing the video..." << endl;
            while(1){
                uchar *iptr = client_img.data;
                uchar buffer[imgSize];
                off = 0;
                recv(sockfd, &imgSize, 4, MSG_WAITALL);
                if(imgSize != 0){
                    read_byte = recv(sockfd, client_img.data, imgSize, MSG_WAITALL);
                }
                char c = (char)waitKey(33.3333);

                if(c==27)
                    break;
                imshow("client", client_img);
                strcpy(buf, "o");
                send(sockfd, buf, 1, 0);
            }
            strcpy(buf, "f");
            send(sockfd, buf, 1, 0);
            destroyAllWindows();
        }
        else{
            cout << "Command not found." << endl;
        }

    }

    close(sockfd);
    return 0;
}

