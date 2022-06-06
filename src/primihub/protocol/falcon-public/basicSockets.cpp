/*
 * basicSockets.cpp
 *
 *  Created on: Aug 3, 2015
 *      Author: froike(Roi Inbar) 
 * 	Modified: Aner Ben-Efraim
 * 
 */
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include "basicSockets.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <iostream>
#include <netinet/tcp.h>
#include <cstdio>
#include <memory>
#include <stdexcept>
#include <array>
using namespace std;

#define bufferSize 256

#ifdef __linux__
	#include <unistd.h>
	#include <sys/socket.h>
	#include <netinet/in.h>
	#include <netdb.h>
	#define Sockwrite(sock, data, size) write(sock, data, size) 
	#define Sockread(sock, buff, bufferSize) read(sock, buff, bufferSize)
	//#define socklen_t int
#elif _WIN32
	//#pragma comment (lib, "ws2_32.lib") //Winsock Library
	#pragma comment (lib, "Ws2_32.lib")
	//#pragma comment (lib, "Mswsock.lib")
	//#pragma comment (lib, "AdvApi32.lib")
	#include<winsock.h>
	//#include <ws2tcpip.h>
	#define socklen_t int
	#define close closesocket
	#define Sockwrite(sock, data, size) send(sock, (char*)data, size, 0)
	#define Sockread(sock, buff, bufferSize) recv(sock, (char*)buff, bufferSize, 0)
	
#endif

/*GLOBAL VARIABLES - LIST OF IP ADDRESSES*/
char** localIPaddrs;
int numberOfAddresses;

//For communication measurements
CommunicationObject commObject;



std::string exec(const char* cmd) 
{
    std::array<char, 128> buffer;
    std::string result;
    std::shared_ptr<FILE> pipe(popen(cmd, "r"), pclose);
    if (!pipe) throw std::runtime_error("popen() failed!");
    while (!feof(pipe.get())) {
        if (fgets(buffer.data(), 128, pipe.get()) != NULL)
            result += buffer.data();
    }
    return result;
}

string getPublicIp()
{
	string s = exec("dig TXT +short o-o.myaddr.l.google.com @ns1.google.com");
    s = s.substr(1, s.length()-3); 
    return s;
}

char** getIPAddresses(const int domain)
{
  char** ans;
  int s;
  struct ifconf ifconf;
  struct ifreq ifr[50];
  int ifs;
  int i;

  s = socket(domain, SOCK_STREAM, 0);
  if (s < 0) {
    perror("socket");
    return 0;
  }

  ifconf.ifc_buf = (char *) ifr;
  ifconf.ifc_len = sizeof ifr;

  if (ioctl(s, SIOCGIFCONF, &ifconf) == -1) {
    perror("ioctl");
    return 0;
  }

  ifs = ifconf.ifc_len / sizeof(ifr[0]);
  numberOfAddresses = ifs+1;
  ans = new char*[ifs+1];

  string ip = getPublicIp(); 
  ans[0] = new char[ip.length()+1];
  strcpy(ans[0], ip.c_str());
  ans[0][ip.length()] = '\0';

  for (i = 1; i <= ifs; i++) {
    char* ip=new char[INET_ADDRSTRLEN];
    struct sockaddr_in *s_in = (struct sockaddr_in *) &ifr[i].ifr_addr;

    if (!inet_ntop(domain, &s_in->sin_addr, ip, INET_ADDRSTRLEN)) {
      perror("inet_ntop");
      return 0;
    }

    ans[i]=ip;
  }

  close(s);

  return ans;
}

int getPartyNum(char* filename)
{

	FILE * f = fopen(filename, "r");

	char buff[STRING_BUFFER_SIZE];
	char ip[STRING_BUFFER_SIZE];

	localIPaddrs=getIPAddresses(AF_INET);
	string tmp;
	int player = 0;
	//for (int i = 0; i < numberOfAddresses; i++)
	//	cout << localIPaddrs[i] << endl;
	while (true)
	{
		fgets(buff, STRING_BUFFER_SIZE, f);
		sscanf(buff, "%s\n", ip);
		for (int i = 0; i < numberOfAddresses; i++)
			if (strcmp(localIPaddrs[i], ip) == 0 || strcmp("127.0.0.1", ip)==0)
				return player;
		player++;
	}
	fclose(f);

}

BmrNet::BmrNet(char* host, int portno) {
	this->port = portno;
#ifdef _WIN32
	this->Cport = (PCSTR)portno;
#endif
	this->host = host;
	this->is_JustServer = false;
	for (int i = 0; i < NUMCONNECTIONS; i++) this->socketFd[i] = -1;
#ifdef _WIN32
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
	{
		printf("Failed. Error Code : %d\n", WSAGetLastError());
	}
	else printf("WSP Initialised.\n");
#endif

}

BmrNet::BmrNet(int portno) {
	this->port = portno;
	this->host = "";
	this->is_JustServer = true;
	for (int i = 0; i < NUMCONNECTIONS; i++) this->socketFd[i] = -1;
#ifdef _WIN32
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
	{
		printf("Failed. Error Code : %d\n", WSAGetLastError());
	}
	else printf("WSP Initialised.\n");
#endif
}

BmrNet::~BmrNet() {
	for (int conn = 0; conn < NUMCONNECTIONS; conn++)
	close(socketFd[conn]);
	#ifdef _WIN32
		WSACleanup();
	#endif
	//printf("Closing connection\n");
}

bool BmrNet::listenNow(){
	int serverSockFd;
	socklen_t clilen;

	struct sockaddr_in serv_addr, cli_addr[NUMCONNECTIONS];


	serverSockFd = socket(AF_INET, SOCK_STREAM, 0);
	if (serverSockFd < 0){
		cout<<"ERROR opening socket"<<endl;
		return false;
	}
	memset(&serv_addr, 0,sizeof(serv_addr));

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	serv_addr.sin_port = htons(this->port);
	
	int yes=1;
	
	if (setsockopt(serverSockFd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) == -1) 
	{
		perror("setsockopt");
		exit(1);
	}
	
	int testCounter=0;//
	while (bind(serverSockFd, (struct sockaddr *) &serv_addr,
			sizeof(serv_addr)) < 0 && testCounter<10){
		cout<<"ERROR on binding: "<< port <<endl;
		cout<<"Count: "<< testCounter<<endl;///
		perror("Binding");
		testCounter++;///
		sleep(2);
	}
	if (testCounter==10) return false;//
	///
	listen(serverSockFd, 10);
	
	for (int conn = 0; conn < NUMCONNECTIONS; conn++)
	{
		clilen = sizeof(cli_addr[conn]);
		//printf("Start listening %d! ", conn);
		this->socketFd[conn] = accept(serverSockFd,
				(struct sockaddr *) &cli_addr[conn],
				 &clilen);
		if (this->socketFd[conn] < 0){
			cout<<"ERROR on accept"<<endl;
			return false;
		}
	//use TCP_NODELAY on the socket 
		int flag = 1;
		int result = setsockopt(this->socketFd[conn],            /* socket affected */
	                          IPPROTO_TCP,     /* set option at TCP level */
	                          TCP_NODELAY,     /* name of option */
	                          (char *) &flag,  /* the cast is historical */
	                          sizeof(int));    /* length of option value */
		if (result < 0) {
		    cout << "error setting NODELAY. exiting" << endl;
		    exit (-1);
		}

		//printf("Connected %d! ", conn);
	}
	close(serverSockFd);
	return true;
}


bool BmrNet::connectNow(){
	struct sockaddr_in serv_addr[NUMCONNECTIONS];
	struct hostent *server;
	int n;

	if (is_JustServer){
		cout<<"ERROR: Never got a host... please use the second constructor"<<endl;;
		return false;
	}

	for (int conn = 0; conn < NUMCONNECTIONS; conn++)
	{
		//fprintf(stderr,"usage %s hostname port\n", host);
		socketFd[conn] = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if (socketFd[conn] < 0){
			cout << ("ERROR opening socket") << endl;
			return false;
		}

		//use TCP_NODELAY on the socket 
		int flag = 1;
		int result = setsockopt(socketFd[conn],            /* socket affected */
	                          IPPROTO_TCP,     /* set option at TCP level */
	                          TCP_NODELAY,     /* name of option */
	                          (char *) &flag,  /* the cast is historical */
	                          sizeof(int));    /* length of option value */
		if (result < 0) {
		    cout << "error setting NODELAY. exiting" << endl;
		    exit (-1);
	       }
		

		memset(&serv_addr[conn], 0, sizeof(serv_addr[conn]));
		serv_addr[conn].sin_family		= AF_INET;
		serv_addr[conn].sin_addr.s_addr	= inet_addr(host);
		serv_addr[conn].sin_port			= htons(port); 
		
		int count = 0;
		//cout << "Trying to connect to server " << endl; cout << "IP: " << host << "port " << port << endl;
		while (connect(socketFd[conn], (struct sockaddr *) &serv_addr[conn], sizeof(serv_addr[conn])) < 0)
		{
				count++;
				if (count % 50 == 0)
				    cout << "Not managing to connect. " << "Count=" << count << endl;
				sleep(1);
		}

		//printf("Connected! %d \n",count);
	}

	return true;
}



bool BmrNet::sendMsg(const void* data, int size, int conn){
	int left = size;
	int n;
	while (left > 0)
	{
		// cout << "n " << n << " size " << size << endl;
		n = Sockwrite(this->socketFd[conn], &((char*)data)[size - left], left);
		// cout << "sockwrite called " << n << endl;
		if (n < 0) {
			cout << "ERROR writing to socket" << endl;
			return false;
		}
		left -= n;
	}
	commObject.incrementSent(size);

	return true;
}

bool BmrNet::receiveMsg(void* buff, int size, int conn){
	int left = size;
	int n;
	memset(buff, 0, (unsigned long)size);
	while (left > 0)
	{
		n = Sockread(this->socketFd[conn], &((char*)buff)[size-left], left);
		if (n < 0) {
			cout << "ERROR reading from socket" << endl;
			cout << "Size = " << size << ", Left = " << left << ", n = " << n << endl;
			return false;
		}
		if (n == 0) break;
		left -= n;
	}
	commObject.incrementRecv(size);

	return true;
}