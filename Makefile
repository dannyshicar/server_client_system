# CC = gcc
CXX = g++
INCLUDE_OPENCV = `pkg-config --cflags --libs opencv`

CLIENT = client.cpp
SERVER = server.cpp
CLI = client
SER = server

all: server client 
  
server: $(SERVER)
	$(CXX) $(SERVER) -o $(SER) -std=c++17  $(INCLUDE_OPENCV)
client: $(CLIENT)
	$(CXX) $(CLIENT) -o $(CLI) -std=c++17  $(INCLUDE_OPENCV)

.PHONY: clean

clean:
	rm $(CLI) $(SER)
