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

void send(char *type, bool statue, int ID, int machineID, int mailBoxID){
	serverPacket packet_Send;
	PacketHeader packet_From_Server;
	MailHeader mail_From_Server;
	
	packet_Send.value = ID;
	packet_Send.statue = statue;
	
	int len = sizeof(packet_Send);
	char *data = new char[len + 1];
	data[len] = '\0';
	memcpy((void *)data, (void *)&packet_Send, len);
	
	packet_From_Server.to = machineID;
	mail_From_Server.to = mailBoxID;
	mail_From_Server.from = 0;
	mail_From_Server.length = len + 1;
	
	if(!postOffice->Send(packet_From_Server, mail_From_Server, data)){
		printf("SEND FAILED\n");
		interrupt->Halt();
	}
}

//----------------------------------------------------------------------
//  CreateLock
//  Creates a new lock and adds it to the serverLock vector
//----------------------------------------------------------------------
void createLock(char *name, int machineID, int mailBoxID){
	int lockID = -1;
	for (int i = 0; i < ServerLocks.size(); i++){
		if (ServerLocks[i].name == name){
			send("CREATELOCK", true, i, machineID, mailBoxID);
			return;
		}
	}
	
	lockID = (signed)ServerLocks.size();
	if (lockID == MAX_LOCK || lockID == -1){
		send("CREATELOCK", false, lockID, machineID, mailBoxID);
		return;
	}
	
	lockID--;
	ServerLocks[lockID].name = new char[sizeof(name)+1];
	strcpy(ServerLocks[lockID].name, name);
	ServerLocks[lockID].count = 0;
	ServerLocks[lockID].valid = true;
	ServerLocks[lockID].waitingQueue = new List;
	ServerLocks[lockID].available = true;
	ServerLocks[lockID].Owner = NULL;
	ServerLocks[lockID].IsDeleted = false;
	
	send("CREATELOCK", true, lockID, machineID, mailBoxID);
}

//----------------------------------------------------------------------
//  Acquire
//  Acquires a lock
//----------------------------------------------------------------------

void acquire(int index, int machineID, int mailBoxID){
	serverPacket packet;
	client *waitingClient = NULL;
	
	if(index < 0 || index > MAX_LOCK){
		printf("LOCK TO BE ACQUIRED IS INVALID\n");
		send("ACQUIRE", false, -1, machineID, mailBoxID);
		return;
	} else if (!ServerLocks[index].valid){
		printf("LOCK TO BE ACQUIRED IS NULL\n");
		send("ACQUIRE", false, -2, machineID, mailBoxID);
		return;
	} else if (ServerLocks[index].Owner.machineID == machineID && ServerLocks[index].Owner.mailBoxID == mailBoxID){
		printf("YOU ALREADY OWN THIS LOCK!\n");
		send("ACQUIRE", false, -3, machineID, mailBoxID);
	}
	
	Server_Lock[index].count++;
	if(ServerLocks[index].available){
		ServerLocks[index].available = false;
		ServerLocks[index].Owner.machineID = machineID;
		ServerLocks[index].Owner.mailBoxID = mailBoxID;
		printf("LOCK ACQUIRED: CLIENT (%d, %d) IS NOW THE OWNER\n", machineID, mailBoxID);
		send("ACQUIRE", true, index, machineID, mailBoxID);
	} else {
		waitingClient = new client;
		waitingClient->machineID = machineID;
		waitingClient->mailBoxID = mailBoxID;
		ServerLocks[index].waitingQueue->Append((void*)waitingClient);
		printf("LOCK ISN'T FREE, CLIENT (%d, %d) APPEND TO WAIT QUEUE\n", machineID, mailBoxID);
	}	
}

//----------------------------------------------------------------------
//  Release
//  Release a lock
//----------------------------------------------------------------------
void release(int index, int machineID, int mailBoxID, int syscall){
	serverPacket packet;
	
	if(index < 0 || index > MAX_LOCK){
		printf("LOCK TO BE RELEASED IS INVALID\n");
		send("RELEASE", false, -1, machineID, mailBoxID);
		return;
	} else if (!ServerLocks[index].valid){
		printf("LOCK TO BE RELEASED IS NULL\n");
		send("RELEASE", false, -2, machineID, mailBoxID);
		return;
	} else if (ServerLocks[index].Owner.machineID == machineID && ServerLocks[index].Owner.mailBoxID == mailBoxID){
		printf("CLIENT (%d, %d) RELEASED THE LOCK, WAITING CLIENTS: %d\n", machineID, mailBoxID, ServerLocks[index].count);
		if (syscall == SC_Release) send("RELEASE", true, index, machineID, mailBoxID);
		if (ServerLocks[index].waitingQueue->IsEmpty()){
			ServerLocks[index].available = true;
			ServerLocks[index].Owner.machineID = -1;
			ServerLocks[index].Owner.mailBoxID = -1;
		} else {
			ServerLocks[index].Owner = *((client*)ServerLocks[index].waitingQueue->Remove());
			ServerLocks[index].count--;
			printf("CLIENT (%d, %d) IS NOW THE LOCK OWNER\n", ServerLocks[index].machineID, ServerLocks[index].mailBoxID);
			send("RELEASE", true, index, ServerLocks[index].machineID, ServerLocks[index].mailBoxID);
		}
	} else {
		printf("YOU AREN'T THE LOCK OWNER\n");
		send("RELEASE", false, -2, machineID, mailBoxID);
	}
	
	if(ServerLocks[index].IsDeleted && ServerLock[index].count == 0){
		delete ServerLocks[index].name;
		delete ServerLocks[index].waitingQueue;
		ServerLocks[index].valid = false;
	}
}

//----------------------------------------------------------------------
//  DestroyLock
//  Destroy a lock
//----------------------------------------------------------------------
void destroy(int index, int machineID, int mailBoxID){
	serverPacket packet;
	
	if(index < 0 || index > MAX_LOCK){
		printf("LOCK TO BE DESTROYED IS INVALID\n");
		send("DESTROYLOCK", false, -1, machineID, mailBoxID);
		return;
	} else if (!ServerLocks[index].valid){
		printf("LOCK TO BE DESTROYED IS NULL\n");
		send("DESTROYLOCK", false, -2, machineID, mailBoxID);
		return;
	}else if (!ServerLocks[index].waitingQueue->IsEmpty() || ServerLocks[index].count > 0){
		printf("LOCK HAS WAITING CLIENTS\n");
		ServerLocks[index].IsDeleted = true;
		send("DESTROYLOCK", false, 1, machineID, mailBoxID);
	} else if (ServerLocks[index].count == 0){
		printf("DESTROYING LOCK!\n");
		ServerLocks[index].valid = false;
		delete ServerLocks[index].name;
		delete ServerLocks[index].waitingQueue;
		send("DESTROYLOCK", true, index, machineID, mailBoxID);
	}
}

//----------------------------------------------------------------------
//  CreateCV
//  create CVs
//----------------------------------------------------------------------
void createCV(char *name, int machineID, int mailBoxID){
	int cvID = -1;
	for (int i = 0; i < ServerCVs.size(); i++){
		if (ServerCVs[i].name == name){
			send("CREATECV", true, i, machineID, mailBoxID);
			return;
		}
	}
	
	cvID = (signed)ServerCVs.size();
	if (cvID == MAX_CV || cvID < 0){
		send("CREATECV", false, lockID, machineID, mailBoxID);
		return;
	}
	
	cvID--;
	ServerCVs[cvID].name = new char[sizeof(name)+1];
	strcpy(ServerCVs[cvID].name, name);
	ServerCVs[cvID].count = 0;
	ServerCVs[cvID].valid = true;
	ServerCVs[cvID].waitingQueue = new List;
	ServerCVs[cvID].available = true;
	ServerCVs[cvID].IsDeleted = false;
	
	send("CREATELOCK", true, cvID, machineID, mailBoxID);
}

//----------------------------------------------------------------------
//  Run
//  Runs the server using a switch statement to handle incoming messages
//----------------------------------------------------------------------
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
				acquire(packet.index, packet_From_Client.from, mail_From_Client.from);
				break;
			case SC_Release:
				printf("REQUEST: RELEASE LOCK FROM CLIENT\n");
				release(packet.index, packet_From_Client.from, mail_From_Client.from, SC_Release);
				break;
			case SC_Wait:
			case SC_Signal:
			case SC_Broadcast:
			case SC_CreateLock:
				printf("REQUEST: CREATE LOCK FROM CLIENT\n");
				createLock(packet.name,packet_From_Client.from, mail_From_Client.from);
				break;
			case SC_DestroyLock:
				printf("REQUEST: DESTROY LOCK FROM CLIENT\n");
				createLock(packet.name,packet_From_Client.from, mail_From_Client.from);
				break;
			case SC_CreateCV:
				printf("REQUEST: CREATE CV FROM CLIENT\n");
				createCV(packet.name, packet_From_Client.from, mail_From_Client.from);
				break;
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
