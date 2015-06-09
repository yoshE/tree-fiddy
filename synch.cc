// synch.cc 
//	Routines for synchronizing threads.  Three kinds of
//	synchronization routines are defined here: semaphores, locks 
//   	and condition variables (the implementation of the last two
//	are left to the reader).
//
// Any implementation of a synchronization routine needs some
// primitive atomic operation.  We assume Nachos is running on
// a uniprocessor, and thus atomicity can be provided by
// turning off interrupts.  While interrupts are disabled, no
// context switch can occur, and thus the current thread is guaranteed
// to hold the CPU throughout, until interrupts are reenabled.
//
// Because some of these routines might be called with interrupts
// already disabled (Semaphore::V for one), instead of turning
// on interrupts at the end of the atomic operation, we always simply
// re-set the interrupt state back to its original value (whether
// that be disabled or enabled).
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "synch.h"
#include "system.h"
#include <iostream>

//----------------------------------------------------------------------
// Semaphore::Semaphore
// 	Initialize a semaphore, so that it can be used for synchronization.
//
//	"debugName" is an arbitrary name, useful for debugging.
//	"initialValue" is the initial value of the semaphore.
//----------------------------------------------------------------------

Semaphore::Semaphore(char* debugName, int initialValue)
{
    name = debugName;
    value = initialValue;
    queue = new List;
}

//----------------------------------------------------------------------
// Semaphore::Semaphore
// 	De-allocate semaphore, when no longer needed.  Assume no one
//	is still waiting on the semaphore!
//----------------------------------------------------------------------

Semaphore::~Semaphore()
{
    delete queue;
}

//----------------------------------------------------------------------
// Semaphore::P
// 	Wait until semaphore value > 0, then decrement.  Checking the
//	value and decrementing must be done atomically, so we
//	need to disable interrupts before checking the value.
//
//	Note that Thread::Sleep assumes that interrupts are disabled
//	when it is called.
//----------------------------------------------------------------------
        
void
Semaphore::P()
{
    IntStatus oldLevel = interrupt->SetLevel(IntOff);	// disable interrupts
    
    while (value == 0) { 			// semaphore not available
	queue->Append((void *)currentThread);	// so go to sleep
	currentThread->Sleep();
    } 
    value--; 					// semaphore available, 
						// consume its value
    
    (void) interrupt->SetLevel(oldLevel);	// re-enable interrupts
}

//----------------------------------------------------------------------
// Semaphore::V
// 	Increment semaphore value, waking up a waiter if necessary.
//	As with P(), this operation must be atomic, so we need to disable
//	interrupts.  Scheduler::ReadyToRun() assumes that threads
//	are disabled when it is called.
//----------------------------------------------------------------------

void
Semaphore::V()
{
    Thread *thread;
    IntStatus oldLevel = interrupt->SetLevel(IntOff);

    thread = (Thread *)queue->Remove();
    if (thread != NULL)	   // make thread ready, consuming the V immediately
	scheduler->ReadyToRun(thread);
    value++;
    (void) interrupt->SetLevel(oldLevel);
}

// Dummy functions -- so we can compile our later assignments 
// Note -- without a correct implementation of Condition::Wait(), 
// the test case in the network assignment won't work!

Lock::Lock(char* debugName) {
  name = debugName;
  available = true; // Sets the lock to FREE
  waitingThreads = new List; // Create waiting list for threads
}

Lock::~Lock() {
  delete waitingThreads;
}

// Acquire() - Turns a FREE lock to BUSY, if unavailable then adds thread to
//		waiting queue
void Lock::Acquire() {
  IntStatus inter = interrupt->SetLevel(IntOff); // Disable Interrupts
  if (isHeldByCurrentThread()){ // See if thread already owns this Lock
    interrupt->SetLevel(inter);
    return;
  }
  if (available){ // If Lock is FREE
    available = false; // Make Lock BUSY and...
    owner = currentThread; // Assign this thread as the owner!
  }else{
    waitingThreads-> Append((void *)currentThread); // Add Thread to waiting List
    currentThread->Sleep(); // Put thread to sleep while it waits for Lock to FREE up
  }
  interrupt->SetLevel(inter);
}

// Release() - Turns the lock to FREE and then wakes up a thread if there
//		are any in queue waiting
void Lock::Release() {
  Thread *thread;
  IntStatus inter = interrupt->SetLevel(IntOff);
  if(!isHeldByCurrentThread()){ // Do you own this lock?
	std::cout << "You already own this lock!\n";
	interrupt->SetLevel(inter);
	return;
  }
  if(!waitingThreads->IsEmpty()){ // If waiting List is not empty
	thread = (Thread *)waitingThreads->Remove(); // Remove first thread and...
	if (thread != NULL) scheduler->ReadyToRun(thread); // Wake it up!
	owner = thread; // Then make that thread this thread's owner
  }else{ // If the waiting is empty, then FREE up!
	available = true;
	owner = NULL;
  }
}

bool Lock::isHeldByCurrentThread(){ // Quick check to see if thread owns the lock
  if(owner == currentThread) return true;
  return false;
}

Condition::Condition(char* debugName) { 
  name = debugName;
  waitingLock = NULL; // Creates a new Lock
  waitingCV = new List;
}

/*Condition::Condition(){
	waitingLock = NULL; // Creates a new Lock
	waitingCV = new List;
}*/

Condition::~Condition() { 
  delete waitingCV; // Deletes the List
}

//  Wait() -- release the lock, relinquish the CPU until signalled, 
//		then re-acquire the lock
void Condition::Wait(Lock* conditionLock) { 
  IntStatus inter = interrupt->SetLevel(IntOff);
  if(conditionLock == NULL){ // If input is null, end sequence
    std::cout << "Your lock is null!\n";
    interrupt->SetLevel(inter);
    return;
  }
  if(waitingLock == NULL){ // If this CV's lock hasn't been set yet
	  waitingLock = conditionLock; // Set the input lock as the CV lock
  }
  conditionLock->Release(); // FREE up the lock
  waitingCV-> Append((void *)currentThread); // Add a thread to waiting List
  conditionLock->Acquire(); // BUSY the lock
  interrupt->SetLevel(inter);
  return;
}

//  Signal() -- wake up a thread, if there are any waiting on 
//		the condition
void Condition::Signal(Lock* conditionLock) { 
  Thread *thread;
  IntStatus inter = interrupt->SetLevel(IntOff);
  if(waitingCV->IsEmpty()){ // If waiting list is empty, then end sequence
    interrupt->SetLevel(inter);
    return;
  }
  thread = (Thread *)waitingCV->Remove(); // Otherwise remove a thread
  if (thread != NULL) scheduler->ReadyToRun(thread); // Wake it up
  if (waitingCV->IsEmpty()) waitingLock = NULL; // If list is empty FREE up lock
  interrupt->SetLevel(inter);
}

//  Broadcast() -- wake up all threads waiting on the condition
void Condition::Broadcast(Lock* conditionLock) { 
  while(!waitingCV->IsEmpty()){ // Cycle through and wake up all threads waiting one by one
	Signal(conditionLock);
  }
}
