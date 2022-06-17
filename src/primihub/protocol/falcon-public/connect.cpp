

#include "connect.h"
#include <thread>
#include <mutex> 
#include "secCompMultiParty.h"
#include <vector>

using namespace std;

namespace primihub{
    namespace falcon
{
#define STRING_BUFFER_SIZE 256
extern void error(string str);


//this player number
extern int partyNum;

//communication
string * addrs;
BmrNet ** communicationSenders;
BmrNet ** communicationReceivers;

//Communication measurements object
extern CommunicationObject commObject;

//setting up communication
void initCommunication(string addr, int port, int player, int mode)
{
	char temp[25];
	strcpy(temp, addr.c_str());
	if (mode == 0)
	{
		communicationSenders[player] = new BmrNet(temp, port);
		communicationSenders[player]->connectNow();
	}
	else
	{
		communicationReceivers[player] = new BmrNet(port);
		communicationReceivers[player]->listenNow();
	}
}


void initializeCommunication(int* ports)
{
	int i;
	communicationSenders = new BmrNet*[NUM_OF_PARTIES];
	communicationReceivers = new BmrNet*[NUM_OF_PARTIES];
	thread *threads = new thread[NUM_OF_PARTIES * 2];
	for (i = 0; i < NUM_OF_PARTIES; i++)
	{
		if (i != partyNum)
		{
			threads[i * 2 + 1] = thread(initCommunication, addrs[i], ports[i * 2 + 1], i, 0);
			threads[i * 2] = thread(initCommunication, "127.0.0.1", ports[i * 2], i, 1);
		}
	}
	for (int i = 0; i < 2 * NUM_OF_PARTIES; i++)
	{
		if (i != 2 * partyNum && i != (2 * partyNum + 1))
			threads[i].join();//wait for all threads to finish
	}

	delete[] threads;
}

void initializeCommunicationSerial(int* ports)//Use this for many parties
{
	communicationSenders = new BmrNet*[NUM_OF_PARTIES];
	communicationReceivers = new BmrNet*[NUM_OF_PARTIES];
	for (int i = 0; i < NUM_OF_PARTIES; i++)
	{
		if (i<partyNum)
		{
		  initCommunication( addrs[i], ports[i * 2 + 1], i, 0);//connect
		  initCommunication("127.0.0.1", ports[i * 2], i, 1);//listen
		}
		else if (i>partyNum)
		{
		  initCommunication("127.0.0.1", ports[i * 2], i, 1);//listen
		  initCommunication( addrs[i], ports[i * 2 + 1], i, 0);//connect
		}
	}
}
  void initializeCommunication(std::vector<std::pair<std::string, uint16_t>> listen_addr, 
               std::vector<std::pair<std::string, uint16_t>> connect_addr)
			   {
							communicationSenders = new BmrNet*[NUM_OF_PARTIES];
							communicationReceivers = new BmrNet*[NUM_OF_PARTIES];
							//for every single node ,it has four ip:port addr
							//keep the addr index in an incremental order ,eg: 0：12  1 :02  2 :01
							if (partyNum == PARTY_A)
							{
								initCommunication(listen_addr[0].first, listen_addr[0].second, 1, 1);//server for 1:receiver and listen
								initCommunication(connect_addr[0].first, connect_addr[0].second, 1, 0);//client for 1 :sender connect
								initCommunication(listen_addr[1].first, listen_addr[1].second, 2, 1);//server for 2 :receiver listen
								initCommunication(connect_addr[1].first, connect_addr[1].second, 2, 0);//client for 2 :sender connect
							}
							else if (partyNum == PARTY_B)
							{
								initCommunication(connect_addr[0].first, connect_addr[0].second, 0, 0);
								initCommunication(listen_addr[0].first, listen_addr[0].second, 0, 1);
								initCommunication(listen_addr[1].first, listen_addr[1].second, 2, 1);
								initCommunication(connect_addr[1].first, connect_addr[1].second, 2, 0);
							}
							else if (partyNum == PARTY_C)
							{
								initCommunication(connect_addr[0].first, connect_addr[0].second, 0, 0);
								initCommunication(listen_addr[0].first, listen_addr[0].second, 0, 1);
								initCommunication(connect_addr[1].first, connect_addr[1].second, 1, 0);
								initCommunication(listen_addr[1].first, listen_addr[1].second, 1, 1);
							}	
			   }
			   
void initializeCommunication(char* filename, int p)
{
	FILE * f = fopen(filename, "r");
	partyNum = p;
	char buff[STRING_BUFFER_SIZE];
	char ip[STRING_BUFFER_SIZE];
	
	addrs = new string[NUM_OF_PARTIES];
	int * ports = new int[NUM_OF_PARTIES * 2];


	for (int i = 0; i < NUM_OF_PARTIES; i++)
	{
		fgets(buff, STRING_BUFFER_SIZE, f);
		sscanf(buff, "%s\n", ip);
		addrs[i] = string(ip);
		//cout << addrs[i] << endl;
		ports[2 * i] = 32000 + i*NUM_OF_PARTIES + partyNum;
		ports[2 * i + 1] = 32000 + partyNum*NUM_OF_PARTIES + i;
	}

	fclose(f);
	initializeCommunicationSerial(ports);

	delete[] ports;
}


//synchronization functions
void sendByte(int player, char* toSend, int length, int conn)
{
	communicationSenders[player]->sendMsg(toSend, length, conn);
	// totalBytesSent += 1;
}

void receiveByte(int player, int length, int conn)
{
	char *sync = new char[length+1];
	communicationReceivers[player]->receiveMsg(sync, length, conn);
	delete[] sync;
	// totalBytesReceived += 1;
}

void synchronize(int length)
{
	char* toSend = new char[length+1];
	memset(toSend, '0', length+1);
	vector<thread *> threads;
	for (int i = 0; i < NUM_OF_PARTIES; i++)
	{
		if (i == partyNum) continue;
		for (int conn = 0; conn < NUMCONNECTIONS; conn++)
		{
			threads.push_back(new thread(sendByte, i, toSend, length, conn));
			threads.push_back(new thread(receiveByte, i, length, conn));
		}
	}
	for (vector<thread *>::iterator it = threads.begin(); it != threads.end(); it++)
	{
		(*it)->join();
		delete *it;
	}
	threads.clear();
	delete[] toSend;
}


void start_communication()
{
	if (commObject.getMeasurement())
		error("Nested communication measurements");

	commObject.reset();
	commObject.setMeasurement(true);
}

void pause_communication()
{
	if (!commObject.getMeasurement())
		error("Communication never started to pause");

	commObject.setMeasurement(false);
}

void resume_communication()
{
	if (commObject.getMeasurement())
		error("Communication is not paused");

	commObject.setMeasurement(true);
}

void end_communication(string str)
{
	cout << "----------------------------------------------" << endl;
	cout << "Communication, " << str << ", P" << partyNum << ": " 
		 << (float)commObject.getSent()/1000000 << "MB (sent) " 
		 << (float)commObject.getRecv()/1000000 << "MB (recv)" << endl;
	cout << "Rounds, " << str << ", P" << partyNum << ": " 
		 << commObject.getRoundsSent() << "(sends) " 
		 << commObject.getRoundsRecv() << "(recvs)" << endl; 
	cout << "----------------------------------------------" << endl;	
	commObject.reset();
}
}// namespace primihub{
}