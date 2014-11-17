/*

Name: Syed Sumair Zafar Student Id: 7099347
Name: Mandeep Singh Student Id: 7163738

*/


#pragma comment(lib,"wsock32.lib")
#define _CRT_SECURE_NO_DEPRECATE

#include <winsock.h>
#include <iostream>
#include <fstream>
#include <windows.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <process.h>
#include "client.h"

using namespace std;

bool FileExists(char * filename)
{
	std::ifstream stream(filename);
	return(stream!=NULL);	
	
}

long GetFileSize(char * filename)
{
	FILE* fp=fopen(filename,"rb");
	fseek(fp,0,SEEK_END);
	LONG size=ftell(fp);
	fclose(fp);
	return size;	
}


bool Client::SendFile(int sock, char * filename, char * SenderHostname, int ServerNum)
{	
	Ack ack; 
	ack.num = -1;
	PacketMsg packet; 
	packet.TypeOfPacket = FRAME;

	long SizeInBytes = 0, BytesCount = 0;
	int numTries;
	bool aMaxTries = false;
	bool BgnFirst = true, DoneLast = false;

	int sequence_number = ServerNum % 2; 
	int BytesSent = 0, BytesRead = 0, TotalBytesRead = 0;
	int SentPacketsEffective = 0, SentPackets = 0;


	if (TRACE) { fout << "Sender: started on hostname " << SenderHostname << endl; }

	FILE * stream = fopen(filename, "r+b");

	if (stream != NULL)
	{
		SizeInBytes = GetFileSize(filename);

		
		while (1) 
		{ 
			if ( SizeInBytes > BUFFER_SIZE)
			{
				packet.header = ( BgnFirst ? FirstData : Data) ;
				BytesCount = BUFFER_SIZE;
			}
			else 
			{
				BytesCount = SizeInBytes;			
				DoneLast = true;
				packet.header = LastData;
			}
			
			SizeInBytes -= BUFFER_SIZE; 
			
			BytesRead = fread(packet.buffer, sizeof(char), BytesCount, stream); 
			
			TotalBytesRead += BytesRead;
			
			packet.bufferLeng = BytesCount;
			
			packet.seqalternate = sequence_number;

			numTries = 0; 
			do
			{
				numTries++;

				if ( SendFrame(sock, &packet) != sizeof(packet) )
					return false;
				SentPacketsEffective++;
				
				if (numTries == 1)
					SentPackets++; 
				BytesSent += sizeof(packet);

				cout << "Sender: sent packet " << sequence_number << " and data is : "<<BUFFER_SIZE <<endl;
				if (TRACE) { fout << "Sender: sent packet " << sequence_number << endl; }
				
				if (DoneLast && (numTries > MAX_TRIES))
				{ 
					aMaxTries = true;
					break;
				}

			}
			while ( RcvFileAck(sock, &ack) != IncomingPacket || ack.num != sequence_number );
			
			if (aMaxTries) 
			{
				cout << "Sender: did not receive ACK for the packet number " << sequence_number << " after maxtries." << endl;
				if (TRACE) { fout << "Sender: did not receive ACK for packet number " << sequence_number << " after maxtries." << endl; }
			}
			else
			{
				cout << "Sender: received ACK number " << ack.num << endl;
				if (TRACE) { fout << "Sender: received ACK number " << ack.num << endl; }
			}

			BgnFirst = false;

			
			sequence_number = (sequence_number == 0 ? 1 : 0);

			
			if (DoneLast) 
				break;
		}

		fclose( stream );

		cout << "Sender: file transfer complete" << endl;
		cout << "Sender: number of effective packets sent : " << SentPacketsEffective << endl;
		cout << "Sender: number of packets sent : " << SentPackets << endl;
		cout << "Sender: number of bytes sent: " << BytesSent << endl;
		cout << "Sender: number of bytes read: " << TotalBytesRead << endl << endl;
		
		
		if (TRACE) 
		{ 
			fout << "Sender: file transfer complete" << endl;
			fout << "Sender: number of effective packets sent :" << SentPacketsEffective << endl;
			fout << "Sender: number of packets sent : " << SentPackets << endl;
			fout << "Sender: number of bytes sent: " << BytesSent << endl;
			fout << "Sender: number of bytes read: " << TotalBytesRead << endl << endl;	
		}
		return true;
	}
	else
	{
		cout << "Sender: could not open  file." << endl;
        if (TRACE) { fout << "Sender: could not open  file." << endl; }
		return false;		
	}
}

int Client::SendReq(int sock, ThreeWayHS * handshakePTR, struct sockaddr_in * sa_in)
{
	
	return sendto(sock, (const char *)handshakePTR, sizeof(*handshakePTR), 0, (struct sockaddr *)sa_in, sizeof(*sa_in));
}


int Client::SendFrame(int sock, PacketMsg * messagePTR)
{
	return sendto(sock, (const char*)messagePTR, sizeof(*messagePTR), 0, (struct sockaddr*)&sa_in, sizeof(sa_in));
}


int Client::SendFrameACK(int sock, Ack * ack)
{
	return sendto(sock, (const char*)ack, sizeof(*ack), 0, (struct sockaddr*)&sa_in, sizeof(sa_in));
}

bool Client::RcvFile(int sock, char * filename, char * receiving_hostname, int ClientNum)
{
	
	PacketMsg packet;
	Ack ack; ack.TypeOfPacket = FRAME_ACK;
	long BytesCount = 0;
	int SentPacketsEffective = 0, SentPackets = 0;
	int ReceivedBytes = 0, WrittenBytes = 0, TotalWrittenBytes = 0;
	int sequence_number = ClientNum % 2;

	if (TRACE) { fout << "Receiver started on host " << receiving_hostname << endl; }

	
	FILE * stream = fopen(filename, "w+b");
	
	if (stream != NULL)
	{
		while (1) 
		{ 
			
			while( RcvFrame(sock, &packet) != IncomingPacket ) {;}
			
			ReceivedBytes += sizeof(packet);
			
			if (packet.TypeOfPacket == HANDSHAKE) 
			{
				cout << "Receiver: received handshake from client:" << handshake.ClientNum << " server:" << handshake.ServerNum << endl;
				if (TRACE) { fout << "Receiver: received from client:" << handshake.ClientNum << " server:" << handshake.ServerNum << endl; }
				if ( SendReq(sock, &handshake, &sa_in) != sizeof(handshake) )
					err_sys("Error in sending packet.");
				cout << "Receiver: sent to client:" << handshake.ClientNum << " server:" << handshake.ServerNum << endl;
				if (TRACE) { fout << "Receiver: sent handshake to client:" << handshake.ClientNum << " server:" << handshake.ServerNum << endl; }
			}
			else if (packet.TypeOfPacket == FRAME)
			{
				cout << "Receiver: received packet number " << (int)packet.seqalternate << " and data is : "<<ReceivedBytes<<endl;
				if (TRACE) { fout << "Receiver: received packet number " << (int)packet.seqalternate << endl; }
				
				if ( (int)packet.seqalternate != sequence_number ) 
				{
					ack.num = (int)packet.seqalternate;
					if ( SendFrameACK(sock, &ack) != sizeof(ack) )
						return false;
					cout << "Receiver: sent ACK " << ack.num << " again" << endl;
					SentPacketsEffective++;
					if (TRACE) { fout << "Receiver: sent ACK " << ack.num << " again" << endl; }
				}
				else 
				{
					
					ack.num = (int)packet.seqalternate;	
					if ( SendFrameACK(sock, &ack) != sizeof(ack) )
						return false;

					cout << "Receiver: sent ACK number " << ack.num << endl;
					if (TRACE) { fout << "Receiver: sent ACK number " << ack.num << endl; }

					SentPacketsEffective++;
					SentPackets++;

					
					BytesCount = packet.bufferLeng;
					WrittenBytes = fwrite(packet.buffer, sizeof(char), BytesCount, stream );
					TotalWrittenBytes += WrittenBytes;
					
					
					sequence_number = (sequence_number == 0 ? 1 : 0);
					
					
					if (packet.header == LastData)
						break;
				}
			}
		}
		
		fclose( stream );

		cout << "Receiver: file transfer complete" << endl;
		cout << "Receiver: number of effective packets sent :" << SentPacketsEffective << endl;
		cout << "Receiver: number of packets sent  " << SentPackets << endl;
		cout << "Receiver: number of bytes received: " << ReceivedBytes << endl;
		cout << "Receiver: number of bytes written: " << TotalWrittenBytes << endl << endl;

		if (TRACE) 
		{ 
			fout << "Receiver: file transfer complete" << endl;
			fout << "Receiver: number of effective packets sent :" << SentPacketsEffective << endl;
			fout << "Receiver: number of packets sent  " << SentPackets << endl;
			fout << "Receiver: number of bytes received: " << ReceivedBytes << endl;
			fout << "Receiver: number of bytes written: " << TotalWrittenBytes << endl << endl;
		}
		return true;
	}
	else
	{
		cout << "Receiver: not able to open the file." << endl;
        if (TRACE) { fout << "Receiver: not able to open the file." << endl; }
		return false;
	}
}

Answer Client::RespReceived(int sock, ThreeWayHS * handshakePTR)
{
	fd_set readfds;			
	FD_ZERO(&readfds);		
	FD_SET(sock, &readfds);	
	int iBytesRecv;
	int outfds = select(1 , &readfds, NULL, NULL, &timeout);
	switch (outfds)
	{
		case 0:
			return TIMEOUT; break;
		case 1:
			iBytesRecv = recvfrom(sock, (char *)handshakePTR, sizeof(*handshakePTR),0, (struct sockaddr*)&sa_in, &sa_in_size);
			return IncomingPacket; break;
		default:
			return ErrorReceive; break;
	}
}

Answer Client::RcvFrame(int sock, PacketMsg * messagePTR)
{
	fd_set readfds;			
	FD_ZERO(&readfds);		
	FD_SET(sock, &readfds);	
	int iBytesRecv;
	int outfds = select(1 , &readfds, NULL, NULL, &timeout);
	switch (outfds)
	{
		case 0:
			return TIMEOUT; break;
		case 1:
			iBytesRecv = recvfrom(sock, (char *)messagePTR, sizeof(*messagePTR),0, (struct sockaddr*)&sa_in, &sa_in_size);
			return IncomingPacket; break;
		default:
			return ErrorReceive; break;
	}
}

Answer Client::RcvFileAck(int sock, Ack * ack)
{
	fd_set readfds;
	FD_ZERO(&readfds);
	FD_SET(sock, &readfds); 
	int iBytesRecv;
	int outfds = select(1 , &readfds, NULL, NULL, &timeout);
	switch (outfds)
	{
		case 0:
			return TIMEOUT; break;
		case 1:
			iBytesRecv = recvfrom(sock, (char *)ack, sizeof(*ack),0, (struct sockaddr*)&sa_in, &sa_in_size);
			return IncomingPacket;
			break;
		default:
			return ErrorReceive; 
			break;
	}
}


bool Client::listFiles(int sock, int ServerNum)
{
	
	Ack ack; 
	ack.num = -1;
	PacketMsg packet; 
	packet.TypeOfPacket = FRAME;

	long SizeInBytes = 0, BytesCount = 0;
	int numTries;
	bool aMaxTries = false;
	bool BgnFirst = true, DoneLast = false;

	int sequence_number = ServerNum % 2; 
	int BytesSent = 0, BytesRead = 0, TotalBytesRead = 0;
	int SentPacketsEffective = 0, SentPackets = 0;

	packet.header = ( BgnFirst ? FirstData : Data) ;
	
	sprintf(packet.buffer, "4");
							
	packet.bufferLeng = BytesCount;
	
	packet.seqalternate = sequence_number;
	
	SendFrame(sock, &packet);
	if (RcvFrame(sock, &packet))
	{
		cout << endl;
		cout << "====================";
		cout << " Serever Files ";
		cout << "====================";
		cout << endl;
		cout << packet.buffer;
		cout << "====================";
		cout << endl;
	}
	return true;

}

bool Client::delFile(int sock,char *file, int ServerNum)
{
	
	Ack ack; 
	ack.num = -1;
	PacketMsg packet; 
	packet.TypeOfPacket = FRAME;

	long SizeInBytes = 0, BytesCount = 0;
	int numTries;
	bool aMaxTries = false;
	bool BgnFirst = true, DoneLast = false;

	int sequence_number = ServerNum % 2; 
	int BytesSent = 0, BytesRead = 0, TotalBytesRead = 0;
	int SentPacketsEffective = 0, SentPackets = 0;

	packet.header = ( BgnFirst ? FirstData : Data) ;
	
	sprintf(packet.buffer, file);
							
	packet.bufferLeng = BytesCount;
	
	packet.seqalternate = sequence_number;
	
	SendFrame(sock, &packet);
	if (RcvFrame(sock, &packet))
	{
		cout << "File has been deleted!";
	}
	return true;

}


void Client::run()
{
	char server[INPUT_LENGTH]; char filename[INPUT_LENGTH]; char choice[INPUT_LENGTH];
	char hostname[HOSTNAME_LENGTH]; char username[USERNAME_LENGTH]; char remotehost[HOSTNAME_LENGTH];
	unsigned long filename_length = (unsigned long)	FILENAME_LENGTH;
	bool running = true;
	//char file[100];

	if (WSAStartup(0x0202,&wsadata) != 0)
	{  
		WSACleanup();  
	    err_sys("Error in starting WSAStartup()\n");
	}

	if ( !GetUserName(username, &filename_length) )
		err_sys("Error getting username");

	if ( gethostname(hostname, (int)HOSTNAME_LENGTH) != 0 ) 
		err_sys("Error getting host name");

	cout <<"Client started" << endl;
	
	cout << "Enter server name : "; 
	cin >> server;	

	while ( strcmp(server, "exit") != 0 )
	{
		if ( (sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0 )
			err_sys("socket() failed");
		
		
		memset(&sa, 0, sizeof(sa)); 
		sa.sin_family = AF_INET; 
		sa.sin_addr.s_addr = htonl(INADDR_ANY); 
		sa.sin_port = htons(CLIENT_PORT); 
				
	
		if (bind( sock, (LPSOCKADDR)&sa, sizeof(sa) ) < 0)
			err_sys("Socket binding error");		

		srand((unsigned)time(NULL));
		random = rand() % MAX_RANDOM; 

		
		cout << "Enter router hostname : "; 
		cin >> remotehost;
		
		cout << "Enter get/put/list/delete : "; cin >> choice; cout << endl;
		strcpy(handshake.hostname, hostname);
		
		if ( running )
		{
			
			struct hostent *rp; 
			rp = gethostbyname(remotehost);
			memset(&sa_in, 0, sizeof(sa_in) );
			memcpy(&sa_in.sin_addr, rp->h_addr, rp->h_length); 
			sa_in.sin_family = rp->h_addrtype;
			sa_in.sin_port = htons(REMOTE_PORT);
			sa_in_size = sizeof(sa_in);

			handshake.ClientNum = random;
			handshake.type = ClientReq;
			handshake.TypeOfPacket = HANDSHAKE;

			
			if ( strcmp(choice, "get") == 0 )
			{
				cout << "Enter file name   : "; 
				cin >> filename;

		
				strcpy(handshake.filename, filename);
		
				handshake.choice = Get;
			}
			else if ( strcmp(choice, "put") == 0 )
			{
				cout << "Enter file name   : "; 
				cin >> filename;

		
				strcpy(handshake.filename, filename);
		
				
				if ( !FileExists(handshake.filename) )
					err_sys("File was not found !");
				else
					handshake.choice = Put;
			}
			else if( strcmp(choice, "list") == 0 )
			{
				handshake.choice = List;
			}
			else if ( strcmp(choice, "delete") == 0 )
			{
				cout << "Enter file name   : "; 
				cin >> filename;

		
				strcpy(handshake.filename, filename);
	
				handshake.choice = Delete;
			}
			else
				err_sys("Invalid choice!");

			
			do
			{
				if ( SendReq(sock, &handshake, &sa_in) != sizeof(handshake) )
					err_sys("Error in sending packet.");
				
				cout << "Client: sent handshake client:" << handshake.ClientNum << endl;
				if (TRACE) { fout << "Client: sent handshake client: " << handshake.ClientNum << endl; }
				
			}
			while ( RespReceived(sock, &handshake) != IncomingPacket );

			
		
			if (handshake.type == FileNotFound)
			{
				cout << "File not found." << endl;
				if (TRACE) { fout << "Client: requested file not found." << endl; }
			}
			else if (handshake.type == Invalid)
			{
				cout << "Invalid request." << endl;
				if (TRACE) { fout << "Client: Invalid request." << endl; }
			}

			

			if (handshake.type == AckNumClient) 
			{
				cout << "Client: received handshake client:" << handshake.ClientNum << " server: " << handshake.ServerNum << endl;
				if (TRACE) { fout << "Client: received handshake client:" << handshake.ClientNum << " server: " << handshake.ServerNum << endl; }
				
				
				handshake.type = AckNumServer;
				int sequence_number = handshake.ServerNum % 2;
				if ( SendReq(sock, &handshake, &sa_in) != sizeof(handshake) )
					err_sys("Error in sending packet.");

				cout << "Client: sent handshake client: " << handshake.ClientNum << " server:" << handshake.ServerNum << endl;
				if (TRACE) { fout << "Client: sent handshake client:" << handshake.ClientNum << " server:" << handshake.ServerNum << endl; }

				switch (handshake.choice)
				{
					case Get: 
						if ( ! RcvFile(sock, handshake.filename, hostname, handshake.ClientNum) )
							err_sys("An error occurred while receiving the file.");
						break;
					case Put: 
						if ( ! SendFile(sock, handshake.filename, hostname, handshake.ServerNum) )
							err_sys("An error occurred while sending the file.");
						break;
					case List:
						//cout<<"Hello";
						listFiles(sock, handshake.ServerNum);
						break;
					case Delete:
						delFile(sock, handshake.filename,handshake.ServerNum);
						break;
					default:
						break;
				}
			}
		}
			
		cout << " client socket is closed." << endl;
		if (TRACE) { fout << "client socket is closed." << endl; }
		
		closesocket(sock);
		
		cout << "Enter server name : "; 
		cin >> server;
	}
    
}

void Client::err_sys(char * fmt,...)
{ 
	perror(NULL);
	va_list args;
	va_start(args,fmt);
	fprintf(stderr,"error: ");
	vfprintf(stderr,fmt,args);
	fprintf(stderr,"\n");
	va_end(args);
	
	getchar();

	exit(1);
}

unsigned long Client::ResolveName(char name[])
{
	struct hostent *host; 
	if ((host = gethostbyname(name)) == NULL)
		err_sys("gethostbyname() failed");
	return *((unsigned long *) host->h_addr_list[0]); 
}

Client::Client(char * fn) 
{
	
	timeout.tv_sec = STIMER;
	timeout.tv_usec = UTIMER;
	
	fout.open(fn);
} 

Client::~Client() 
{
	
	fout.close();

	WSACleanup();
}


int main(int argc, char *argv[]) {

	Client * cli = new Client();
	cli->run();
	return 0;
}