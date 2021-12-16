# server_client_system
simple server client system implemented by TCP socket programming
## Description

Provide a server client system, and support four operation
1. get [file1], [file2]... : get files from server
2. put [file1], [file2]... : push files from client to server
3. ls : ls the files in server
4. play [video.mpg] : play video from server

and support multiple client connection to server executing instructions at the same time
## Implementation
* using TCP socket to send data between server and client
* using select to achieve multiple client request simultaneously
* using openCV to play the video

## Usage


To use this repo locally, start by cloning it and make it

``` bash
$ git clone https://github.com/dannyshicar/server_client_system.git
$ make
```

#### Run it locally.

``` bash
$ ./server [port]
```
``` bash
$ ./client [client_id] [ip]:[port] 
```
* [ip] [port]is the ip address, port number of the server
* After launching, a folder named b08902012_server_folder/ b08902012_<client_id>_client_folder for the server/client should be create.

#### command
``` bash
$ ls
$ put <filename1> <filename2> … <filenameN>
$ get <filename1> <filename2> … <filenameN>
$ play <mpg_videofile>
```


---

