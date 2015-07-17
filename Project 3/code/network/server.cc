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
std::vector<ServerMV> ServerMVs;

void send(char *type, bool status, int ID, int machineID, int mailBoxID){
	serverPacket packet_Send;
	PacketHeader packet_From_Server;
	MailHeader mail_From_Server;
	
	packet_Send.value = ID;
	packet_Send.status = status;
	
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
	for (int i = 0; i < (signed)ServerLocks.size(); i++){
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
	ServerLock temp;
	temp.name = new char[sizeof(name)+1];
	strcpy(temp.name, name);
	temp.count = 0;
	temp.valid = true;
	temp.waitingQueue = new List;
	temp.available = true;
	temp.IsDeleted = false;
	ServerLocks[lockID] = temp;
	
	send("CREATELOCK", true, lockID, machineID, mailBoxID);
}

//----------------------------------------------------------------------
//  Acquire
//  Acquires a lock
//----------------------------------------------------------------------

void acquire(int index, int machineID, int mailBoxID){
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
	
	ServerLocks[index].count++;
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
			printf("CLIENT (%d, %d) IS NOW THE LOCK OWNER\n", ServerLocks[index].Owner.machineID, ServerLocks[index].Owner.mailBoxID);
			send("RELEASE", true, index, ServerLocks[index].Owner.machineID, ServerLocks[index].Owner.mailBoxID);
		}
	} else {
		printf("YOU AREN'T THE LOCK OWNER\n");
		send("RELEASE", false, -2, machineID, mailBoxID);
	}
	
	if(ServerLocks[index].IsDeleted && ServerLocks[index].count == 0){
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
	for (int i = 0; i < (signed)ServerCVs.size(); i++){
		if (ServerCVs[i].name == name){
			send("CREATECV", true, i, machineID, mailBoxID);
			return;
		}
	}
	
	cvID = (signed)ServerCVs.size();
	if (cvID == MAX_CV || cvID < 0){
		send("CREATECV", false, cvID, machineID, mailBoxID);
		return;
	}
	
	ServerCV temp;
	temp.name = new char[sizeof(name)+1];
	strcpy(temp.name, name);
	temp.count = 0;
	temp.cvID = cvID;
	temp.lockID = -1;
	temp.valid = true;
	temp.waitingQueue = new List;
	temp.IsDeleted = false;
	ServerCVs[cvID] = temp;
	
	send("CREATELOCK", true, cvID, machineID, mailBoxID);
}

//----------------------------------------------------------------------
//  destroyCV
//  destroy CVs
//----------------------------------------------------------------------
void destroyCV(int index, int machineID, int mailBoxID){
	if(index < 0 || index > MAX_CV){
		printf("CV TO BE DESTROYED IS INVALID\n");
		send("DESTROYCV", false, -1, machineID, mailBoxID);
		return;
	} else if (!ServerCVs[index].valid){
		printf("CV TO BE DESTROYED IS NULL\n");
		send("DESTROYCV", false, -2, machineID, mailBoxID);
		return;
	}else if (!ServerCVs[index].waitingQueue->IsEmpty() || ServerCVs[index].count > 0){
		printf("CV HAS WAITING CLIENTS\n");
		ServerCVs[index].IsDeleted = true;
		send("DESTROYCV", false, 1, machineID, mailBoxID);
	} else if (ServerCVs[index].count == 0){
		printf("DESTROYING CV!\n");
		ServerCVs[index].valid = false;
		delete ServerCVs[index].name;
		delete ServerCVs[index].waitingQueue;
		send("DESTROYCV", true, index, machineID, mailBoxID);
	}
}

//----------------------------------------------------------------------
//  Signal
//  Call Signal on a CV
//----------------------------------------------------------------------
void signal(int lockID, int index, int machineID, int mailBoxID){
	client signalClient;
	
	if(lockID < 0 || index > MAX_LOCK){
		printf("LOCK TO SIGNAL IS INVALID\n");
		send("SIGNAL", false, -2, machineID, mailBoxID);
		return;
	} else if (index < 0 || index > MAX_CV){
		printf("SIGNAL INVALID INDEX\n");
		send("SIGNAL", false, -2, machineID, mailBoxID);
		return;
	} else if (!ServerLocks[lockID].valid || !ServerCVs[index].valid){
		printf("LOCK WITH CV IS INVALID\n");
		send("SIGNAL", false, -2, machineID, mailBoxID);
		return;
	} else if (ServerLocks[lockID].Owner.machineID != machineID || ServerLocks[lockID].Owner.mailBoxID != mailBoxID){
		printf(" CLIENT DOESN'T HAVE THE LOCK\n");
		send("SIGNAL", false, -2, machineID, mailBoxID);
		return;
	} else if (ServerCVs[index].lockID != lockID){
		printf("LOCK DOESN'T MATCH FOR SIGNAL\n");
		send("SIGNAL", false, -2, machineID, mailBoxID);
		return;
	} else if (ServerCVs[index].waitingQueue->IsEmpty()){
		printf("QUEUE IS EMPTY\n");
		send("SIGNAL", false, -2, machineID, mailBoxID);
	}
	
	signalClient = *((client *)ServerCVs[index].waitingQueue->Remove());
	if (ServerCVs[index].waitingQueue->IsEmpty()){
		ServerCVs[index].lockID = -1;
	}
	
	ServerCVs[index].count--;
	acquire(index, signalClient.machineID, signalClient.mailBoxID);
	send("SIGNAL", true, index, machineID, mailBoxID);
}

//----------------------------------------------------------------------
//  Wait
//  Call Wait on a CV
//----------------------------------------------------------------------
void wait(int lockID, int index, int machineID, int mailBoxID){
	client *waitingClient = NULL;
		
	if(lockID < 0 || lockID > MAX_LOCK){
		printf("LOCK TO WAIT ON IS INVALID\n");
		send("WAIT", false, -1, machineID, mailBoxID);
		return;
	} else if(index < 0 || index > MAX_CV){
		printf("CV TO WAIT ON IS INVALID\n");
		send("WAIT", false, -1, machineID, mailBoxID);
		return;
	}  else if (!ServerLocks[lockID].valid || !ServerCVs[index].valid){
		printf("CV AND LOCK ARE INVALID\n");
		send ("WAITCV", false, -2, machineID, mailBoxID);
		return;
	}
	
	if (ServerCVs[index].lockID == -1){
		ServerCVs[index].lockID = lockID;
	}
	
	release(lockID, machineID, mailBoxID, 1);
	ServerCVs[index].count++;
	waitingClient = new client;
	waitingClient -> machineID = machineID;
	waitingClient -> mailBoxID = mailBoxID;
	ServerCVs[index].waitingQueue->Append((void *)waitingClient);
}

//----------------------------------------------------------------------
//  Broadcast
//  Call Broadcast on a CV
//----------------------------------------------------------------------
void broadcast(int lockID, int index, int machineID, int mailBoxID){
	client signalClient;
	
	if(lockID < 0 || lockID > MAX_LOCK){
		printf("LOCK TO WAIT ON IS INVALID\n");
		send("WAIT", false, -1, machineID, mailBoxID);
		return;
	} else if(index < 0 || index > MAX_CV){
		printf("CV TO WAIT ON IS INVALID\n");
		send("WAIT", false, -1, machineID, mailBoxID);
		return;
	} else if (!ServerLocks[lockID].valid || !ServerCVs[index].valid){
		printf("CV AND LOCK ARE INVALID\n");
		send ("WAITCV", false, -2, machineID, mailBoxID);
		return;
	}
	
	while(!ServerCVs[index].waitingQueue->IsEmpty()){
		signal(lockID, index, machineID, mailBoxID);
	}
	
	send("BROADCAST", true, index, machineID, mailBoxID);
}

//----------------------------------------------------------------------
//  Create MV
//  Create a new monitor variable
//----------------------------------------------------------------------
void createMV(char *name, int value, int machineID, int mailBoxID){
		for(int i = 0; i < (signed)ServerMVs.size(); i++){
			if (ServerMVs[i].name = name){
				send("CREATEMV", true, i, machineID, mailBoxID);
			}
		}
		
		int mvID = ServerMVs.size();
		if(mvID >= MAX_MV || mvID < 0){
			printf("TOO MANY MVs\n");
			send("CREATEMV", false, -1, machineID, mailBoxID);
			return;
		} else {
			ServerMV temp;
			temp.name = new char[sizeof(name)+1];
			strcpy(temp.name, name);
			temp.count = 0;
			temp.mvID = mvID;
			temp.valid = true;
			temp.value = value;
			ServerMVs[mvID] = temp;
			send("CREATEMV", true, mvID, machineID, mailBoxID);
		}
}

void getMV(int index, int machineID, int mailBoxID){
	
}

void destroyMV(int index, int machineID, int mailBoxID){
	
}

//----------------------------------------------------------------------
//  Set MV
//  Set a new monitor variable
//----------------------------------------------------------------------
void setMV(int index, int value, int machineID, int mailBoxID){
	serverPacket packet;

	if(index >= MAX_MV || index < 0){
		printf("TOO MANY MVs\n");
		send("SETMV", false, -1, machineID, mailBoxID);
		return;
	} else if (!ServerMVs[index].valid){
		printf("MV ISN'T VALID\n");
		send("SETMV", false, -2, machineID, mailBoxID);
		return;
	}
	
	ServerMVs[index].value = value;
	send("SETMV", true, ServerMVs[index].value, machineID, mailBoxID);
}

//----------------------------------------------------------------------
//  RunServer
//  Runs the server using a switch statement to handle incoming messages
//----------------------------------------------------------------------
void RunServer(){
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
				printf("REQUEST: WAIT FROM CLIENT\n");
				wait(packet.index, packet.index2, packet_From_Client.from, mail_From_Client.from);
				break;
			case SC_Signal:
				printf("REQUEST: SIGNAL FROM CLIENT\n");
				signal(packet.index, packet.index2, packet_From_Client.from, mail_From_Client.from);
				break;
			case SC_Broadcast:
				printf("REQUEST: BROADCAST FROM CLIENT\n");
				broadcast(packet.index, packet.index2, packet_From_Client.from, mail_From_Client.from);
				break;
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
				printf("REQUEST: DESTROY CV FROM CLIENT\n");
				destroyCV(packet.index, packet_From_Client.from, mail_From_Client.from);
				break;
			case SC_CreateMV:
				printf("REQUEST: CREATE MV FROM CLIENT\n");
				createMV(packet.name, packet.value, packet_From_Client.from, mail_From_Client.from);
				break;
			case SC_SetMV:
				printf("REQUEST: SET MV FROM CLIENT\n");
				setMV(packet.index, packet.value, packet_From_Client.from, mail_From_Client.from);
				break;
			case SC_GetMV:
				printf("REQUEST: GET MV FROM CLIENT\n");
				getMV(packet.index, packet_From_Client.from, mail_From_Client.from);
				break;
			case SC_DestroyMV:
				printf("REQUEST: DELETE MV FROM CLIENT\n");
				destroyMV(packet.index, packet_From_Client.from, mail_From_Client.from);
				break;
			default:
				printf("SERVER: INVALID SYSCALL %d\n", packet.syscall);
		}
	}	
}
