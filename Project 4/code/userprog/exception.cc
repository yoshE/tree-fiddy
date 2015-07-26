// exception.cc 
//	Entry point into the Nachos kernel from user programs.
//	There are two kinds of things that can cause control to
//	transfer back to here from user code:
//
//	syscall -- The user code explicitly requests to call a procedure
//	in the Nachos kernel.  Right now, the only function we support is
//	"Halt".
//
//	exceptions -- The user code does something that the CPU can't handle.
//	For instance, accessing memory that doesn't exist, arithmetic errors,
//	etc.  
//
//	Interrupts (which can also cause control to transfer from user
//	code into the Nachos kernel) are handled elsewhere.
//
// For now, this only handles the Halt() system call.
// Everything else core dumps.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "system.h"
#include "syscall.h"
#include "synch.h"
#include <stdio.h>
#include <sstream>
#include <vector>
#include <iostream>
#include <cstdio>
#include <cstring>

#define NameSize 30
using namespace std;

Lock* syscallLock = new Lock("Syscall");
std::vector<KernelLock>LockTable;		// Table of all current Locks
std::vector<KernelCV>CVTable;		// Table of all current Condition Variables
Lock* PrintfLock = new Lock("PrintfLock");
Lock* LockTableLock = new Lock("LockTableLock");		// Lock for accessing the LockTable
Lock* CVTableLock = new Lock("CVTableLock");		// Lock for accessing the CVTable

bool PopulateTLB_IPT(int currentVPN);

int copyin(unsigned int vaddr, int len, char *buf) {
    // Copy len bytes from the current thread's virtual address vaddr.
    // Return the number of bytes so read, or -1 if an error occors.
    // Errors can generally mean a bad virtual address was passed in.
    bool result;
    int n=0;			// The number of bytes copied in
    int *paddr = new int;

    while ( n >= 0 && n < len) {
      result = machine->ReadMem( vaddr, 1, paddr );
      while(!result) // FALL 09 CHANGES
	  {
   			result = machine->ReadMem( vaddr, 1, paddr ); // FALL 09 CHANGES: TO HANDLE PAGE FAULT IN THE ReadMem SYS CALL
	  }	
      
      buf[n++] = *paddr;
     
      if ( !result ) {
	//translation failed
	return -1;
      }

      vaddr++;
    }

    delete paddr;
    return len;
}

int copyout(unsigned int vaddr, int len, char *buf) {
    // Copy len bytes to the current thread's virtual address vaddr.
    // Return the number of bytes so written, or -1 if an error
    // occors.  Errors can generally mean a bad virtual address was
    // passed in.
    bool result;
    int n=0;			// The number of bytes copied in

    while ( n >= 0 && n < len) {
      // Note that we check every byte's address
      result = machine->WriteMem( vaddr, 1, (int)(buf[n++]) );

      if ( !result ) {
	//translation failed
	return -1;
      }

      vaddr++;
    }

    return n;
}

void Create_Syscall(unsigned int vaddr, int len) {
    // Create the file with the name in the user buffer pointed to by
    // vaddr.  The file name is at most MAXFILENAME chars long.  No
    // way to return errors, though...
    char *buf = new char[len+1];	// Kernel buffer to put the name in

    if (!buf) return;

    if( copyin(vaddr,len,buf) == -1 ) {
	printf("%s","Bad pointer passed to Create\n");
	delete buf;
	return;
    }

    buf[len]='\0';

    fileSystem->Create(buf,0);
    delete[] buf;
    return;
}

int Open_Syscall(unsigned int vaddr, int len) {
    // Open the file with the name in the user buffer pointed to by
    // vaddr.  The file name is at most MAXFILENAME chars long.  If
    // the file is opened successfully, it is put in the address
    // space's file table and an id returned that can find the file
    // later.  If there are any errors, -1 is returned.
    char *buf = new char[len+1];	// Kernel buffer to put the name in
    OpenFile *f;			// The new open file
    int id;				// The openfile id

    if (!buf) {
	printf("%s","Can't allocate kernel buffer in Open\n");
	return -1;
    }

    if( copyin(vaddr,len,buf) == -1 ) {
	printf("%s","Bad pointer passed to Open\n");
	delete[] buf;
	return -1;
    }

    buf[len]='\0';

    f = fileSystem->Open(buf);
    delete[] buf;

    if ( f ) {
	if ((id = currentThread->space->fileTable.Put(f)) == -1 )
	    delete f;
	return id;
    }
    else
	return -1;
}

void Write_Syscall(unsigned int vaddr, int len, int id) {
    // Write the buffer to the given disk file.  If ConsoleOutput is
    // the fileID, data goes to the synchronized console instead.  If
    // a Write arrives for the synchronized Console, and no such
    // console exists, create one. For disk files, the file is looked
    // up in the current address space's open file table and used as
    // the target of the write.
    
    char *buf;		// Kernel buffer for output
    OpenFile *f;	// Open file for output

    if ( id == ConsoleInput) return;
    
    if ( !(buf = new char[len]) ) {
	printf("%s","Error allocating kernel buffer for write!\n");
	return;
    } else {
        if ( copyin(vaddr,len,buf) == -1 ) {
	    printf("%s","Bad pointer passed to to write: data not written\n");
	    delete[] buf;
	    return;
	}
    }

    if ( id == ConsoleOutput) {
      for (int ii=0; ii<len; ii++) {
	printf("%c",buf[ii]);
      }

    } else {
	if ( (f = (OpenFile *) currentThread->space->fileTable.Get(id)) ) {
	    f->Write(buf, len);
	} else {
	    printf("%s","Bad OpenFileId passed to Write\n");
	    len = -1;
	}
    }

    delete[] buf;
}

int Read_Syscall(unsigned int vaddr, int len, int id) {
    // Write the buffer to the given disk file.  If ConsoleOutput is
    // the fileID, data goes to the synchronized console instead.  If
    // a Write arrives for the synchronized Console, and no such
    // console exists, create one.    We reuse len as the number of bytes
    // read, which is an unnessecary savings of space.
    char *buf;		// Kernel buffer for input
    OpenFile *f;	// Open file for output

    if ( id == ConsoleOutput) return -1;
    
    if ( !(buf = new char[len]) ) {
	printf("%s","Error allocating kernel buffer in Read\n");
	return -1;
    }

    if ( id == ConsoleInput) {
      //Reading from the keyboard
      scanf("%s", buf);

      if ( copyout(vaddr, len, buf) == -1 ) {
	printf("%s","Bad pointer passed to Read: data not copied\n");
      }
    } else {
	if ( (f = (OpenFile *) currentThread->space->fileTable.Get(id)) ) {
	    len = f->Read(buf, len);
	    if ( len > 0 ) {
	        //Read something from the file. Put into user's address space
  	        if ( copyout(vaddr, len, buf) == -1 ) {
		    printf("%s","Bad pointer passed to Read: data not copied\n");
		}
	    }
	} else {
	    printf("%s","Bad OpenFileId passed to Read\n");
	    len = -1;
	}
    }

    delete[] buf;
    return len;
}

void Close_Syscall(int fd) {
    // Close the file associated with id fd.  No error reporting.
    OpenFile *f = (OpenFile *) currentThread->space->fileTable.Remove(fd);

    if ( f ) {
      delete f;
    } else {
      printf("%s","Tried to close an unopen file\n");
    }
}

int rand_Syscall(){
	return rand();
}


#ifdef NETWORK		// If we are running from network then use these receive and send functions
void SendToPO(char *syscallType,clientPacket packet){		// Sends a packet from the client to the server
	PacketHeader packet_From_Client;		// Packet to send
	MailHeader mail_From_Client;
	int len = sizeof(packet);		// Find size of given packet
	char* data = new char[len];		// Creates a char array data that contains the packet
	packet.ServerArg = 0;
	memcpy((void *)data, (void *)&packet, len);		// Copy in the data
	// data[len - 1] = '\0';

	if(SERVERS <= 1){
		packet_From_Client.to = 0;
	} else {
		packet_From_Client.to = rand()%SERVERS;
	}

	mail_From_Client.to = 0;
	packet_From_Client.from = myMachineID;
	mail_From_Client.from = currentThread->getPID();
	mail_From_Client.length = len;
	
	bool s = postOffice->Send(packet_From_Client, mail_From_Client, data);		// Sends the data from the client to the server
	if (!s){		// If send returned as a failure
		printf("COULDN'T SEND DATA\n");
		interrupt->Halt();
	}
}

int ReceiveFromPO(char *syscallType){		// Receive packets from the server to the client
	PacketHeader packet_From_Server;
	MailHeader mail_From_Server;
	serverPacket packet;
	
	int len = sizeof(packet);
	char *data = new char[len + 1];
	
	postOffice->Receive(0, &packet_From_Server, &mail_From_Server, data);		// Receive the packet from the server
	memcpy((void *)&packet, (void *)data, len);		// Copy the data into the packet
	if (!packet.status) return -1;		// If packet had status failure, then return error
	return packet.value;		// Return value of packet
} 

serverPacket ReceivePacket(char *syscallType){			/*Function that performs PostOffice->Receive , which is mainly used for handling Monitor Variables*/
	PacketHeader pFromSToC;									/*Creating an object of type PacketHeader, which has the information about the data in the packet*/		
	MailHeader mFromSToC;									/*Creating an object of type MailHeader, which has the mail ID*/
	serverPacket tempPacketReceive;
	
	int len=sizeof(tempPacketReceive);
	char *data=new char[len+1];

	postOffice->Receive(0,&pFromSToC,&mFromSToC,data); 		/*Performing the "ReceiveFromPO" function, that receives the data from server to client*/
	memcpy((void *)&tempPacketReceive,(void*)data,len);		/*Performing copy function into a packet structure*/
	
	return tempPacketReceive;
}
#endif

void Acquire_Syscall(int n){		// Syscall to acquire a lock... takes an int that corresponds to lock (in their tables)
	#ifdef NETWORK
		clientPacket packet;
		packet.syscall = SC_Acquire;		// The syscall you want is SC_Acquire
		packet.index = n;		// You want to acquire a lock at index n
		
		SendToPO("ACQUIRE", packet);		// Send packet with request to server
		n = ReceiveFromPO("ACQUIRE");		// retrieve returned value (index of lock if acquired)
		
		if(n == -1){		// If lock wasn't acquired due to out of index error
			printf("LOCK ACQUIRE ERROR\n");
			interrupt->Halt();
		} else {		// Otherwise acquired the lock!
			printf("LOCK ACQUIRE SUCCESSFUL\n");
		}
	#else
		LockTableLock->Acquire();
		if (n < 0 || n > (signed)LockTable.size() - 1 || LockTable[n].IsDeleted || LockTable[n].Kernel_Lock == NULL){		// Check if data exists for entered value
			printf("%s", "Invalid Lock Table Number in Acquire.\n");
			LockTableLock->Release();
			return;		// If the int entered is wrong, print error and return
		}
		LockTable[n].Kernel_Lock->Acquire();		// Acquire said lock
		LockTableLock->Release();
	#endif
}

void Release_Syscall(int n){		// Syscall to release a lock... takes an int that corresponds to lock (in their tables)
	#ifdef NETWORK
		clientPacket packet;
		packet.syscall = SC_Release;		// Syscall you want is SC_Release
		packet.index = n;		// Add index of lock you want to release to packet
		
		SendToPO("RELEASE", packet);		// send packet to Server
		n = ReceiveFromPO("RELEASE");		// Receive packet from the Server
		
		if(n == -1){		// If release was a failure
			printf("LOCK RELEASE ERROR\n");
			interrupt->Halt();
		} else {
			printf("LOCK RELEASE SUCCESSFUL\n");
		}
	#else
		LockTableLock->Acquire();
	
		if (n < 0 || n > (signed)LockTable.size() - 1 || LockTable[n].Kernel_Lock == NULL){		// Check if data exists for entered value
			printf("%s", "Invalid Lock Table Number in Release.\n");
			LockTableLock->Release();
			return;		// If the int entered is wrong, print error and return
		}
		LockTable[n].Kernel_Lock->Release();		// Releases said lock
		LockTableLock->Release();
	
		if (LockTable[n].IsDeleted && LockTable[n].Kernel_Lock->waitingThreads->IsEmpty()){		// If Lock is set for deletion and there are no threads waiting...
			LockTable[n].Kernel_Lock = NULL;		// Delete this lock
		}
	#endif
}

void Wait_Syscall(int x, int lock){		// Syscall for CV Wait... first int is for position of CV, second is for position of Lock (in their tables)
	#ifdef NETWORK
		clientPacket packet;
		packet.syscall = SC_Wait;		// syscall is SC_Signal
		packet.index = lock;		// Add value of lockID you want to wait on
		packet.index2 = x;		// Add value of CV you want to wait on
		
		SendToPO("WAIT", packet);
		int n = ReceiveFromPO("WAIT");
		
		if (n == -1){
			printf("WAIT ERROR\n");
			interrupt->Halt();
		} else {
			printf("WAIT SUCCESSFUL\n");
		}
	#else
	
		CVTableLock->Acquire();
	
		if (x < 0 || x > (signed)CVTable.size() - 1 || CVTable[x].CV == NULL || lock < 0 || lock > (signed)LockTable.size() - 1 || LockTable[lock].Kernel_Lock == NULL ){		// Check if data exists for entered value
			printf("%s", "Invalid CV Table Number and/or Invalid Lock Table Number.\n");
			CVTableLock->Release();
			LockTable[lock].Kernel_Lock->Release();
			return;		// Return if either of the values entered are incorrect
		}
		CVTableLock->Release();
		CVTable[x].CV->Wait(LockTable[lock].Kernel_Lock);		// Wait on said lock
		LockTable[lock].Kernel_Lock->Release();
	#endif
}

void Signal_Syscall(int y, int a){		// Syscall call for Signal... first int is for position of CV, second is for position of Lock (in their tables)	
	#ifdef NETWORK
		clientPacket packet;
		packet.syscall = SC_Signal;		// Syscall is SC_Signal
		packet.index = a;		// First value is index of lock
		packet.index2 = y;
		
		SendToPO("SIGNAL", packet);
		int n = ReceiveFromPO("SIGNAL");
		
		if (n == -1){
			printf("SIGNAL ERROR\n");
			interrupt->Halt();
		} else {
			printf("SIGNAL SUCCESSFUL\n");
		}
	#else
	
		CVTableLock->Acquire();
		if (y < 0 || y > (signed)CVTable.size() - 1 || CVTable[y].CV == NULL || a < 0 || a > (signed)LockTable.size() - 1 || LockTable[a].Kernel_Lock == NULL  ){		// Check if data exists for entered value
			printf("%s", "Invalid CV Table Number and/or Invalid Lock Table Number.\n");
			CVTableLock->Release();
			LockTable[a].Kernel_Lock->Release();
			return;		// Return if either of the values entered are incorrect
		}
	
		CVTable[y].CV->Signal(LockTable[a].Kernel_Lock);		// Signal said CV
		if (CVTable[y].IsDeleted && CVTable[y].CV->waitingCV->IsEmpty()){		// If CV is set for deletion and there are no threads waiting...
			CVTable[y].CV = NULL;		// Delete the CV
		}
		CVTableLock->Release();
	#endif
}

void Broadcast_Syscall(int z, int b){		// Broadcast syscall for CV... first int is for position of CV, second is for position of Lock (in their tables)
	#ifdef NETWORK
		clientPacket packet;
		packet.syscall = SC_Broadcast;		// Syscall is Broadcast
		packet.index = b;		// first index is lockID
		packet.index2 = z;		// Second index is CV ID
		
		SendToPO("BROADCAST", packet);
		int n = ReceiveFromPO("BROADCAST");
		
		if (n == -1){
			printf("BROADCAST ERROR\n");
			interrupt->Halt();
		} else {
			printf("BROADCAST SUCCESSFUL\n");
		}
	#else
		CVTableLock->Acquire();
	
		if (z < 0 || z > (signed)CVTable.size() - 1 || CVTable[z].CV == NULL || b < 0 || b > (signed)LockTable.size() - 1 || LockTable[b].Kernel_Lock == NULL ){		// Check if data exists for entered value
			printf("%s", "Invalid CV Table Number and/or Invalid Lock Table Number.\n");
			CVTableLock->Release();
			LockTable[b].Kernel_Lock->Release();
			return;		// Return if either of the values entered are incorrect
		}
	
		CVTable[z].CV->Broadcast(LockTable[b].Kernel_Lock);		// Broadcast on said lock
		CVTableLock->Release();
	#endif
}

int CreateLock_Syscall(unsigned int vaddr, int index){		// Creates a new lock syscall
	char *name = new char[NameSize];
	
	if (copyin(vaddr, NameSize, name) == -1){
		DEBUG('a', "bad pointer passed to create lock\n");
		delete[] name;
		return -2;
	}
	
	stringstream ss;
	ss << name << index;
	std::string se = ss.str();
	char* s = new char[se.size() + 1];
	std::copy(se.begin(), se.end(), s);
	s[se.size()] = '\0';

	#ifdef NETWORK
		clientPacket packet;
		packet.syscall = SC_CreateLock;
		strncpy(packet.name, s, sizeof(s));
		SendToPO("CREATELOCK", packet);
		int n = ReceiveFromPO("CREATELOCK");
		
		if (n == -1){
			printf("CREATE LOCK ERROR\n");
			interrupt->Halt();
		} else {
			printf("CREATE LOCK SUCCESSFUL\n");
		}
	#else
		DEBUG('a',"%s: CreateLock_Syscall initiated.\n", currentThread->getName());
		LockTableLock->Acquire();
	
		KernelLock new_Lock;		// Creates a new lock struct
		Lock* tempLock = new Lock(s);		// Creates a new lock
		new_Lock.Kernel_Lock = tempLock;		// Places lock into struct
		new_Lock.Owner = NULL;		// Sets Owner of lock to NULL (default)
		new_Lock.IsDeleted = false;		// Lock has not been deleted and is functional
	
		LockTable.push_back(new_Lock);		// Adds lock struct to vector
		int n = LockTable.size() - 1;		// Finds index value of newly created lock
	
		LockTableLock->Release();		
		DEBUG('a', "%s: Created Lock with number %d\n", currentThread->getName(), n);
	#endif
	
	return n;		// Return that value to the thread so they can acquire/release the new lock
}

void DestroyLock_Syscall(int n){		// Destroys an existing lock syscall
	#ifdef NETWORK
		clientPacket packet;
		packet.syscall = SC_DestroyLock;
		packet.index = n;
		
		SendToPO("DESTROYLOCK", packet);
		n = ReceiveFromPO("DESTROYLOCK");
		
		if (n == -1){
			printf("DESTROY LOCK ERROR\n");
			interrupt->Halt();
		} else {
			printf("DESTROY LOCK SUCCESSFUL\n");
		}
	#else
		LockTableLock->Acquire();
	
		if (n < 0 || n > (signed)LockTable.size() - 1 || LockTable[n].Kernel_Lock == NULL ){		// Check if data exists for entered value
			printf("%s", "Invalid Lock Table Number.\n");
			LockTableLock->Release();		// If Lock doesn't exist (or is already deleted) return failure
			return;
		}
		
		if (!LockTable[n].Kernel_Lock->waitingThreads->IsEmpty() && !LockTable[n].IsDeleted){		// If threads are waiting for Lock and Deleted hasn't been set
			LockTable[n].IsDeleted = true;		// Set the lock for deletion
			printf("%s", "Lock has waiting Threads, setting Delete\n");
		} else if (LockTable[n].Kernel_Lock->waitingThreads->IsEmpty() && !LockTable[n].IsDeleted){			// If no threads are waiting for Lock and Deleted hasn't been set
			LockTable[n].IsDeleted = true;		// Set the lock for deletion
			LockTable[n].Kernel_Lock = NULL;		// Delete the lock
			printf("%s", "Lock has no waiting Threads, setting Delete and Lock\n");
		} else if (LockTable[n].Kernel_Lock->waitingThreads->IsEmpty() && LockTable[n].IsDeleted){		// If no threads are waiting for Lock and Deleted has been set
			LockTable[n].Kernel_Lock = NULL;		// Delete the lock
			printf("%s", "Lock has waiting Threads, now setting Lock to NULL\n");
		}
		LockTableLock->Release();
	#endif
}

int CreateCV_Syscall(unsigned int vaddr, int index){		// Creates a new CV in CVTable
	char *name = new char[NameSize];
	
	if (copyin(vaddr, NameSize, name) == -1){
		DEBUG('a', "bad pointer passed to create CV\n");
		delete[] name;
		return -2;
	}
	
	stringstream ss;
	ss << name << index;
	string se = ss.str();
	char* s = new char[se.size() + 1];
	std::copy(se.begin(), se.end(), s);
	s[se.size()] = '\0';
	
	#ifdef NETWORK
		clientPacket packet;
		packet.syscall = SC_CreateCV;
		strncpy(packet.name, s, sizeof(s));
		
		SendToPO("CREATECV", packet);
		int x = ReceiveFromPO("CREATECV");
		
		if (x == -1){
			printf("CREATECV ERROR\n");
			interrupt->Halt();
		} else {
			printf("CREATECV SUCCESSFUL\n");
		}
	#else
		CVTableLock->Acquire();
		KernelCV new_cv;		// New CV Struct
		Condition* cv = new Condition(s);		// New CV
		new_cv.CV = cv;		// Places new CV into struct
		new_cv.IsDeleted = false;		// CV has not been deleted yet
	
		CVTable.push_back(new_cv);		// Adds new struct to CVTable
		int x = CVTable.size() - 1;		// Finds index of newly created CV
		CVTableLock->Release();
	#endif
	
	return x;		// Returns said value to Thread to use new CV
}

void DestroyCV_Syscall(int n){		// Destroys an existing CV in CVTable
	#ifdef NETWORK
		clientPacket packet;
		packet.syscall = SC_DestroyCV;
		//strncpy(packet.name, name, sizeof(name));
		
		SendToPO("DESTROYCV", packet);
		int x = ReceiveFromPO("DESTROYCV");
		
		if (x == -1){
			printf("DESTROYCV ERROR\n");
			interrupt->Halt();
		} else {
			printf("DESTROYCV SUCCESSFUL\n");
		}
	#else
		CVTableLock->Acquire();
	
		if (n < 0 || n > (signed)CVTable.size() - 1 || CVTable[n].CV == NULL ){		// Check if data exists for entered value
			printf("%s", "Invalid CV Table Number.\n");
			CVTableLock->Release();
			return;		// If CV doesn't exist (or is already deleted) return failure
		}
	
		if (!CVTable[n].CV->waitingCV->IsEmpty() && !CVTable[n].IsDeleted){		// If threads are waiting for CV and Delete isn't set
			CVTable[n].IsDeleted = true;		// Set the CV for deletion
			printf("%s", "CV has waiting Threads, setting Delete\n");
		} else if (CVTable[n].CV->waitingCV->IsEmpty() && !CVTable[n].IsDeleted){		// If no threads are waiting for CV and Delete isn't set
			CVTable[n].IsDeleted = true;		// Set the CV for deletion
			CVTable[n].CV = NULL;		// Delete the CV	
			printf("%s", "CV has no waiting Threads, setting Delete and Lock\n");
		} else if (CVTable[n].CV->waitingCV->IsEmpty() && CVTable[n].IsDeleted){		// If no threads are waiting for CV and Delete was set
			CVTable[n].CV = NULL;		// Delete the CV
			printf("%s", "CV has waiting Threads, now setting Lock to NULL\n");
		}
		CVTableLock->Release();
	#endif
}

void newProcess(int arg){		// Restores the thread
    currentThread->space->InitRegisters();
	
	currentThread->space->SaveState();
    currentThread->space->RestoreState();
    machine->Run();
        // We should never reach here.
}

int Exec_Syscall(unsigned int vaddr, unsigned int length) {		// Creates a new process
	char* filename = new char[MAXFILENAME];
	
	if(!filename) {		// Allocate space for the name
		printf("Failed to allocate filename buffer (size = %d)!\n", MAXFILENAME);
		delete[] filename;
		return -1;
	}
	if(copyin(vaddr, MAXFILENAME, filename) == -1) {		// Find the physical data from the kernel
		printf("Invalid pointer (vaddr = %d)!\n", vaddr);
		delete[] filename;
		return -1;
	}
	filename[length] = '\0';
	
	OpenFile* file = fileSystem->Open(filename);
	
	if(!file) {		// Open the file
		printf("File (filename = %s) could not be opened!\n", filename);
		delete[] filename;
		//delete file;
		return -1;
	}
	
	syscallLock->Acquire();
	
	Process* p = new Process();		// New process
	Thread* t = new Thread("Exec");		// New Thread
	t->space = new AddrSpace(file);		// The addrspace of the new thread is set to an empty space
	
	p->activeThreadCount = 1;		// Process has 1 thread
	p->inactiveThreadCount = 0;		// Process has no sleeping/inactive thread
	int processID = processTable->Put(p);		// Find the index of the new process in the processTable
	if(processID == -1) {		// If i failed to be placed into the processTable
		printf("Could not add process to Process Table!\n");
		delete[] filename;
		//delete file;
		delete p;
		return -1;		// Return failure
	}
	printf("Added new process with ID = %d\n", processID);
	p->id = processID;		// Sets the ID of the process to the Process ID
	t->setPID(p->id);		// Sets the process ID of the thread to the process ID
	t->setID(1);		// Sets ID of the thread to 1 (First thread of the new process)
	
	//delete file;		// Delete the used files
	delete[] filename;
	syscallLock->Release();		// Release the lock
	
	t->Fork((VoidFunctionPtr) newProcess, 0);		// Fork the new thread of the new process
	return processID;
}

int allocateForkSpace(){
	int i, nextPage;
	TranslationEntry *newPageTable = new TranslationEntry[currentThread->space->getNumPages() + 8];
	for (i = 0; i < currentThread->space->getNumPages(); i++){
		newPageTable[i].virtualPage = currentThread->space->pageTable[i].virtualPage;
		newPageTable[i].physicalPage = currentThread->space->pageTable[i].physicalPage;
		newPageTable[i].valid = currentThread->space->pageTable[i].valid;
		newPageTable[i].use = currentThread->space->pageTable[i].use;
		newPageTable[i].dirty = currentThread->space->pageTable[i].dirty;
		newPageTable[i].readOnly = currentThread->space->pageTable[i].readOnly;
	}
	for (i = currentThread->space->getNumPages(); i < currentThread->space->getNumPages() + 8; i++){
		newPageTable[i].virtualPage = i;		// for now, virtual page # = phys page #
		memMapLock -> Acquire();
		nextPage = memMap -> Find();
		if(nextPage == -1)
		{
			printf("\nNachos ran out of memory : It will now halt\n");
			interrupt -> Halt(); // Halt nachos and exit 
		}
		memMapLock -> Release();
		newPageTable[i].physicalPage = nextPage;
		newPageTable[i].valid = true;
		newPageTable[i].use = false;
		newPageTable[i].dirty = false;
		newPageTable[i].readOnly = false;
	}
	delete currentThread->space->pageTable;
	currentThread->space->pageTable = newPageTable;
	currentThread->space->setNumPages();
	currentThread->space->RestoreState();
	return currentThread->space->getNumPages();
}

void kernelRun(int vaddr) {		// Set up new thread
	machine->WriteRegister(PCReg, (unsigned int)vaddr);
	machine->WriteRegister(NextPCReg, (unsigned int)vaddr+4);
	
	int stackPosition = allocateForkSpace();
	printf("StackPosition = %d\n", stackPosition);
	machine->WriteRegister(StackReg, stackPosition * PageSize - 16);

	machine->Run();		// Run the thread
	ASSERT(false);
}

void Fork_Syscall(int funcAddr){		// Creates a new thread to run
    syscallLock->Acquire();
    printf("%s", "Forking Thread!\n");
    DEBUG('a', "%s: Called Fork_Syscall.\n",currentThread->getName());
    Thread *t = new Thread(currentThread->getName());		// new thread name is said as currentThread
	int processID = currentThread->getPID();		
	Process* p = (Process*) processTable->Get(processID);		// Find process that runs currentThread
	if(!p) {		// If no process exists, return error
		printf("Failed to fetch process %d from Process Table!\n", processID);
		return;
	}
	p->activeThreadCount++;		// Increment count of active threads for said process
	t->setID(p->inactiveThreadCount + p->activeThreadCount);		// ID of new thread is equal threads created by process
	t->setPID(processID);		// Set process ID of thread to process ID
	t->space = currentThread->space;		// Sets addrspace of thread to current thread addrspace
	
	printf("FORK THREAD ID IS %d\n", currentThread->getID());
	
	
    t->Fork((VoidFunctionPtr) kernelRun, funcAddr);		// Runs the new thread
    currentThread->space->RestoreState();		// Restores state of currentThread
    syscallLock->Release();
}

void Exit_Syscall(int code) {		// Ends threads and process'
	printf("EXITING WITH CODE = %d\n", code);
	printf("EXIT THREAD ID IS %d\n", currentThread->getID());
	
	syscallLock->Acquire();
	
	int processID = currentThread->getPID();		// Find processID of currentThread
	Process* p = (Process*)processTable->Get(processID);		// Find the process that owns this thread
	if(!p) {		// If no process, return error
		printf("Could not retrieve process %d from Process Table!\n", processID);
	}
	
	if (processTable->size > 1){		// If there are more than 1 process running
		if (p->activeThreadCount == 1){		// If this thread is the only active thread
			syscallLock->Release();
			processTable->Remove(processID);		// End the process
			currentThread->Finish();		// End currentThread
		}else {		// If there are multiple threads running
			printf("Thread Finishing!\n");
			p->activeThreadCount--;		// Decrement active thread count for the process
			p->inactiveThreadCount++;
			syscallLock->Release();
			currentThread->Finish();		// End currentThread
		}
	} else {		// If there is only one process left
		if (p->activeThreadCount == 1){
			processTable->Remove(processID);		// End the process
			printf("Ending Last Process!\n");
			interrupt->Halt();		// Kill the program
		}else {		// If there are multiple threads left
			printf("Thread Finishing!\n");
			p->activeThreadCount--;		// Decrement active threads
			p->inactiveThreadCount++;
			syscallLock->Release();	
			currentThread->Finish();		// End currentThread
		}
	}
}

void printf_Syscall(unsigned int vaddr, int len, int a, int b) {
    // Print string of length len to synchronized console including 
    // the integer arguments supplied in the args array. 
    PrintfLock->Acquire();
	char* c_buf;
    c_buf=new char[len];
    copyin(vaddr,len,c_buf);
	
	if (b >= 100){
		int x = b % 100;
		int y = (b / 100) - 1;
		printf(c_buf, a, x, y);
	}else if (b == -1){
		printf(c_buf, a);
	} else if (a == -1 && b == -1){
		printf(c_buf);
	}else {
		printf(c_buf, a, b);
	}

    delete c_buf;
    PrintfLock->Release();

}

#ifdef NETWORK

int CreateMV_Syscall(unsigned int vaddr, int initialValue, int index){		/*System call for creating a monitor variable*/
	char *name = new char[NameSize];
	
	if (copyin(vaddr, NameSize, name) == -1){
		DEBUG('a', "bad pointer passed to create lock\n");
		delete[] name;
		return -2;
	}
	
	stringstream ss;
	ss << name << index;
	string se = ss.str();
	char* s = new char[se.size() + 1];
	std::copy(se.begin(), se.end(), s);
	s[se.size()] = '\0';
	
	int id;
	syscallLock->Acquire();									
	clientPacket tempPacketSend;									/*Creating a packet on the client side*/
	strncpy(tempPacketSend.name,s,sizeof(s));
	tempPacketSend.syscall = SC_CreateMV;							
	tempPacketSend.value = initialValue;
	SendToPO("CREATEMV",tempPacketSend);		
	id = ReceiveFromPO("CREATEMV");
	if(id == -1){												/*Halts the program if the id received is -1*/
		printf("CREATEMV ERROR CREATING MV\n");
		interrupt->Halt();
	}
	printf("CREATEMV MV %d CREATED \n",id);					/*The MV is created if the program comes to this else part*/
	syscallLock->Release();
	return id;
}

void SetMV_Syscall(int id,int value){							/*System call for setting a monitor variable*/
	clientPacket tempPacketSend;									/*Creating a packet in the client side*/
	tempPacketSend.syscall = SC_SetMV;                            /*Assigning the required data in the packet*/
	tempPacketSend.index = id;                                 
	tempPacketSend.value = value;                                 
	SendToPO("SETMV",tempPacketSend);                                 /*calls the send function which will perform postoffice->send*/
	serverPacket receivedPacket = ReceivePacket("SETMV");    /*Receives the index and stores it in a variable*/
	if(receivedPacket.status == true)                           
		return;                                                 
		                                                        
	printf("SETMV ERROR SETTING MV\n");
	interrupt->Halt();                                          
}

int GetMV_Syscall(int id){									/*System call for getting a monitor variable*/
	clientPacket tempPacketSend;									/*Creating a packet in the client side*/
	tempPacketSend.syscall = SC_GetMV;                            /*Assigning the required datea in the packet*/
	tempPacketSend.index = id;                                 
	SendToPO("GETMV",tempPacketSend);                                 /*calls the send function which will perform postoffice->send*/
	serverPacket receivedPacket = ReceivePacket("GETMV");    /*Receives the index and stores it in a variable*/
	if(receivedPacket.status == true)                           
		return receivedPacket.value;
	printf("SETMV ERROR GETTING MV\n");
	interrupt->Halt();
}

void DestroyMV_Syscall(int id){
	clientPacket tempPacketSend;									/*Creating a packet in the client side*/
	tempPacketSend.syscall = SC_DestroyMV;                        /*Assigning the required data in the packet*/
	tempPacketSend.index = id;                                 
	SendToPO("DESTROYMV",tempPacketSend);                             /*calls the send function which will perform postoffice->send*/
	id = ReceiveFromPO("DESTROYMV");                                  /*Receives the index and stores it in a variable*/
	
	if(id == -1){
		printf("DESTROYMV ERROR DESTROYING MV\n");
		interrupt->Halt();
	}
	else 
		printf("MV DESTROYED\n");
		return;
}

#endif

void ExceptionHandler(ExceptionType which) {
    int type = machine->ReadRegister(2); // Which syscall?
    int rv=0; 	// the return value from a syscall

    if ( which == SyscallException ) {
		switch (type) {
			default:
				DEBUG('a', "Unknown syscall - shutting down.\n");
				break;
			case SC_Halt:
				DEBUG('a', "Shutdown, initiated by user program.\n");
				interrupt->Halt();
				break;
			case SC_Create:
				DEBUG('a', "Create syscall.\n");
				Create_Syscall(machine->ReadRegister(4), machine->ReadRegister(5));
				break;
			case SC_Open:
				DEBUG('a', "Open syscall.\n");
				rv = Open_Syscall(machine->ReadRegister(4), machine->ReadRegister(5));
				break;
			case SC_Write:
				DEBUG('a', "Write syscall.\n");
				Write_Syscall(machine->ReadRegister(4),
				  machine->ReadRegister(5),
				  machine->ReadRegister(6));
				break;
			case SC_Read:
				DEBUG('a', "Read syscall.\n");
				rv = Read_Syscall(machine->ReadRegister(4),
				  machine->ReadRegister(5),
				  machine->ReadRegister(6));
				break;
			case SC_Close:
				DEBUG('a', "Close syscall.\n");
				Close_Syscall(machine->ReadRegister(4));
				break;
			case SC_Exec:
				DEBUG('a', "Exec syscall.\n");
				rv = Exec_Syscall(machine->ReadRegister(4), machine->ReadRegister(5));
				break;
			case SC_Fork:
				DEBUG('a', "Fork syscall.\n");
				Fork_Syscall(machine->ReadRegister(4));
				break;
			case SC_Exit:
				Exit_Syscall(machine->ReadRegister(4));
				break;
			case SC_Yield:						// Causes the current Thread to yield
				DEBUG('a', "Yield Thread.\n");
				currentThread->Yield();
				break;
			case SC_Acquire:		// Syscall for Acquire
				DEBUG('a', "AcquireKernel_Lock Syscall.\n");
				Acquire_Syscall(machine->ReadRegister(4));		// Calls Acquire_Syscall with entered parameter (int)
				break;
			case SC_Release:		// Syscall for Release
				DEBUG('a', "ReleaseKernel_Lock Syscall.\n");
				Release_Syscall(machine->ReadRegister(4));		// Call Release_Syscall with entered parameter (int)
				break;
			case SC_Wait:		// Syscall for Wait
				DEBUG('a', "Wait for CV Syscall.\n");
				Wait_Syscall(machine->ReadRegister(4),		
							 machine->ReadRegister(5));		// Calls Wait_Syscall with entered parameter (int, int)
				break;
			case SC_Signal:		// Syscall for Signal
				DEBUG('a', "Signal for CV Syscall.\n");
				Signal_Syscall(machine->ReadRegister(4), machine->ReadRegister(5));		// Calls Signal_Syscall with entered parameters (int, int)
				break;
			case SC_Broadcast:		// Syscall for Broadcast
				DEBUG('a', "Broadcast for CV Syscall.\n");
				Broadcast_Syscall(machine->ReadRegister(4),
								  machine->ReadRegister(5));		// Calls Broadcast_Syscall with entered parameters (int, int)
				break;
			case SC_CreateLock:		// Syscall for creating locks
				DEBUG('a', "Create Lock Syscall.\n");
				rv = CreateLock_Syscall(machine->ReadRegister(4), machine->ReadRegister(5));		// Calls CreateLock_Syscall with no parameters
				break;
			case SC_DestroyLock:		// Syscall for deleting locks
				DEBUG('a', "Destroy Lock Syscall.\n");
				DestroyLock_Syscall(machine->ReadRegister(4));		// Calls DestroyLock_Syscall with entered parameters (int)
				break;
			case SC_CreateCV:		// Syscall for creating CVs
				DEBUG('a', "Create CV Syscall.\n");
				rv = CreateCV_Syscall(machine->ReadRegister(4), machine->ReadRegister(5));		// Calls CreateCV_Syscall with entered parameters (int)
				break;
			case SC_DestroyCV:		// Syscall for destroying CVs
				DEBUG('a', "Destroy CV Syscall.\n");
				DestroyCV_Syscall(machine->ReadRegister(4));		// Calls DestroyCV_Syscall with entered parameters (int)
				break;
			case SC_printf:
				DEBUG('a', "printf Syscall.\n");
				printf_Syscall(machine->ReadRegister(4), machine->ReadRegister(5), machine->ReadRegister(6), machine->ReadRegister(7));
				break;
			case SC_rand:
				DEBUG('a', "rand Syscall.\n");
				rv = rand_Syscall();
				break;
				
			#ifdef NETWORK
			case SC_CreateMV:
				DEBUG('a', "CreateMV Syscall.\n");
				rv = CreateMV_Syscall(machine->ReadRegister(4), machine->ReadRegister(5), machine->ReadRegister(6));
				break;
			case SC_SetMV:
				DEBUG('a', "SetMV Syscall.\n");
				SetMV_Syscall(machine->ReadRegister(4), machine->ReadRegister(5));
				break;
			case SC_GetMV:
				DEBUG('a', "GetMV Syscall.\n");
				rv = GetMV_Syscall(machine->ReadRegister(4));
				break;
			case SC_DestroyMV:
				DEBUG('a', "DestroyMV Syscall.\n");
				DestroyMV_Syscall(machine->ReadRegister(4));
				break;
			#endif
		}		
		// Put in the return value and increment the PC
		machine->WriteRegister(2,rv);
		machine->WriteRegister(PrevPCReg,machine->ReadRegister(PCReg));
		machine->WriteRegister(PCReg,machine->ReadRegister(NextPCReg));
		machine->WriteRegister(NextPCReg,machine->ReadRegister(PCReg)+4);
		return;
	} else {
      cout<<"Unexpected user mode exception - which:"<<which<<"  type:"<< type<<endl;
      interrupt->Halt();
    }
}
