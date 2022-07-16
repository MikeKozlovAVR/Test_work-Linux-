#include <iostream>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
using namespace std;
//Описание класса TCP_client
class TCP_client{
public:	
	char sendBuff[1025];
	char readBuff[1024];
	int sockedID = 0;
	int valread = 0;
	const char* serverIP;
	struct sockaddr_in server_addr;
	
	TCP_client(const char* serverIP_, int port);
	bool connectToServer();
	bool readData(int sockedID_);
	void writeData(int sockedID_, char* data);
	void closeConnection();
	void clearReadBuff();
	void clearSendBuff();
};
TCP_client::TCP_client(const char* serverIP_, int port){
	memset(this->readBuff, 0, sizeof(this->readBuff));
	memset(this->sendBuff, 0, sizeof(this->sendBuff));
	if((this->sockedID = socket(AF_INET, SOCK_STREAM, 0)) < 0)  //create socked IPv4 TCP
	{
		printf("\nError : Could not create socket \n");
	}
	memset(&server_addr, '0', sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(port);
	this->serverIP = serverIP_;
}
bool TCP_client::connectToServer(){
	bool connectStatus = 0;
	if(inet_pton(AF_INET, this->serverIP, &server_addr.sin_addr)<=0)
	{
		connectStatus = 0;
	}
	else
	{
		connectStatus = 1;
	}
	if( connect(this->sockedID, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
	{
		connectStatus = 0;
	}
	else
	{
		connectStatus = 1;
	}
	return connectStatus;
}
bool TCP_client::readData(int sockedID_){
	bool readed = 0;
	int valread = 0;
	//if(valread = read(sockedID_, this->readBuff, 1024) > 0)
	if(valread = recv(sockedID_, this->readBuff, 1024, 0) > 0)
	{
		readed = 1;
	}
	else
	{
		readed = 0;
	}
	return readed;
}
void TCP_client::writeData(int sockedID_, char* data){
	snprintf(this->sendBuff, sizeof(this->sendBuff), data);
	//write(sockedID_, this->sendBuff, strlen(this->sendBuff));
	send(sockedID_, this->sendBuff, strlen(this->sendBuff), 0); 
	memset(sendBuff, 0, 1025);
}
void TCP_client::closeConnection(){
	close(this->sockedID);
}
void TCP_client::clearReadBuff(){
	memset(this->readBuff, 0, 1024);
}
void TCP_client::clearSendBuff(){
	memset(this->sendBuff, 0, 1025);
}

//-------------------Описание класса TCP_server-----------
class TCP_server{
public:
	char sendBuff[1025];
	char readBuff[1024];
	int listenID = 0;
	int connectID = 0;
	struct sockaddr_in server_addr;
	
	TCP_server(int port, int maxClients);
	int clientIsConnected();
	void writeData(int connectID_, char* data);
	bool readData(int connectID_);
	void closeConnection();
	void clearReadBuff();
	void clearSendBuff();
};

TCP_server::TCP_server(int port, int maxClients){
	listenID = socket(AF_INET, SOCK_STREAM, 0); //create socket/sockettype IPv4/protocoltype TCP
	memset(&server_addr, '0', sizeof(server_addr));
	memset(this->sendBuff, 0, sizeof(this->sendBuff));
	memset(this->readBuff, 0, sizeof(this->readBuff));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);   //any IP (127.0.0.1)
	server_addr.sin_port = htons(port); 				 //server port
	bind(listenID, (struct sockaddr*)&server_addr, sizeof(server_addr)); //start with serv_addr
	listen(this->listenID, maxClients);
}
int TCP_server::clientIsConnected(){
	int connectID_ = 0;
	while(1)
	{
		connectID_ = accept(this->listenID, (struct sockaddr*)NULL, NULL);
		break;
	}
	return connectID_;
}
void TCP_server::writeData(int connectID_, char* data){
	snprintf(this->sendBuff, sizeof(this->sendBuff), data);
	//write(connectID_, this->sendBuff, strlen(this->sendBuff));
	send(connectID_, this->sendBuff, strlen(this->sendBuff), 0);
	memset(sendBuff, 0, sizeof sendBuff);
}
bool TCP_server::readData(int connectID_){
	bool readed = 0;
	int valread = 0;
	//if(valread = read(connectID_, this->readBuff, 1024) > 0)
	if(valread = recv(connectID_, this->readBuff, 1024, 0) > 0)
	{
		readed = 1;
	}
	else
	{
		readed = 0;
	}
	
	return readed;
}
void TCP_server::closeConnection(){
	close(this->connectID);
}
void TCP_server::clearReadBuff(){
	memset(this->readBuff, 0, 1024);
}
void TCP_server::clearSendBuff(){
	memset(this->sendBuff, 0, 1025);
}


