// addrspace.cc 
//	Routines to manage address spaces (executing user programs).
//
//	In order to run a user program, you must:
//
//	1. link with the -N -T 0 option 
//	2. run coff2noff to convert the object file to Nachos format
//		(Nachos object code format is essentially just a simpler
//		version of the UNIX executable object code format)
//	3. load the NOFF file into the Nachos file system
//		(if you haven't implemented the file system yet, you
//		don't need to do this last step)
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "../threads/system.h"
#include "addrspace.h"
#include "noff.h"
#include "table.h"
#include "synch.h"

extern BitMap *memMap;
extern Lock *iptLock;	//IPT lock

extern "C" { int bzero(char *, int); };

class IPT:public TranslationEntry {
    public:
        AddrSpace *space;// addrspace class pointer to identify process
		
	IPT(){}
};

IPT *ipt = new IPT[NumPhysPages]; //ipt is created.

Table::Table(int s) : map(s), table(0), lock(0), size(s) {
    table = new void *[size];
    lock = new Lock("TableLock");
}

Table::~Table() {
    if (table) {
	delete table;
	table = 0;
    }
    if (lock) {
	delete lock;
	lock = 0;
    }
}

void *Table::Get(int i) {
    // Return the element associated with the given if, or 0 if
    // there is none.

    return (i >=0 && i < size && map.Test(i)) ? table[i] : 0;
}

int Table::Put(void *f) {
    // Put the element in the table and return the slot it used.  Use a
    // lock so 2 files don't get the same space.
    int i;	// to find the next slot

    lock->Acquire();
    i = map.Find();
    lock->Release();
    if ( i != -1)
	table[i] = f;
    return i;
}

void *Table::Remove(int i) {
    // Remove the element associated with identifier i from the table,
    // and return it.

    void *f =0;

    if ( i >= 0 && i < size ) {
	lock->Acquire();
	if ( map.Test(i) ) {
	    map.Clear(i);
	    f = table[i];
	    table[i] = 0;
	}
	lock->Release();
    }
    return f;
}

//----------------------------------------------------------------------
// SwapHeader
// 	Do little endian to big endian conversion on the bytes in the 
//	object file header, in case the file was generated on a little
//	endian machine, and we're now running on a big endian machine.
//----------------------------------------------------------------------

static void 
SwapHeader (NoffHeader *noffH)
{
	noffH->noffMagic = WordToHost(noffH->noffMagic);
	noffH->code.size = WordToHost(noffH->code.size);
	noffH->code.virtualAddr = WordToHost(noffH->code.virtualAddr);
	noffH->code.inFileAddr = WordToHost(noffH->code.inFileAddr);
	noffH->initData.size = WordToHost(noffH->initData.size);
	noffH->initData.virtualAddr = WordToHost(noffH->initData.virtualAddr);
	noffH->initData.inFileAddr = WordToHost(noffH->initData.inFileAddr);
	noffH->uninitData.size = WordToHost(noffH->uninitData.size);
	noffH->uninitData.virtualAddr = WordToHost(noffH->uninitData.virtualAddr);
	noffH->uninitData.inFileAddr = WordToHost(noffH->uninitData.inFileAddr);
}

//----------------------------------------------------------------------
// AddrSpace::AddrSpace
// 	Create an address space to run a user program.
//	Load the program from a file "executable", and set everything
//	up so that we can start executing user instructions.
//
//	Assumes that the object code file is in NOFF format.
//
//	"executable" is the file containing the object code to load into memory
//
//      It's possible to fail to fully construct the address space for
//      several reasons, including being unable to allocate memory,
//      and being unable to read key parts of the executable.
//      Incompletely consretucted address spaces have the member
//      constructed set to false.
//----------------------------------------------------------------------

AddrSpace::AddrSpace(OpenFile *executable) : fileTable(MaxOpenFiles) {
    NoffHeader noffH;
    unsigned int i, size;

    // Don't allocate the input or output to disk files
    fileTable.Put(0);
    fileTable.Put(0);

    executable->ReadAt((char *)&noffH, sizeof(noffH), 0);
    if ((noffH.noffMagic != NOFFMAGIC) && 
		(WordToHost(noffH.noffMagic) == NOFFMAGIC))
    	SwapHeader(&noffH);
    ASSERT(noffH.noffMagic == NOFFMAGIC);

    size = noffH.code.size + noffH.initData.size + noffH.uninitData.size ;
    numPages = divRoundUp(size, PageSize) + divRoundUp(UserStackSize,PageSize);
                                                // we need to increase the size
						// to leave room for the stack
    size = numPages * PageSize;

    ASSERT(numPages <= NumPhysPages);		// check we're not trying
						// to run anything too big --
						// at least until we have
						// virtual memory

    DEBUG('a', "Initializing address space, num pages %d, size %d\n", 
					numPages, size);
// first, set up the translation 
    pageTable = new TranslationEntry[numPages];
    for (i = 0; i < numPages; i++) {
	pageTable[i].virtualPage = i;		// for now, virtual page # = phys page #
		int ppn = memMap->Find();		// Sets ppn to first available page in memMap
		if ( ppn < 0 ) {		// If there are no spaces in memMap, then error!
			printf("Physical Pages too small!\n");
			interrupt->Halt();
		}
	pageTable[i].physicalPage = ppn;		// Set page to be used to ppn
	pageTable[i].valid = TRUE;
	pageTable[i].use = FALSE;
	pageTable[i].dirty = FALSE;
	pageTable[i].readOnly = FALSE;  // if the code segment was entirely on 
					// a separate page, we could set its 
					// pages to be read-only
     executable->ReadAt(&(machine->mainMemory[ppn*PageSize]),
			PageSize, noffH.code.inFileAddr + i * PageSize);
      
   }
}

//----------------------------------------------------------------------
// AddrSpace::~AddrSpace
//
// 	Dealloate an address space.  release pages, page tables, files
// 	and file tables
//----------------------------------------------------------------------

AddrSpace::~AddrSpace()
{
    delete pageTable;
}

//----------------------------------------------------------------------
// AddrSpace::PopulateTLB
// 	populate the TLB from pageTable
//----------------------------------------------------------------------

int currentTLBIndex = 0;
void AddrSpace::PopulateTLB(int n){
	IntStatus oldLevel = interrupt->SetLevel(IntOff);
	currentTLBIndex = (currentTLBIndex + 1) % TLBSize;
	
	machine->tlb[currentTLBIndex].virtualPage = pageTable[n].virtualPage;
	machine->tlb[currentTLBIndex].virtualPage = pageTable[n].virtualPage;
	machine->tlb[currentTLBIndex].physicalPage = pageTable[n].physicalPage;
	machine->tlb[currentTLBIndex].valid = pageTable[n].valid;
	machine->tlb[currentTLBIndex].use = pageTable[n].use;
	machine->tlb[currentTLBIndex].dirty = pageTable[n].dirty;
	machine->tlb[currentTLBIndex].readOnly = pageTable[n].readOnly;
	interrupt->SetLevel(oldLevel);
}

void handleIPTMiss(int currentVPN) {
	memoryLock->Acquire();
	int pageIndex = memory->Find(currentVPN);
	memoryLock->Release();
	
	// Assuming page is available
	TranslationEntry** table = currentThread->space->pageTable;
	
	table[currentVPN].physicalPage = pageIndex;
	table[currentVPN].valid = true;
	
	if(false) {		// todo: if requires executable
		executable->ReadAt(&(machine->mainMemory[pageIndex*PageSize]), PageSize, table[currentVPN].byteOffset);
		ipt[pageIndex].dirty = false;
	} else {
		iptLock->Acquire();
		ipt[pageIndex].physicalPage = pageIndex;
		ipt[pageIndex].virtualPage = currentVPN;
		ipt[pageIndex].use = true;
		ipt[pageIndex].valid = true;
		ipt[pageIndex].readOnly = table[currentVPN].readOnly;
		ipt[pageIndex].space = currentThread->space;
		iptLock->Release();
	}
	
	return pageIndex;
}

//----------------------------------------------------------------------
// AddrSpace::PopulateTLB_IPT
// 	populate the TLB from IPT
//----------------------------------------------------------------------

bool PopulateTLBFromIPT(int currentVPN){
	int i=0; 
	bool foundInIPT = false;
	int physicalPage = -1;

	iptLock -> Acquire();		// Acquire IPT lock
	for(i=0; i<NumPhysPages; i++){		// Search IPT if required vpn entry is in page table or not
		// Check if there is a valid, currently unused, and same process entry required vpn in IPT
		if( ipt[i].virtualPage == currentVPN && ipt[i].valid == TRUE && ipt[i].use == FALSE && ipt[i].space == currentThread->space){
			physicalPage = i;			// If it exists, that ppn is used
			ipt[i].use = TRUE; 			// Use bit is set to true
			foundInIPT = true;
			break;
		}
	}
	iptLock -> Release(); 

	// If there is a IPT miss
	if(physicalPage == -1){
		physicalPage = currentThread->space->handleIPTMiss(currentVPN);		// Handle IPT miss NEED TO IMPLEMENT
	} else {		// Populate TLB from IPT
		foundInIPT = true;
		IntStatus oldLevel = interrupt->SetLevel(IntOff);		// Turn off interrupts
		currentTLBIndex = (currentTLBIndex + 1)% TLBSize;		// Current index is moved to next location

		if(machine->tlb[currentTLBIndex].valid == TRUE){		// If TLB index is valid
			ipt[machine->tlb[currentTLBIndex].physicalPage].dirty = machine->tlb[currentTLBIndex].dirty;		//Propagate Dirty Bit
		}

		machine->tlb[currentTLBIndex].virtualPage = ipt[physicalPage].virtualPage;
		machine->tlb[currentTLBIndex].physicalPage = ipt[physicalPage].physicalPage;
		machine->tlb[currentTLBIndex].valid = ipt[physicalPage].valid;
		machine->tlb[currentTLBIndex].use = FALSE;
		machine->tlb[currentTLBIndex].dirty = ipt[physicalPage].dirty;
		machine->tlb[currentTLBIndex].readOnly = ipt[physicalPage].readOnly;
		interrupt->SetLevel(oldLevel);		// Turn Interrupts back on
	}
	
	iptLock -> Acquire();
	ipt[physicalPage].use = FALSE;
	iptLock -> Release(); 
	return foundInIPT;
}

//----------------------------------------------------------------------
// AddrSpace::InitRegisters
// 	Set the initial values for the user-level register set.
//
// 	We write these directly into the "machine" registers, so
//	that we can immediately jump to user code.  Note that these
//	will be saved/restored into the currentThread->userRegisters
//	when this thread is context switched out.
//----------------------------------------------------------------------

void
AddrSpace::InitRegisters()
{
    int i;

    for (i = 0; i < NumTotalRegs; i++)
	machine->WriteRegister(i, 0);

    // Initial program counter -- must be location of "Start"
    machine->WriteRegister(PCReg, 0);	

    // Need to also tell MIPS where next instruction is, because
    // of branch delay possibility
    machine->WriteRegister(NextPCReg, 4);

   // Set the stack register to the end of the address space, where we
   // allocated the stack; but subtract off a bit, to make sure we don't
   // accidentally reference off the end!
    machine->WriteRegister(StackReg, numPages * PageSize - 16);
    DEBUG('a', "Initializing stack register to %x\n", numPages * PageSize - 16);
}

//----------------------------------------------------------------------
// AddrSpace::SaveState
// 	On a context switch, save any machine state, specific
//	to this address space, that needs saving.
//
//	For now, nothing!
//----------------------------------------------------------------------

void AddrSpace::SaveState() 
{}

//----------------------------------------------------------------------
// AddrSpace::RestoreState
// 	On a context switch, restore the machine state so that
//	this address space can run.
//
//      For now, tell the machine where to find the page table.
//----------------------------------------------------------------------

void AddrSpace::RestoreState() 
{
    machine->pageTable = pageTable;
    machine->pageTableSize = numPages;
	
	int i = 0;
	IntStatus oldLevel = interrupt->SetLevel(IntOff);
	for (i = 0; i < TLBSize; i++){
		machine->tlb[i].valid = false;
	}
	interrupt->SetLevel(oldLevel);
}
