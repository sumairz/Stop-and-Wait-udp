/*

Name: Syed Sumair Zafar Student Id: 7099347
Name: Mandeep Singh Student Id: 7163738

*/

#define HOST_NAME_LEN 40
#define USERNAME_LENGTH 40
#define FILENAME_LENGTH 40
#define BUFFER_SIZE 1024
#define STIMER 0
#define MAX_RANDOM 256;
#define STOPnWAIT 1
#define UTIMER 300000
#define SERVER_PORT 5001
#define REMOTE_PORT 7001
#define TRACE 1
#define MaxTries 10

using namespace std ;

#include <winsock.h>
#include <fstream>
#include <iostream>
#include <time.h>
#include <list>
#include <stdio.h>

typedef enum { Get=1, Put, List, Delete } Choice;
typedef enum { TIMEOUT=1, IncomingPacket, ErrorRecieved} Answer;
typedef enum { ClientReq=1, AckNumClient, AckNumServer, FileNotFound, Invalid } HandshakeType;
typedef enum { FirstData=1, Data, LastData } PacketMsgHeader;
typedef enum { HANDSHAKE=1, FRAME, FrameACK, ListFiles } PacketType;

typedef struct {
	PacketType TypeOfPacket;
	HandshakeType type;
	Choice choice;
	int ClientNum;
	int ServerNum;
	char hostname[HOST_NAME_LEN];
	char filename[FILENAME_LENGTH];
} ThreeWayHS;

typedef struct { 
	PacketType TypeOfPacket;
	int num;
} Ack;

typedef struct {
	PacketType TypeOfPacket;
	PacketMsgHeader header;
	unsigned int seqalternate:STOPnWAIT;
	int bufferLeng;
	char buffer[BUFFER_SIZE]; 
} PacketMsg;

class Server
{
	int sock;					
	struct sockaddr_in sa;		
	struct sockaddr_in sa_in;	
	int sa_in_size;
	char server_name[HOST_NAME_LEN];
	struct timeval timeout;
	int random;
	WSADATA wsadata;

	ThreeWayHS handshake;

private:
	 ofstream fout;			

public: 
	Server(char *fn="server_log.txt");	
	~Server();
	void run();

	bool SendFile(int, char *, char *, int);
	bool RcvFile(int, char *, char *, int);
	int SendingReq(int, ThreeWayHS *, struct sockaddr_in *);
	Answer RespReceived(int, ThreeWayHS *);
	int SendFrame(int, PacketMsg *);
	int SendFrameACK(int, Ack *);
	Answer RcvFrame(int, PacketMsg *);
	Answer RcvFileAck(int, Ack *);

	bool listFiles(int, int);
	bool delFile(int, char *, int);
	unsigned long ResolveName(char name[]);
    void err_sys(char * fmt,...);
};

