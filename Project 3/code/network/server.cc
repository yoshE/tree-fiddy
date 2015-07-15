#include "system.h"
#include "network.h"
#include "post.h"
#include "interrupt.h"
#include "thread.h"
#include "list.h"
#include <sstream>
#include <string.h>
#include <stdio.h>
#include <vector>

#define SC_Acquire  	 11
#define SC_Release  	 12
#define SC_Wait     	 13
#define SC_Signal   	 14
#define SC_Broadcast	 15
#define SC_CreateLock	 16
#define SC_DestroyLock   17
#define SC_CreateCV 	 18
#define SC_DestroyCV 	 19
#define SC_CreateMV		 22
#define SC_SetMV		 23
#define SC_GetMV		 24
#define SC_DestroyMV	 25

std::vector<ServerLock> ServerLocks;
std::vector<ServerCV> ServerCVs;
std::vector<<ServerMV> ServerMVs;

int createLock(){
	
}

void Run(){
	PacketHeader packet_From_Client;
	MailHeader mail_From_Client;
	clientPacket packet;
	int len = sizeof(packet);
	char *data = new char[len + 1];
	data[len] = '\0';
	
	while(true){
		printf("SERVER: WAITING FOR CLIENT REQUEST\n");
		
		postOffice->Receive(0,&packet_From_Client, &mail_From_Client, data);
		memcpy((void *)&packet, (void *)data, len);
		
		switch(packet.syscall){
			case SC_Acquire:
				printf("REQUEST: ACQUIRE LOCK FROM CLIENT\n");
				createLock(packet.name, packet_From_Client.from, mail_From_Client.from);
				break;
			case SC_Release:
			case SC_Wait:
			case SC_Signal:
			case SC_Broadcast:
			case SC_CreateLock:
			case SC_DestroyLock:
			case SC_CreateCV:
			case SC_DestroyCV:
			case SC_CreateMV:
			case sc_SetMV:
			case SC_GetMV:
			case SC_DestroyMV:
			case default:
				printf("SERVER: INVALID SYSCALL %d\n", packet.syscall);
		}
	}	
}
