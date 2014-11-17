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
#include "server.h"
#include <vector>
#include <string>
#include <direct.h>

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


bool Server::SendFile(int sock, char * filename, char * SenderHostname, int ClientNum)
{	
	PacketMsg packet;
	packet.TypeOfPacket = FRAME;
	Ack ack;
	ack.num = -1;
	long SizeInBytes = 0, BytesCount = 0;
	int iBytesSent = 0, BytesRead = 0, TotalBytesRead = 0;
	int EffectiveSentPackets = 0, SentPackets = 0;
	bool BgnFirst = true, DoneLast = false;
	int seq_no = ClientNum % 2; 
	int numTries;
	bool aMaxTries = false;
		
	if (TRACE) { fout << "Sender: started on host " << SenderHostname << endl; }// log file

	
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
			
			packet.seqalternate = seq_no;	
			
			
			numTries = 0; 
			do 
			{
				numTries++;
				if ( SendFrame(sock, &packet) != sizeof(packet) )
					return false;
				EffectiveSentPackets++;

				
				if (numTries == 1)
					SentPackets++; 
				iBytesSent += sizeof(packet); 
				
				cout << "Sender: sent packets " << seq_no <<" and data is :"<<BUFFER_SIZE<< endl;
				if (TRACE) { fout << "Sender: sent packets " << seq_no << endl; }
				
				if (DoneLast && (numTries > MaxTries))
				{ 
					aMaxTries = true;
					break;
				}

			}
			while ( RcvFileAck(sock, &ack) != IncomingPacket || ack.num != seq_no );
			
			if (aMaxTries) 
			{
				cout <<  "Sender: did not receive ACK for the packet number " << seq_no << " after maxtries." << endl;
				if (TRACE) 
				{ 
					fout <<  "Sender: did not receive ACK for the packet number " << seq_no << " after maxtries"  << endl; 
				}
			}
			else
			{
				cout << "Sender: received the ACK number " << ack.num << endl;
				if (TRACE) { fout << "Sender: received ACK number " << ack.num << endl; }
			}
			
			BgnFirst = false;
			
			seq_no = (seq_no == 0 ? 1 : 0);
			
			
			if (DoneLast) 
				break;
		}
		
		fclose( stream );
		cout << "Sender: file transfer complete" << endl;
		cout << "Sender: number of packets sent: " << EffectiveSentPackets <<" packets "<< endl;
		cout << "Sender: number of effective packets sent :" << SentPackets <<" packets "<< endl;
		cout << "Sender: number of bytes sent: " << iBytesSent<<" bytes " << endl;
		cout << "Sender: number of bytes read: " << TotalBytesRead <<" bytes "<< endl << endl;
		
		if (TRACE) 
		{ 
			fout << "Sender: file transfer completed" << endl;
			fout << "Sender: number of packets sent: " << EffectiveSentPackets <<" packets"<< endl;			
			fout << "Sender: number of effective packets sent : " << SentPackets<<" packets" << endl;
			fout << "Sender: number of bytes sent: " << iBytesSent <<"bytes"<< endl;
			fout << "Sender: number of bytes read: " << TotalBytesRead <<"bytes"<< endl << endl;	
		}

		return true;
	}
	else
	{
		cout << "Sender: could not open file." << endl;
        if (TRACE) { fout << "Sender: could not open file." << endl; }
		return false;		
	}
}


int Server::SendingReq(int sock, ThreeWayHS * handshakePTR, struct sockaddr_in * sa_in) 
{
	int iBytesSent = sendto(sock, (const char *)handshakePTR, sizeof(*handshakePTR), 0, (struct sockaddr *)sa_in, sizeof(*sa_in));
	return iBytesSent;
}


int Server::SendFrame(int sock, PacketMsg * messagePTR)
{
	int iBytesSent = sendto(sock, (const char*)messagePTR, sizeof(*messagePTR), 0, (struct sockaddr*)&sa_in, sizeof(sa_in));
	return iBytesSent;
}

int Server::SendFrameACK(int sock, Ack * ack)
{
	int iBytesSent = sendto(sock, (const char*)ack, sizeof(*ack), 0, (struct sockaddr*)&sa_in, sizeof(sa_in));
	return iBytesSent;
}

bool Server::RcvFile(int sock, char * filename, char * receiving_hostname, int ServerNum)
{
	PacketMsg packet;
	Ack ack; ack.TypeOfPacket = FrameACK;
	long BytesCount = 0;
	int EffectiveSentPackets = 0, SentPackets = 0;
	int ReceivedBytes = 0, WrittenBytes = 0, TotalWrittenBytes = 0;
	int seq_no = ServerNum % 2;

	if (TRACE) { fout << "Receiver: started on host " << receiving_hostname << endl; }

		
	FILE * stream = fopen(filename, "w+b");
	
	if (stream != NULL)
	{
		while (1) 
		{ 
			
			while( RcvFrame(sock, &packet) != IncomingPacket ) {;}

			ReceivedBytes += sizeof(packet); 

			if (packet.TypeOfPacket == HANDSHAKE) 
			{
				cout << "Receiver: received handshake from client" << handshake.ClientNum << " server no " << handshake.ServerNum << endl;
				if (TRACE) { fout << "Receiver: received handshake from client" << handshake.ClientNum << " server no " << handshake.ServerNum << endl; }
			}
			else if (packet.TypeOfPacket == FRAME)
			{
				cout << "Receiver: received packet number " << (int)packet.seqalternate << " and data is : "<<ReceivedBytes<<endl;
				if (TRACE) { fout << "Receiver: received packet number " << (int)packet.seqalternate << endl; }
				 
				if ( (int)packet.seqalternate != seq_no )
				{
					ack.num = (int)packet.seqalternate;
					if ( SendFrameACK(sock, &ack) != sizeof(ack) )
						return false;
					cout << "Receiver: sent ACK number" << ack.num << " again" << endl;
					if (TRACE) { fout << "Receiver: sent ACK number " << ack.num << " again" << endl; }
					EffectiveSentPackets++;
				}
				else 
				{
					
					ack.num = (int)packet.seqalternate;
					if ( SendFrameACK(sock, &ack) != sizeof(ack) )
						return false;
					cout << "Receiver: sent ack number " << ack.num << endl;
					if (TRACE) { fout << "Receiver: sent ack number" << ack.num << endl; }
					EffectiveSentPackets++;
					SentPackets++; 

					
					BytesCount = packet.bufferLeng;
					WrittenBytes = fwrite(packet.buffer, sizeof(char), BytesCount, stream );
					TotalWrittenBytes += WrittenBytes;
					
					
					seq_no = (seq_no == 0 ? 1 : 0);
					
					
					if (packet.header == LastData)
						break;
				}
			}
		}
		
		fclose( stream );

		cout << "Receiver: file transfer has completed" << endl;
		cout << "Receiver: number of effective packets sent : " << EffectiveSentPackets <<"Packets"<< endl;
		cout << "Receiver: number of packets sent  " << SentPackets <<"Packets"<< endl;
		cout << "Receiver: number of bytes received: " << ReceivedBytes <<"bytes"<< endl;
		cout << "Receiver: number of bytes written: " << TotalWrittenBytes <<"bytes"<< endl << endl;

		if (TRACE) 
		{ 
			fout << "Receiver: file transfer has completed" << endl;
			fout << "Receiver: number of effective packets sent : " << EffectiveSentPackets <<"Packets"<< endl;
			fout << "Receiver: number of packets sent   " << SentPackets <<"Packets"<< endl;
			fout << "Receiver: number of bytes received: " << ReceivedBytes<<"bytes" << endl;
			fout << "Receiver: number of bytes written: " << TotalWrittenBytes <<"bytes"<< endl << endl;
		}

		return true;
	}
	else
	{
		cout << "Receiver: not able to open the file." << endl;
        if (TRACE) { fout << "Receiver:not able to open the file." << endl; }
		return false;
	}
}


Answer Server::RespReceived(int sock, ThreeWayHS * handshakePTR)
{
	fd_set readfds;			
	FD_ZERO(&readfds);		
	FD_SET(sock, &readfds);	
	int iBytesRecv;
	int outfds = select(1 , &readfds, NULL, NULL, &timeout);

	switch (outfds)
	{
		case 0:
			return TIMEOUT;
			break;
		case 1:
			iBytesRecv = recvfrom(sock, (char *)handshakePTR, sizeof(*handshakePTR),0, (struct sockaddr*)&sa_in, &sa_in_size);
			return IncomingPacket;
			break;
		default:
			return ErrorRecieved;
			break;
	}
}

Answer Server::RcvFrame(int sock, PacketMsg * messagePTR)
{
	fd_set readfds;			
	FD_ZERO(&readfds);		
	FD_SET(sock, &readfds);	
	int iBytesRecv;
	int outfds = select(1 , &readfds, NULL, NULL, &timeout);
	switch (outfds)
	{
		case 0:
			return TIMEOUT;
			break;
		case 1:
			iBytesRecv = recvfrom(sock, (char *)messagePTR, sizeof(*messagePTR),0, (struct sockaddr*)&sa_in, &sa_in_size);
			return IncomingPacket;
			break;
		default:
			return ErrorRecieved;
			break;
	}
}

Answer Server::RcvFileAck(int sock, Ack * ack)
{
	fd_set readfds;
	FD_ZERO(&readfds);
	FD_SET(sock, &readfds); 
	int iBytesRecv;
	int outfds = select(1 , &readfds, NULL, NULL, &timeout);

	switch (outfds)
	{
		case 0:
			return TIMEOUT;
			break;
		case 1:
			iBytesRecv = recvfrom(sock, (char *)ack, sizeof(*ack),0, (struct sockaddr*)&sa_in, &sa_in_size);
			return IncomingPacket;
			break;
		default:
			return ErrorRecieved; 
			break;
	}
}

vector<string> get_all_files_within_folder(string folder)
{
    vector<string> names;
    char search_path[200];
    sprintf_s(search_path, "%s*.", folder.c_str());
    WIN32_FIND_DATA fd; 
    HANDLE hFind = ::FindFirstFile(".\\*", &fd); 
    if(hFind != INVALID_HANDLE_VALUE) 
    { 
        do 
        { 
            // read all (real) files in current folder, delete '!' read other 2 default folder . and ..
            if(! (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ) 
            {
                names.push_back(fd.cFileName);
            }
        }while(::FindNextFile(hFind, &fd)); 
        ::FindClose(hFind); 
    } 
	return names;
}


bool Server::listFiles(int sock, int ServerNum)
{

	char * dir = _getcwd(NULL, 0);
	string p;
	char getval[1];

	for(int i=0;dir[i] != 0;i++)
	{
		p += dir[i];
	}
	
	string c;
		
	vector<string> names_got = get_all_files_within_folder(p);

	for (vector<string>::iterator n = names_got.begin();n != names_got.end();++n)
	{
		c = c + *n + "\n";
	}
	
	char *cstr = &c[0];	
	
	Ack ack; 
	ack.num = -1;
	PacketMsg packet; 
	packet.TypeOfPacket = ListFiles;

	long SizeInBytes = 0, BytesCount = 0;
	int numTries;
	bool aMaxTries = false;
	bool BgnFirst = true, DoneLast = false;

	int sequence_number = ServerNum % 2; 
	int BytesSent = 0, BytesRead = 0, TotalBytesRead = 0;
	int SentPacketsEffective = 0, SentPackets = 0;

	packet.header = ( BgnFirst ? FirstData : Data) ;
	
	sprintf(packet.buffer, cstr);
							
	packet.bufferLeng = BytesCount;
	
	packet.seqalternate = sequence_number;
	
	SendFrame(sock, &packet);
	return true;

}

bool Server::delFile(int sock, char *filename, int ServerNum)
{
		cout<< "Requested file to be deleted is : "<< filename <<endl;
		FILE *file = fopen(filename, "rb"); 
		if (!file){
			throw exception();
		}
		else{
			cout << "The requested file exists !!!" << endl;
		}
		fclose(file);

		 if(remove(filename)!=0){
			cout << " Unable to delete the file....please try again  later !!!!" << endl;
			
		 }
		 else{
			cout << "File '" <<filename << "' successfully deleted !!!!" << endl;
				 
		 }
	return true;
}

void Server::run()
{
	if (WSAStartup(0x0202,&wsadata) != 0)
	{  
		WSACleanup();  
	    err_sys("Error in starting WSAStartup()\n");
	}
	
	
	if(gethostname(server_name, HOST_NAME_LEN)!=0)
		err_sys("Server gethostname() error.");
	
	printf("Server: started.");
	
	if ( (sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0 )
		err_sys("socket() failed");
	
	
	memset(&sa, 0, sizeof(sa));				
	sa.sin_family = AF_INET;                
	sa.sin_addr.s_addr = htonl(INADDR_ANY); 
	sa.sin_port = htons(SERVER_PORT);       
	sa_in_size = sizeof(sa_in);	

	
	if (bind(sock, (LPSOCKADDR)&sa, sizeof(sa) ) < 0)
		err_sys("Socket binding error");
	
	while ( RespReceived(sock, &handshake) != IncomingPacket || handshake.type != ClientReq ) {;}

	cout << "Server: received handshake from client" << handshake.ClientNum << endl;
	if (TRACE) { fout << "Server: received handshake from client" << handshake.ClientNum << endl; }//log file

	if ( handshake.choice == Get )
	{
		cout << "Server: host: " << handshake.hostname << " requests get of file: \"" << handshake.filename << endl;
		if (TRACE) { fout << "Server: host: " << handshake.hostname << " requests get of file: " << handshake.filename << endl; }
		
		if ( FileExists(handshake.filename) )
			handshake.type = AckNumClient; 
		else
		{
			handshake.type = FileNotFound;
			cout << "Server: file not found." << endl;	
			if (TRACE) { fout << "Server: filenot found." << endl; }
		}
	}
	else if ( handshake.choice == Put )
	{
		cout << "Server: host " << handshake.hostname << " requests put of file: " << handshake.filename << endl;
		if (TRACE) { fout << "Server:  on host " << handshake.hostname << " requests put of file: " << handshake.filename << endl; }
		handshake.type = AckNumClient; 	
	}
	else if( handshake.choice == List)
	{
		cout << "Client requested for list of files" << endl;
		handshake.type = AckNumClient; 
		//listFiles(sock, handshake.ServerNum);
	}
	else if( handshake.choice == Delete)
	{
		cout << "Client requested for the deletion of a file " <<handshake.filename<< endl;
		handshake.type = AckNumClient; 
	}
	else
	{
		handshake.type = Invalid;
		cout << "Server: Invalid request ." << endl;	
		if (TRACE) { fout << "Server: Invalid request." << endl; }
	}
	
	if (handshake.type != AckNumClient) 
	{
		if ( SendingReq(sock, &handshake, &sa_in) != sizeof(handshake) )
			err_sys("Error in sending packet.");

		cout << "Server: sent error message to client. "<< endl;
		if (TRACE) { fout << "Server: sent error message to client. "<< endl; }
	}
	else if (handshake.type == AckNumClient)
	{
		srand((unsigned)time(NULL));
		random = rand() % MAX_RANDOM; 
		handshake.ServerNum = random;

		
		do {
			if ( SendingReq(sock, &handshake, &sa_in) != sizeof(handshake) )
				err_sys("Error in sending packet.");

			cout << "Server: sent handshake CLEINT" << handshake.ClientNum << " Server" << handshake.ServerNum << endl;
			if (TRACE) { fout << "Server: sent handshake CLEINT" << handshake.ClientNum << " Server" << handshake.ServerNum << endl; }

		}
		while( RespReceived(sock, &handshake) != IncomingPacket || handshake.type != AckNumServer );

		cout << "Server: received handshake CLEINT" << handshake.ClientNum << " Server" << handshake.ServerNum << endl;
		if (TRACE) { fout << "Server: received handshake CLEINT" << handshake.ClientNum << " Server" << handshake.ServerNum << endl; }

		if (handshake.type == AckNumServer)
		{
			switch (handshake.choice)
			{
				case Get:
					if ( ! SendFile(sock, handshake.filename, server_name, handshake.ClientNum) )
						err_sys("An error occurred while sending the file.");	
					break;

				case Put:
					if ( ! RcvFile(sock, handshake.filename, server_name, handshake.ServerNum) )
						err_sys("An error occurred while receiving the file.");	
					break;
				case List:
				
					listFiles(sock, handshake.ServerNum);
					break;
				case Delete:
					delFile(sock, handshake.filename,handshake.ServerNum);
						break;
				default:
					break;
			}
		}
		else
		{
			cout << "Error in Handshaking" << endl;
			if (TRACE) { fout << "Error in Handshaking" << endl; }
		}
	}

	cout << "Closing server socket." << endl;
	if (TRACE) { fout << "Closing server socket." << endl; }

	closesocket(sock);
	
	getchar();
}

void Server::err_sys(char * fmt,...)
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

unsigned long Server::ResolveName(char name[])
{
	struct hostent *host; 

	if ((host = gethostbyname(name)) == NULL)
		err_sys("gethostbyname() failed");

	return *((unsigned long *) host->h_addr_list[0]); 
}

Server::Server(char * fn) 
{
	
	timeout.tv_sec = STIMER;
	timeout.tv_usec = UTIMER;
	
	
	fout.open(fn);
} 


Server::~Server() 
{
	
	fout.close();

	WSACleanup();
}

int main(void)
{
	Server * ser = new Server();
	ser->run();
	return 0;
}
