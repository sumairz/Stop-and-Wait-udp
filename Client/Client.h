/*

Name: Syed Sumair Zafar Student Id: 7099347
Name: Mandeep Singh Student Id: 7163738

*/



#define STIMER 0
#define UTIMER 300000	
#include <fstream>
#define CLIENT_PORT 5000
#define REMOTE_PORT 7000
#define TRACE 1
#define MAX_TRIES 10
#define INPUT_LENGTH    40
#define HOSTNAME_LENGTH 40
#define USERNAME_LENGTH 40
#define FILENAME_LENGTH 40
#define BUFFER_SIZE 1024
#define MAX_RANDOM 256
#define SEQUENCE_WIDTH 1 

typedef enum { Get=1, Put, List, Delete } Choice;
typedef enum { TIMEOUT=1, IncomingPacket, ErrorReceive} Answer;
typedef enum { ClientReq=1, AckNumClient, AckNumServer, FileNotFound, Invalid } HandshakeType;
typedef enum { FirstData=1, Data, LastData } PacketMsgHeader;
typedef enum { HANDSHAKE=1, FRAME, FRAME_ACK, ListFiles } PacketType;

typedef struct { 
	PacketType TypeOfPacket;
	int num;
} Ack;

typedef struct {
	PacketType TypeOfPacket;
	PacketMsgHeader header;
	unsigned int seqalternate:SEQUENCE_WIDTH;
	int bufferLeng;
	char buffer[BUFFER_SIZE];
} PacketMsg;

typedef struct {
	PacketType TypeOfPacket;
	HandshakeType type;
	Choice choice;
	int ClientNum;
	int ServerNum;
	char hostname[HOSTNAME_LENGTH];
	char filename[FILENAME_LENGTH];
} ThreeWayHS;

class Client
{
    int sock;						
	struct sockaddr_in sa;			
	struct sockaddr_in sa_in;		
	int sa_in_size;
	struct timeval timeout;
	ThreeWayHS handshake;
	int random;
	WSADATA wsadata;

private:
	std::ofstream fout;			

public:
	Client(char *fn="client_log.txt"); 
	~Client();	
    void run();	

	bool SendFile(int, char *, char *, int);
	int SendReq(int, ThreeWayHS *, struct sockaddr_in *);
	int SendFrame(int, PacketMsg *);
	int SendFrameACK(int, Ack *);

	bool RcvFile(int, char *, char *, int);
	Answer RespReceived(int, ThreeWayHS *);
	Answer RcvFrame(int, PacketMsg *);
	Answer RcvFileAck(int, Ack *);

	bool listFiles(int, int);
	bool delFile(int,char *,int);

	unsigned long ResolveName(char name[]);
    void err_sys(char * fmt,...);
};
