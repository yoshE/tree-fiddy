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

std::vector<ServerLock> ServerLocks;		// vector of server locks
std::vector<ServerCV> ServerCVs;		// vector of server cvs
std::vector<ServerMV> ServerMVs;		// vector of server mvs

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
		printf("SERVERLOCKS[i] IS \"%s\"\n", ServerLocks[i].name);
		printf("NAME IS \"%s\"\n", name);
		if (strncmp(ServerLocks[i].name, name, 30)){	// If lock already exists in the server then return that ID
			send("CREATELOCK", true, i, machineID, mailBoxID);
			return;
		} else if (ServerLocks[i].name == name){		// If lock already exists in the server then return that ID
			send("CREATELOCK", true, i, machineID, mailBoxID);
			return;
		}
	}
	
	lockID = (signed)ServerLocks.size();		// new ID is equal to the size of vector
	if (lockID == MAX_LOCK || lockID == -1){		// If vector is "full" (over 200) then fail
		send("CREATELOCK", false, lockID, machineID, mailBoxID);
		return;
	}
	printf("NAMEEEEE IS \"%s\"\n", name);
	ServerLock temp;		// new struct ServerLock that will be added to ServerLocks[]
	temp.name = new char[sizeof(name)+1];
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
	
	if(index < 0 || index > MAX_LOCK){		// if lock to be acquired has an incorrect index (under 0 or over 200)
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
	if(index < 0 || index > MAX_LOCK){
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
	if(index < 0 || index > MAX_LOCK){
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
		if (ServerCVs[i].name == name){
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
	client signalClient;	// client that will be signalled
	
	if(lockID < 0 || index > MAX_LOCK){		// Is lock index invalid?
		printf("LOCK TO SIGNAL IS INVALID\n");
		send("SIGNAL", false, -2, machineID, mailBoxID);
		return;
	} else if (index < 0 || index > MAX_CV){		// Is CV index invalid?
		printf("SIGNAL INVALID INDEX\n");
		send("SIGNAL", false, -2, machineID, mailBoxID);
		return;
	} else if (!ServerLocks[lockID].valid || !ServerCVs[index].valid){		// Are both lock and CV valid?
		printf("LOCK WITH CV IS INVALID\n");
		send("SIGNAL", false, -2, machineID, mailBoxID);
		return;
	} else if (ServerLocks[lockID].Owner.machineID != machineID || ServerLocks[lockID].Owner.mailBoxID != mailBoxID){		// Does client own lock?
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
	acquire(index, signalClient.machineID, signalClient.mailBoxID);		// acquire the lock for the client
	send("SIGNAL", true, index, machineID, mailBoxID);		// send success msg
}

//----------------------------------------------------------------------
//  Wait
//  Call Wait on a CV
//----------------------------------------------------------------------
void wait(int lockID, int index, int machineID, int mailBoxID){
	client *waitingClient = NULL;		// client that will be appended to wait queue
		
	if(lockID < 0 || lockID > MAX_LOCK){		// Is lock index invalid?
		printf("LOCK TO WAIT ON IS INVALID\n");
		send("WAIT", false, -1, machineID, mailBoxID);
		return;
	} else if(index < 0 || index > MAX_CV){		// Is CV index invalid?
		printf("CV TO WAIT ON IS INVALID\n");
		send("WAIT", false, -1, machineID, mailBoxID);
		return;
	}  else if (!ServerLocks[lockID].valid || !ServerCVs[index].valid){		// Are both CV and Lock valid?
		printf("CV AND LOCK ARE INVALID\n");
		send ("WAITCV", false, -2, machineID, mailBoxID);
		return;
	}
	
	if (ServerCVs[index].lockID == -1){		// If lockID of CV is unset
		ServerCVs[index].lockID = lockID;		// Set lockID of CV to current lock
	}
	
	release(lockID, machineID, mailBoxID, 1);		// release the lock
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
	if(index >= MAX_MV || index < 0){		// Is MV index invalid?
		printf("TOO MANY MVs\n");
		send("GETMV", false, -1, machineID, mailBoxID);
		return;
	} else if (!ServerMVs[index].valid){		// Is MV valid?
		printf("MV ISN'T VALID\n");
		send("GETMV", false, -2, machineID, mailBoxID);
		return;
	}
	
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
		
	if(serverMVs[index].count == 0){		// If there are no clients using MV
		delete serverMV[index].name;		// delete EVERYTHING
		serverMV[index].valid = false;	
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
//  RunServer
//  Runs the server using a switch statement to handle incoming messages
//----------------------------------------------------------------------
void RunServer(){
	PacketHeader packet_From_Client;
	MailHeader mail_From_Client;
	clientPacket packet;
	int len = sizeof(packet);
	char *data = new char[len];
	//data[len] = '\0';
	
	while(true){		// Run forever
		printf("SERVER: WAITING FOR CLIENT REQUEST\n");
		
		postOffice->Receive(0,&packet_From_Client, &mail_From_Client, data);		// Receive packet from Clients
		memcpy((void *)&packet, (void *)data, len);
		
		switch(packet.syscall){		// Parse based on syscall requested
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
