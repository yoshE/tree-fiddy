// system.h 
//	All global variables used in Nachos are defined here.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#ifndef SYSTEM_H
#define SYSTEM_H

#include "copyright.h"
#include "utility.h"
#include "thread.h"
#include "scheduler.h"
#include "../machine/interrupt.h"
#include "../machine/stats.h"
#include "../machine/timer.h"
#include "synch.h"
#include "../userprog/addrspace.h"
#include "../userprog/table.h"
#include "../userprog/bitmap.h"

#define PROCESS_TABLE_MAX_SIZE	32

struct Process {		// struct to hold thread count for processes
	int id;		// ID of the process
	int activeThreadCount;		// Number of threads currently running
	int inactiveThreadCount;
};

struct KernelLock{		// struct to hold locks for the lock table
	Lock* Kernel_Lock;		// Actual Lock
	Thread *Owner;		// Owner of the Lock
	bool IsDeleted;		// If the lock is slated for deletion
};

struct KernelCV{		// struct to hold CVs for the CV table
	Condition* CV;
	bool IsDeleted;
};

// Initialization and cleanup routines
extern void Initialize(int argc, char **argv); 	// Initialization,
						// called before anything else
extern void Cleanup();				// Cleanup, called when
						// Nachos is done.

extern Thread *currentThread;			// the thread holding the CPU
extern Thread *threadToBeDestroyed;  		// the thread that just finished
extern Scheduler *scheduler;			// the ready list
extern Interrupt *interrupt;			// interrupt status
extern Statistics *stats;			// performance metrics
extern Timer *timer;				// the hardware alarm clock

extern Table *processTable;		// Table of processes
extern BitMap *memMap;		// BitMap of available pages for user programs
extern BitMap *memory;			// BitMap for representing availability of memory

extern List *evictQueue;

#ifdef USER_PROGRAM
#include "machine.h"
extern Machine* machine;	// user program memory and registers
#endif

#ifdef FILESYS_NEEDED 		// FILESYS or FILESYS_STUB 
#include "filesys.h"
extern FileSystem  *fileSystem;
#endif

#ifdef FILESYS
#include "synchdisk.h"
extern SynchDisk   *synchDisk;
#endif

#ifdef NETWORK
#include "post.h"
extern PostOffice* postOffice;
#endif

#endif // SYSTEM_H
