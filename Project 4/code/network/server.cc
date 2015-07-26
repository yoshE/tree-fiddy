#include "system.h"
#include "network.h"
#include "post.h"
#include "interrupt.h"
#include "thread.h"
#include "list.h"
#include "bitmap.h"
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

std::vector<ServerLock> ServerLocks;		// vector of server locks
std::vector<ServerCV> ServerCVs;		// vector of server cvs
std::vector<ServerMV> ServerMVs;		// vector of server mvs

struct pendingRequests{		// Struct for dealing with server requests
	int doYouHave[5];
	int doYouHaveReply[5];
	int syscall;
	int count;
	int value;
	int clientCount;
	int noOfReplies;
	int flag;
	
	char name[10];
	
	bool received;
	 
	List *requestWaitQueue;
	List *serverQueue;	
	List *clientQueue;
};

struct pendingRequests requests[6000];
BitMap *requestMap;

void send(char *type, bool status, int ID, int machineID, int mailBoxID){		// Send a file from the server to the client
	serverPacket packet_Send;		// packet that will be sent to the client
	PacketHeader packet_From_Server;
	MailHeader mail_From_Server;
	
	packet_Send.value = ID;		// set the ID of the packet to the value that we want to return to the client
	packet_Send.status = status;		// Set the status equal to the success of the syscall action
	
	int len = sizeof(packet_Send);		// find size of packet to send
	char *data = new char[len];		// put it into data
	data[len] = '\0';
	memcpy((void *)data, (void *)&packet_Send, len);
	
	packet_From_Server.to = machineID;	
	mail_From_Server.to = mailBoxID;
	mail_From_Server.from = 0;
	mail_From_Server.length = len;
	
	if(!postOffice->Send(packet_From_Server, mail_From_Server, data)){		// Send the packet to the client
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
		if (!strcmp(ServerLocks[i].name, name)){	// If lock already exists in the server then return that ID
			send("CREATELOCK", true, i, machineID, mailBoxID);
			return;
		} 
	}
	
	lockID = (signed)ServerLocks.size();		// new ID is equal to the size of vector
	if (lockID == MAX_LOCK || lockID == -1){		// If vector is "full" (over 200) then fail
		send("CREATELOCK", false, lockID, machineID, mailBoxID);
		return;
	}
	
	printf("Name is: %s\n", name);
	ServerLock temp;		// new struct ServerLock that will be added to ServerLocks[]
	temp.name = new char[sizeof(name)];
	strcpy(temp.name, name);		// copy in the name
	temp.count = 0;
	temp.valid = true;		// Lock is valid
	temp.waitingQueue = new List;
	temp.available = true;
	temp.IsDeleted = false;
	ServerLocks.push_back(temp);		// add struct into vector
	
	send("CREATELOCK", true, lockID, machineID, mailBoxID);		// send msg back to client that action was successful
}

//----------------------------------------------------------------------
//  Acquire
//  Acquires a lock
//----------------------------------------------------------------------

void acquire(int index, int machineID, int mailBoxID){
	client *waitingClient = NULL;
	
	if(index < 0 || index > (myMachineID+1)*MAX_LOCK){		// if lock to be acquired has an incorrect index (under 0 or over 200)
		printf("LOCK TO BE ACQUIRED IS INVALID\n");
		send("ACQUIRE", false, -1, machineID, mailBoxID);
		return;
	} else if (!ServerLocks[index].valid){		// If lock is no longer valid (it was deleted)
		printf("LOCK TO BE ACQUIRED IS NULL\n");
		send("ACQUIRE", false, -2, machineID, mailBoxID);
		return;
	} else if (ServerLocks[index].Owner.machineID == machineID && ServerLocks[index].Owner.mailBoxID == mailBoxID){		// If client already owns lock
		printf("YOU ALREADY OWN THIS LOCK!\n");
		send("ACQUIRE", false, -3, machineID, mailBoxID);
	}
	
	ServerLocks[index].count++;		// increase lock usage count
	if(ServerLocks[index].available){		// if Lock is available
		ServerLocks[index].available = false;		// it is no longer available
		ServerLocks[index].Owner.machineID = machineID;		// set owner of lock to requesting client
		ServerLocks[index].Owner.mailBoxID = mailBoxID;
		printf("LOCK ACQUIRED: CLIENT (%d, %d) IS NOW THE OWNER\n", machineID, mailBoxID);
		send("ACQUIRE", true, index, machineID, mailBoxID);		// send msg back to client with success
	} else {
		waitingClient = new client;		// Create new waiting client and add to waiting queue for said lock
		waitingClient->machineID = machineID;
		waitingClient->mailBoxID = mailBoxID;
		ServerLocks[index].waitingQueue->Append((void*)waitingClient);		// add to queue
		printf("LOCK ISN'T FREE, CLIENT (%d, %d) APPEND TO WAIT QUEUE\n", machineID, mailBoxID);
	}	
}

//----------------------------------------------------------------------
//  Release
//  Release a lock
//----------------------------------------------------------------------
void release(int index, int machineID, int mailBoxID, int syscall){
	if(index < 0 || index > (myMachineID+1)*MAX_LOCK){
		printf("LOCK TO BE RELEASED IS INVALID\n");
		send("RELEASE", false, -1, machineID, mailBoxID);
		return;
	} else if (!ServerLocks[index].valid){
		printf("LOCK TO BE RELEASED IS NULL\n");
		send("RELEASE", false, -2, machineID, mailBoxID);
		return;
	} else if (ServerLocks[index].Owner.machineID == machineID && ServerLocks[index].Owner.mailBoxID == mailBoxID){		// If client owns this lock
		printf("CLIENT (%d, %d) RELEASED THE LOCK, WAITING CLIENTS: %d\n", machineID, mailBoxID, ServerLocks[index].count);
		if (syscall == SC_Release) send("RELEASE", true, index, machineID, mailBoxID);	// If request was for release and not signal, send msg back to client
		if (ServerLocks[index].waitingQueue->IsEmpty()){	// If waiting queue is empty
			ServerLocks[index].available = true;		// lock is now available
			ServerLocks[index].Owner.machineID = -1;	// lock as no owner
			ServerLocks[index].Owner.mailBoxID = -1;
		} else {
			ServerLocks[index].Owner = *((client*)ServerLocks[index].waitingQueue->Remove());		// remove a client from waiting queue
			ServerLocks[index].count--;		// remove count of waiting clients
			printf("CLIENT (%d, %d) IS NOW THE LOCK OWNER\n", ServerLocks[index].Owner.machineID, ServerLocks[index].Owner.mailBoxID);
			send("RELEASE", true, index, ServerLocks[index].Owner.machineID, ServerLocks[index].Owner.mailBoxID);		// send success msg
		}
	} else {		// If client doesn't own lock
		printf("YOU AREN'T THE LOCK OWNER\n");
		send("RELEASE", false, -2, machineID, mailBoxID);
	}
	
	if(ServerLocks[index].IsDeleted && ServerLocks[index].count == 0){		// If lock is marked for deletion and there are no waiting clients
		delete ServerLocks[index].name;		// delete EVERYTHING
		delete ServerLocks[index].waitingQueue;
		ServerLocks[index].valid = false;
	}
}

//----------------------------------------------------------------------
//  DestroyLock
//  Destroy a lock
//----------------------------------------------------------------------
void destroy(int index, int machineID, int mailBoxID){
	if(index < 0 || index > (myMachineID+1)*MAX_LOCK){
		printf("LOCK TO BE DESTROYED IS INVALID\n");
		send("DESTROYLOCK", false, -1, machineID, mailBoxID);
		return;
	} else if (!ServerLocks[index].valid){
		printf("LOCK TO BE DESTROYED IS NULL\n");
		send("DESTROYLOCK", false, -2, machineID, mailBoxID);
		return;
	}else if (!ServerLocks[index].waitingQueue->IsEmpty() || ServerLocks[index].count > 0){		// If lock has waiting clients
		printf("LOCK HAS WAITING CLIENTS\n");
		ServerLocks[index].IsDeleted = true;		// mark the lock for later deletion
		send("DESTROYLOCK", false, 1, machineID, mailBoxID);
	} else if (ServerLocks[index].count == 0){		// If lock has no waiting clients
		printf("DESTROYING LOCK!\n");
		ServerLocks[index].valid = false;		// DELETE EVERYTHING
		delete ServerLocks[index].name;
		delete ServerLocks[index].waitingQueue;
		send("DESTROYLOCK", true, index, machineID, mailBoxID);		// send success msg
	}
}

//----------------------------------------------------------------------
//  CreateCV
//  create CVs
//----------------------------------------------------------------------
void createCV(char *name, int machineID, int mailBoxID){
	int cvID = -1;
	for (int i = 0; i < (signed)ServerCVs.size(); i++){	// Check if CV already exists
		if (strcmp(ServerCVs[i].name, name) == 0){
			send("CREATECV", true, i, machineID, mailBoxID);
			return;
		}
	}
	
	// OTHERWISE SAME AS CREATELOCK
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
	temp.lockID = -1;		// starts with no lock
	temp.valid = true;
	temp.waitingQueue = new List;
	temp.IsDeleted = false;
	ServerCVs.push_back(temp);
	
	send("CREATELOCK", true, cvID, machineID, mailBoxID);
}

//----------------------------------------------------------------------
//  destroyCV
//  destroy CVs (same as destroyLock)
//----------------------------------------------------------------------
void destroyCV(int index, int machineID, int mailBoxID){
	if(index < 0 || index > (myMachineID+1)*MAX_CV){
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
	clientPacket packetToSend;
	client signalClient;
	PacketHeader pFromSToS;
	MailHeader mFromSToS;
	
	int len = sizeof(packetToSend);
	char *data=new char[len+1];
	data[len]='\0';
	
	if (index < 0 || index > (myMachineID+1)*MAX_CV){		// Is CV index invalid?
		printf("SIGNAL INVALID INDEX\n");
		send("SIGNAL", false, -2, machineID, mailBoxID);
		return;
	} else if (!ServerCVs[index].valid){		// Are both lock and CV valid?
		printf("LOCK WITH CV IS INVALID\n");
		send("SIGNAL", false, -2, machineID, mailBoxID);
		return;
	} else if (SERVERS == 1 && ServerLocks[lockID].Owner.machineID != machineID || ServerLocks[lockID].Owner.mailBoxID != mailBoxID){		// Does client own lock?
		printf(" CLIENT DOESN'T HAVE THE LOCK\n");
		send("SIGNAL", false, -2, machineID, mailBoxID);
		return;
	} else if (ServerCVs[index].lockID != lockID){		// Does lock match CV lock?
		printf("LOCK DOESN'T MATCH FOR SIGNAL\n");
		send("SIGNAL", false, -2, machineID, mailBoxID);
		return;
	} else if (ServerCVs[index].waitingQueue->IsEmpty()){		// Are there clients waiting on the CV?
		printf("QUEUE IS EMPTY\n");
		send("SIGNAL", false, -2, machineID, mailBoxID);
	}
	
	signalClient = *((client *)ServerCVs[index].waitingQueue->Remove());		// Remove a client from the waiting queue
	if (ServerCVs[index].waitingQueue->IsEmpty()){		// If queue is empty...
		ServerCVs[index].lockID = -1;		// Reset Lock associated with CV
	}
	
	ServerCVs[index].count--;		// Decrement usage count of CV
	
	if(SERVERS == 1){
		acquire(lockID, signalClient.machineID, signalClient.mailBoxID);
	}
	else{
		packetToSend.index = lockID;
		packetToSend.syscall = SC_Acquire;
		packetToSend.ServerArg = 0;
		
		pFromSToS.from = signalClient.machineID;
		mFromSToS.from = signalClient.mailBoxID;
		pFromSToS.to = lockID/200;
		mFromSToS.to = 0;
		mFromSToS.length = sizeof(packetToSend);
		
		memcpy((void *)data,(void *)&packetToSend,len);
		postOffice->Send(pFromSToS,mFromSToS,data);
	}
	
	send("SIGNAL", true, index, machineID, mailBoxID);		// send success msg
}

//----------------------------------------------------------------------
//  Wait
//  Call Wait on a CV
//----------------------------------------------------------------------
void wait(int lockID, int index, int machineID, int mailBoxID){
	clientPacket packetToSend;
	client *waitingClient = NULL;
	PacketHeader packet_From_SToS;
	MailHeader mail_From_SToS;
	
	int len = sizeof(packetToSend);
	char *data=new char[len+1];
	data[len]='\0';
		
	if(index < 0 || index > (myMachineID+1)*MAX_CV){		// Is CV index invalid?
		printf("CV TO WAIT ON IS INVALID\n");
		send("WAIT", false, -1, machineID, mailBoxID);
		return;
	}  else if (!ServerCVs[index].valid){		// Are both CV and Lock valid?
		printf("CV AND LOCK ARE INVALID\n");
		send ("WAITCV", false, -2, machineID, mailBoxID);
		return;
	}
	
	if (ServerCVs[index].lockID == -1){		// If lockID of CV is unset
		ServerCVs[index].lockID = lockID;		// Set lockID of CV to current lock
	}
	
	if(SERVERS == 1){
		release(lockID, machineID, mailBoxID, 1);		// release the lock
	}else{
		packetToSend.value = SC_Wait;
		packetToSend.index = lockID;
		packetToSend.syscall = SC_Release;
		packetToSend.ServerArg = 0;
		
		packet_From_SToS.from = machineID;
		mail_From_SToS.from = mailBoxID;
		packet_From_SToS.to = lockID/200;
		mail_From_SToS.to = 0;
		mail_From_SToS.length = sizeof(packetToSend);
		
		memcpy((void *)data,(void *)&packetToSend,len);
		postOffice->Send(packet_From_SToS,mail_From_SToS,data);
	}
	
	ServerCVs[index].count++;		// add CV usage count
	waitingClient = new client;
	waitingClient -> machineID = machineID;
	waitingClient -> mailBoxID = mailBoxID;
	ServerCVs[index].waitingQueue->Append((void *)waitingClient);		// add current client to waiting queue for CV
}

//----------------------------------------------------------------------
//  Broadcast
//  Call Broadcast on a CV
//----------------------------------------------------------------------
void broadcast(int lockID, int index, int machineID, int mailBoxID){
	client signalClient;
	
	if(index < 0 || index > (myMachineID+1)*MAX_CV){
		printf("CV TO WAIT ON IS INVALID\n");
		send("WAIT", false, -1, machineID, mailBoxID);
		return;
	} else if (!ServerCVs[index].valid){
		printf("CV AND LOCK ARE INVALID\n");
		send ("WAITCV", false, -2, machineID, mailBoxID);
		return;
	}
	
	while(!ServerCVs[index].waitingQueue->IsEmpty()){		// Until there are no more waiting Clients
		signal(lockID, index, machineID, mailBoxID);		// Signal all clients
	}
	
	send("BROADCAST", true, index, machineID, mailBoxID);		// send success msg
}

//----------------------------------------------------------------------
//  Create MV
//  Create a new monitor variable
//----------------------------------------------------------------------
void createMV(char *name, int value, int machineID, int mailBoxID){
	for(int i = 0; i < (signed)ServerMVs.size(); i++){		// Check if MV already exists
		if (strcmp(ServerMVs[i].name, name) == 0){
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
		temp.value = value;		// Value is set to initial value of requesting syscall client
		ServerMVs.push_back(temp);
		send("CREATEMV", true, mvID, machineID, mailBoxID);		// send success msg
	}
}

//----------------------------------------------------------------------
//  Get MV
//  Get a monitor variable
//----------------------------------------------------------------------
void getMV(int index, int machineID, int mailBoxID){
	if(index >= ((myMachineID+1)*MAX_MV || index < 0)){		// Is MV index invalid?
		printf("TOO MANY MVs\n");
		send("GETMV", false, -1, machineID, mailBoxID);
		return;
	} else if (!ServerMVs[index].valid){		// Is MV valid?
		printf("MV ISN'T VALID\n");
		send("GETMV", false, -2, machineID, mailBoxID);
		return;
	}
	
	printf("MV at index %d is value %d\n", index, ServerMVs[index].value);
	int x = ServerMVs[index].value;		// Find value of MV
	send("GETMV", true, x, machineID, mailBoxID);		// Return value of MV
	return;
}

//----------------------------------------------------------------------
//  Destroy MV
//  Destroy a monitor variable
//----------------------------------------------------------------------
void destroyMV(int index, int machineID, int mailBoxID){
	if(index >= MAX_MV || index < 0){
		printf("TOO MANY MVs\n");
		send("DESTROYMV", false, -1, machineID, mailBoxID);
		return;
	} else if (!ServerMVs[index].valid){
		printf("MV ISN'T VALID\n");
		send("DESTROYMV", false, -2, machineID, mailBoxID);
		return;
	}
		
	if(ServerMVs[index].count == 0){		// If there are no clients using MV
		delete ServerMVs[index].name;		// delete EVERYTHING
		ServerMVs[index].valid = false;	
		printf("MV IS DESTROYED\n");
		send("DESTROYMV",true,index,machineID,mailBoxID);
	}else{		// If MV is in use, can't destroy it
		printf("\nDESTROY MV : MV CANNOT BE DESTROYED");
		send("DESTROYMV", false, -1, machineID, mailBoxID);
	}
}

//----------------------------------------------------------------------
//  Set MV
//  Set a monitor variable
//----------------------------------------------------------------------
void setMV(int index, int value, int machineID, int mailBoxID){
	if(index >= MAX_MV || index < 0){
		printf("TOO MANY MVs\n");
		send("SETMV", false, -1, machineID, mailBoxID);
		return;
	} else if (!ServerMVs[index].valid){
		printf("MV ISN'T VALID\n");
		send("SETMV", false, -2, machineID, mailBoxID);
		return;
	}
	
	ServerMVs[index].value = value;		// Set MV value to requests value from syscall
	send("SETMV", true, ServerMVs[index].value, machineID, mailBoxID);		// return success msg with new value
	return;
}

//----------------------------------------------------------------------
//  End
//  Ends the server simulation
//----------------------------------------------------------------------

//----------------------------------------------------------------------
//  SearchMyself
//  See if current server has data by iterating through vector and comparing
//  name strings. Return index if a match is found.
//----------------------------------------------------------------------
int SearchMyself(int syscall,char name[],int index){
	int ID = -1;
	if(syscall == SC_CreateLock){
		for (int i = 0; i < (signed)ServerLocks.size(); i++){
			if(strcmp(ServerLocks[i].name, name) == 0){
				ID = i;
				break;
			}
		}
	}
	if(syscall == SC_CreateCV){
		for (int i = 0; i < (signed)ServerCVs.size(); i++){
			if(strcmp(ServerCVs[i].name, name) == 0){
				ID = i;
				break;
			}
		}
	}
	if(syscall == SC_CreateMV){
		for (int i = 0; i < (signed)ServerMVs.size(); i++){
			if(strcmp(ServerMVs[i].name, name) == 0){
				ID = i;
				break;
			}
		}
	}
	if(syscall==SC_GetMV || syscall == SC_SetMV){
		if(ServerMVs[index].valid){
			for (int i = 0; i < (signed)ServerMVs.size(); i++){
				if(strcmp(ServerMVs[i].name, name) == 0){
					ID = i;
					break;
				}
			}
		} else{
			ID = -1;
		}
	}
	if(syscall==SC_Acquire || syscall == SC_Release){
		if(ServerLocks[index].valid){
			for (int i = 0; i < (signed)ServerLocks.size(); i++){
				if(strcmp(ServerLocks[i].name, name) == 0){
					ID = i;
					break;
				}
			}
		} else {
			ID = -1;
		}
	}
	if(syscall==SC_Wait || syscall == SC_Signal || syscall == SC_Broadcast){
		if(ServerCVs[index].valid){
			for (int i = 0; i < (signed)ServerCVs.size(); i++){
				if(strcmp(ServerCVs[i].name, name) == 0){
					ID = i;
					break;
				}
			}
		} else {
			ID = -1;
		}
	}
	return ID;
}

//----------------------------------------------------------------------
//  SwitchCase
//	call different internal methods
//----------------------------------------------------------------------
void switchCase(int syscall,char name[],int index,int index2,int machineID,int mailBoxID,int value){
	switch(syscall){		// Parse based on syscall requested
		case SC_Acquire:
			printf("REQUEST: ACQUIRE LOCK FROM CLIENT\n");
			acquire(index, machineID, mailBoxID);
			break;
		case SC_Release:
			printf("REQUEST: RELEASE LOCK FROM CLIENT\n");
			release(index, machineID, mailBoxID, SC_Release);
			break;
		case SC_Wait:
			printf("REQUEST: WAIT FROM CLIENT\n");
			wait(index, index2, machineID, mailBoxID);
			break;
		case SC_Signal:
			printf("REQUEST: SIGNAL FROM CLIENT\n");
			signal(index, index2, machineID, mailBoxID);
			break;
		case SC_Broadcast:
			printf("REQUEST: BROADCAST FROM CLIENT\n");
			broadcast(index, index2, machineID, mailBoxID);
			break;
		case SC_CreateLock:
			printf("REQUEST: CREATE LOCK FROM CLIENT\n");
			createLock(name,machineID, mailBoxID);
			break;
		case SC_DestroyLock:
			printf("REQUEST: DESTROY LOCK FROM CLIENT\n");
			createLock(name, machineID, mailBoxID);
			break;
		case SC_CreateCV:
			printf("REQUEST: CREATE CV FROM CLIENT\n");
			createCV(name, machineID, mailBoxID);
			break;
		case SC_DestroyCV:
			printf("REQUEST: DESTROY CV FROM CLIENT\n");
			destroyCV(index, machineID, mailBoxID);
			break;
		case SC_CreateMV:
			printf("REQUEST: CREATE MV FROM CLIENT\n");
			createMV(name, value, machineID, mailBoxID);
			break;
		case SC_SetMV:
			printf("REQUEST: SET MV FROM CLIENT\n");
			setMV(index, value, machineID, mailBoxID);
			break;
		case SC_GetMV:
			printf("REQUEST: GET MV FROM CLIENT\n");
			getMV(index, machineID, mailBoxID);
			break;
		case SC_DestroyMV:
			printf("REQUEST: DELETE MV FROM CLIENT\n");
			destroyMV(index, machineID, mailBoxID);
			break;
		default:
			printf("SERVER: INVALID SYSCALL %d\n", syscall);
	}
}

//----------------------------------------------------------------------
//  serverToServer
//  Send messages between servers
//----------------------------------------------------------------------
void serverToServer(clientPacket packet,int serverID){
	PacketHeader pFromSToS,pFromSToC;									/*Creating Machine id objects*/
	MailHeader mFromSToS,mFromSToC;										/*Creating Mailbox id objects*/	
	clientPacket packetToSend;											/*Creating client packet object*/		
	serverPacket packetToSendToClient;									/*Creating server packet object*/
	
	int len = sizeof (packet);
	int clientID, j, index, ID;
	int holdingServer = -1;
	client *waitingClient = NULL;
	
	bool flag=false;
	char *data=new char[len];
	ID = -1;
	
	if(packet.syscall == SC_Wait || packet.syscall == SC_Signal || packet.syscall == SC_Broadcast)
		ID = SearchMyself(packet.syscall, packet.name, packet.index2);				/*Searching in the current server vector*/
	else													
		ID = SearchMyself(packet.syscall, packet.name, packet.index);				/*Searching in the current server vector*/
			
	clientID = packet.clientID;
	pFromSToC.to = clientID/100;																			/*Assigning the machine id of the client to send the packet to the client directly*/
	mFromSToC.to = clientID%100;																			/*Assigning the mailbox id of the client to send the packet to the client directly*/
	pFromSToC.from = myMachineID;
	mFromSToC.from = 0;
	mFromSToC.length = sizeof(packet);
	if(ID!=-1){																								/*The program comes here if the current server has got the client's syscall in its vector*/
		flag = 1;
		switchCase(packet.syscall,packet.name,packet.index,packet.index2,pFromSToC.to,mFromSToC.to,packet.value);
	}
	else{
		index = -1;																							/*If the syscall is not in the current server's vector, it will add the client packet to its pendingRequest structure*/
		for(j=0;j<200;j++){
			if(requests[j].received){
				if(requests[j].syscall==packet.syscall && !strcmp(requests[j].name,packet.name)){
					index = j;																				/*Checks if the syscall is already in its requests structure*/
					break;
				}
			}
		}
		waitingClient = new client;
		waitingClient->machineID = pFromSToC.to;
		waitingClient->mailBoxID = mFromSToC.to;
		if(index !=-1){
			if(packet.syscall==SC_CreateLock || packet.syscall==SC_CreateCV || packet.syscall == SC_CreateMV){			
				if (serverID < myMachineID) {
					if(requests[index].doYouHave[serverID] == 0){										/*If current server's machine id > requested server's machine id*/
						printf("\n Replying yes to Server %d since that server already said NO",serverID);
						flag = 1;
						requests[index].requestWaitQueue->Append((void *)waitingClient);				/*If the syscall is already present in current server's pending queue, it will append the current syscall request to the index of its already present syscall using a queue data structure*/
						// Send YES - 1
					}else if(requests[index].doYouHave[serverID] == 1){									/*Sets the flag to 1 if the requested server's doYouHave has the value 1*/
						flag = 0;
						printf("\nReplying no to Server %d since my priority is low for request(%d,%s)", serverID, packet.syscall, packet.name);
						//Send NO - 0
					}else{																					/*Sets the flag to 0 if the requested server's doYouHave doesn't have the value either 0 or 1 */
						flag = 0;
						printf("\nReplying NO to Server %d since my priority is low for request(%d,%s)",serverID,packet.syscall,packet.name);
						//Send NO - 0
					}
				}else{ //My machine's priority is more													/*If current server's machine id < requested server's machine id*/
					if (requests[index].doYouHave[serverID] == 0 ) {
						flag = 1;
						printf("\n Replying yes to Server %d since my priority is higher",serverID);
						requests[index].requestWaitQueue->Append((void *)waitingClient);
						// Send  YES - 1
					}else if(requests[index].doYouHave[serverID] == 1){								/*Sets the flag to 0 if the requested server's doYouHave has the value 1*/
						flag = 0;
						printf("\n Replying no to Server %d since that server already said yes",serverID);
					}else{																				/*Sets the flag to 1 if the requested server's doYouHave doesn't have the value either 0 or 1 */
						flag = 1;
						printf("\n Replying YES to Server %d since that server didn't reply anything to me yet and my priority is higher",serverID);
						requests[index].requestWaitQueue->Append((void *)waitingClient);
					}
				}
			}
		}else{																							/*Enters here if the current server doesn't have the requested syscall in its pendingQueue*/		
			flag = 0;
			printf("\nReplying no to Server %d since I don't have any pending request(%d,%s)",serverID,packet.syscall,packet.name);
			//Send NO - 0
		}
	}
	
	packetToSend.clientID = clientID;																	/*Assigning the required fields to a new packet which is to be sent*/	
	packetToSend.syscall = packet.syscall;
	packetToSend.status = flag;
	packetToSend.index = packet.index;
	packetToSend.index2 = packet.index2;
	packetToSend.ServerArg = 2;																			/*Assigning the serverArg value to 2 which indicates that the reply is for the doYouHave message*/
	strcpy(packetToSend.name,packet.name);
	packetToSend.value = packet.value;
	
	mFromSToS.to = 0;
	mFromSToS.length = sizeof(packet);
	pFromSToS.to = SERVERS;
	pFromSToS.from = myMachineID;
	
	printf("\n Server %d sending %d to Server %d's doYouHave request",myMachineID,flag,serverID); 
	memcpy((void *)data,(void *)&packetToSend,len);
	postOffice->Send(pFromSToS,mFromSToS,data);															/*Sending the reply*/
	return;
}

//----------------------------------------------------------------------
//  RunServer
//  Runs the server using a switch statement to handle incoming messages
//----------------------------------------------------------------------
void RunServer(){
	PacketHeader packet_From_Client, pFromSToS;
	MailHeader mail_From_Client, mFromSToS;
	clientPacket packet;
	clientPacket packetSend;
	int len = sizeof(packet);
	char *data = new char[len];
	//data[len] = '\0';
	
	requestMap = new BitMap(5000);
	                                
	int index;
	int clientID,j;
	int count = 0;
	int terminatingCount = 0;
	int ID = -1;
	int holdingServer = -1;
	
	client *waitingClient = NULL;
	client *processingClient = NULL;
	
	
	while(true){		// Run forever
		printf("SERVER: WAITING FOR CLIENT REQUEST\n");
		postOffice->Receive(0,&packet_From_Client, &mail_From_Client, data);		// Receive packet from Clients
		memcpy((void *)&packet, (void *)data, len);
		
		if(packet.ServerArg == 0){ // Request From client	
			if(SERVERS == 1){	
				switchCase(packet.syscall, packet.name, packet.index, packet.index2, packet_From_Client.from, mail_From_Client.from, packet.value);
				continue;
			}
			
			mFromSToS.from = 0;			/*Assign it the required fields to send the packet*/
			mFromSToS.to = 0;
			pFromSToS.from = myMachineID;
			mFromSToS.length = mail_From_Client.length;
			
			packetSend.clientID = (packet_From_Client.from*100)+mail_From_Client.from;		/*This is done for appending both the machine and the mailbox id into a single value, for the server to identify the credentials of the client*/
			packetSend.index = packet.index;
			packetSend.index2 = packet.index2;
			packetSend.value = packet.value;
			packetSend.syscall = packet.syscall;
			strcpy(packetSend.name,packet.name);
			packetSend.ServerArg = 1;		// Msg is from a Server
			
			ID = -1;
			if(packet.syscall == SC_Wait || packet.syscall == SC_Signal || packet.syscall == SC_Broadcast){
				ID = SearchMyself(packet.syscall,packet.name,packet.index2); /* Check to see if ID exists in vector */
			}else{
				ID = SearchMyself(packet.syscall,packet.name,packet.index);
			}
			
			if(ID == -1){		// Server doesn't have the data
				if(packet.syscall==SC_CreateLock || packet.syscall==SC_CreateCV || packet.syscall == SC_CreateMV){		// Incoming request from client is for create	
					index = -1;
					for(j=0;j<200;j++){
						if(requests[j].received){		// Look through request list to see if there exists a request that matches incoming client request
							if(requests[j].syscall==packet.syscall && !strcmp(requests[j].name,packet.name)){
								index = j;		// If there is, set index to that request index
								break;
							}	
						}
					}
					if(index == -1){		// If there are no server requests that match client request
						index = requestMap->Find();
						requests[index].requestWaitQueue = new List;
						for(j = 0; j < SERVERS; j++) requests[index].doYouHave[j] = 3;
						if(requests[index].requestWaitQueue->IsEmpty())	printf("\n List is initially empty");
						
					}
					
					waitingClient = new client;
					waitingClient->machineID = packet_From_Client.from;
					waitingClient->mailBoxID = mail_From_Client.from;
					
					requests[index].syscall = packet.syscall;		// Create a request 
					strcpy(requests[index].name,packet.name);
					requests[index].value = packet.value;
					requests[index].received = true;
					requests[index].requestWaitQueue->Append((void *)waitingClient);
					requests[index].doYouHave[myMachineID] = 0;
					memcpy((void *)data,(void *)&packetSend,len);
					
					for(j = 0; j < SERVERS; j++){
						if(j != myMachineID){ 										// Current Server shouldn't send to its own mailbox
							printf("Asking servers if they have\n");
							pFromSToS.to = j;
							postOffice->Send(pFromSToS,mFromSToS,data);
						}
					}
				} else if(packet.syscall==SC_Acquire || packet.syscall==SC_Release || packet.syscall==SC_GetMV || packet.syscall==SC_SetMV){																	// Handling non create requests
					packetSend.index = packet.index;
					packetSend.syscall = packet.syscall;
					packetSend.ServerArg = 0;
					
					pFromSToS.from = packet_From_Client.from;
					mFromSToS.from = mail_From_Client.from;
					pFromSToS.to = packet.index/200;
					mFromSToS.to = 0;
					mFromSToS.length = sizeof(packetSend);
					
					memcpy((void *)data,(void *)&packetSend,len);
					postOffice->Send(pFromSToS,mFromSToS,data);
				}else{		/* If syscall is for Signal, Wait, or Broadcast */
					packetSend.index = packet.index;
					packetSend.syscall = packet.syscall;
					packetSend.ServerArg = 0;
					
					pFromSToS.from = packet_From_Client.from;
					mFromSToS.from = mail_From_Client.from;
					pFromSToS.to = packet.index2/200;
					mFromSToS.to = 0;
					mFromSToS.length = sizeof(packetSend);
					
					memcpy((void *)data,(void *)&packetSend,len);
					postOffice->Send(pFromSToS,mFromSToS,data);		// Send packet to another server
				}
			}else{		// Processing the request since the resource is available in the current server
				switchCase(packet.syscall, packet.name, packet.index, packet.index2, packet_From_Client.from, mail_From_Client.from, packet.value);
			}
		} else if(packet.ServerArg == 1){		// doYouHave request From server
			printf("\nInside doyouhave request server message : From Server : %d", packet_From_Client.from);
			serverToServer(packet, packet_From_Client.from);
		} else if(packet.ServerArg ==2){		// doYouHave reply from server
			index = -1;
			for(j=0;j<200;j++){
				if(requests[j].received){		// Identify the corresponding entry in the pending queue
					if(requests[j].syscall == packet.syscall && !strcmp(requests[j].name, packet.name)){
						index = j;
						break;
					}	
				}else {
					index = -1;
				}
			}
			if(index != -1){
				requests[index].doYouHave[packet_From_Client.from] = packet.status;
				count = 0;
				terminatingCount = 0;
				for(j=0;j<SERVERS;j++){												
					if(requests[index].doYouHave[j]==0)	count++;		// Identifying the number of servers which said NO
					if(requests[index].doYouHave[j]!=3)	terminatingCount++;	// Identifying the number of servers which replied	
				}
				printf("\n So far Doyouhave msgs from %d servers have been received",count);
				if(count==SERVERS){
					while(!(requests[index].requestWaitQueue->IsEmpty())){		// Process the request if all the other servers said No for the DoYouHave request
						processingClient = ((client*)requests[index].requestWaitQueue->Remove());
						switchCase(packet.syscall, packet.name, packet.index, packet.index2, processingClient->machineID, processingClient->mailBoxID, packet.value);
					}
				}
				if(terminatingCount==SERVERS){		// Remove the entry in the pending queue if all the servers replied
					requests[index].syscall = -1;
					requests[index].name[0] = '\0';
					requests[index].received = false;
				}
			} else {
				printf("\nPending Queue(%d,%s): %d is no longer valid", packet.syscall, packet.name, index);
			}
		} else{													// Never occurs
			printf("\nINVALID SERVER ARGUMENT: %d\n", packet.ServerArg);
		}
	}
}
