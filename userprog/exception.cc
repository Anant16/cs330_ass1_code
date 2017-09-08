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
#include "console.h"
#include "synch.h"

//----------------------------------------------------------------------
// ExceptionHandler
// 	Entry point into the Nachos kernel.  Called when a user program
//	is executing, and either does a syscall, or generates an addressing
//	or arithmetic exception.
//
// 	For system calls, the following is the calling convention:
//
// 	system call code -- r2
//		arg1 -- r4
//		arg2 -- r5
//		arg3 -- r6
//		arg4 -- r7
//
//	The result of the system call, if any, must be put back into r2. 
//
// And don't forget to increment the pc before returning. (Or else you'll
// loop making the same system call forever!
//
//	"which" is the kind of exception.  The list of possible exceptions 
//	are in machine.h.
//----------------------------------------------------------------------
static Semaphore *readAvail;
static Semaphore *writeDone;
static void ReadAvail(int arg) { readAvail->V(); }
static void WriteDone(int arg) { writeDone->V(); }

static void ConvertIntToHex (unsigned v, Console *console)
{
   unsigned x;
   if (v == 0) return;
   ConvertIntToHex (v/16, console);
   x = v % 16;
   if (x < 10) {
      writeDone->P() ;
      console->PutChar('0'+x);
   }
   else {
      writeDone->P() ;
      console->PutChar('a'+x-10);
   }
}

void forknew(int arg){

    DEBUG('t', "Now in thread \"%s\"\n", currentThread->getName());

        // If the old thread gave up the processor because it was finishing,
            // we need to delete its carcass.  Note we cannot delete the thread 
            // before now (for example, in NachOSThread::FinishThread()), because up to this
           // point, we were still running on the old thread's st
    if (threadToBeDestroyed != NULL) {
        delete threadToBeDestroyed;
        threadToBeDestroyed = NULL;
    }
                                            
       #ifdef USER_PROGRAM
     if (currentThread->space != NULL) {     // if there is an address space
            currentThread->RestoreUserState();     // to restore, do it.
            currentThread->space->RestoreContextOnSwitch();
    }
         machine->Run();
        #endif
}

void
ExceptionHandler(ExceptionType which)
{
    int type = machine->ReadRegister(2);
    int memval, vaddr, printval, tempval, exp, regno, exitcode;
    TranslationEntry *entry;
    OpenFile *executable;
    ProcessAddressSpace *space;
    unsigned int vpgno, offset, pageFrame, tpid, tppid, cpid;
    unsigned printvalus;        // Used for printing in hex
    if (!initializedConsoleSemaphores) {
       readAvail = new Semaphore("read avail", 0);
       writeDone = new Semaphore("write done", 1);
       initializedConsoleSemaphores = true;
    }
    Console *console = new Console(NULL, NULL, ReadAvail, WriteDone, 0);;

    if ((which == SyscallException) && (type == SysCall_Halt)) {
	DEBUG('a', "Shutdown, initiated by user program.\n");
   	interrupt->Halt();
    }
    else if ((which == SyscallException) && (type == SysCall_PrintInt)) {
       printval = machine->ReadRegister(4);
       if (printval == 0) {
	  writeDone->P() ;
          console->PutChar('0');
       }
       else {
          if (printval < 0) {
	     writeDone->P() ;
             console->PutChar('-');
             printval = -printval;
          }
          tempval = printval;
          exp=1;
          while (tempval != 0) {
             tempval = tempval/10;
             exp = exp*10;
          }
          exp = exp/10;
          while (exp > 0) {
	     writeDone->P() ;
             console->PutChar('0'+(printval/exp));
             printval = printval % exp;
             exp = exp/10;
          }
       }
       // Advance program counters.
       machine->WriteRegister(PrevPCReg, machine->ReadRegister(PCReg));
       machine->WriteRegister(PCReg, machine->ReadRegister(NextPCReg));
       machine->WriteRegister(NextPCReg, machine->ReadRegister(NextPCReg)+4);
    }
    else if ((which == SyscallException) && (type == SysCall_PrintChar)) {
	writeDone->P() ;
        console->PutChar(machine->ReadRegister(4));   // echo it!
       // Advance program counters.
       machine->WriteRegister(PrevPCReg, machine->ReadRegister(PCReg));
       machine->WriteRegister(PCReg, machine->ReadRegister(NextPCReg));
       machine->WriteRegister(NextPCReg, machine->ReadRegister(NextPCReg)+4);
    }
    else if ((which == SyscallException) && (type == SysCall_PrintString)) {
       vaddr = machine->ReadRegister(4);
       machine->ReadMem(vaddr, 1, &memval);
       while ((*(char*)&memval) != '\0') {
	  writeDone->P() ;
          console->PutChar(*(char*)&memval);
          vaddr++;
          machine->ReadMem(vaddr, 1, &memval);
       }
       // Advance program counters.
       machine->WriteRegister(PrevPCReg, machine->ReadRegister(PCReg));
       machine->WriteRegister(PCReg, machine->ReadRegister(NextPCReg));
       machine->WriteRegister(NextPCReg, machine->ReadRegister(NextPCReg)+4);
    }
    else if ((which == SyscallException) && (type == SysCall_PrintIntHex)) {
       printvalus = (unsigned)machine->ReadRegister(4);
       writeDone->P() ;
       console->PutChar('0');
       writeDone->P() ;
       console->PutChar('x');
       if (printvalus == 0) {
          writeDone->P() ;
          console->PutChar('0');
       }
       else {
          ConvertIntToHex (printvalus, console);
       }
       // Advance program counters.
       machine->WriteRegister(PrevPCReg, machine->ReadRegister(PCReg));
       machine->WriteRegister(PCReg, machine->ReadRegister(NextPCReg));
       machine->WriteRegister(NextPCReg, machine->ReadRegister(NextPCReg)+4);
    }
    else if ((which == SyscallException) && (type == SysCall_GetReg)) {

	regno = machine->ReadRegister(4);
	machine->WriteRegister(2, machine->ReadRegister(regno));
       
       // Advance program counters.
       machine->WriteRegister(PrevPCReg, machine->ReadRegister(PCReg));
       machine->WriteRegister(PCReg, machine->ReadRegister(NextPCReg));
       machine->WriteRegister(NextPCReg, machine->ReadRegister(NextPCReg)+4);
	
    }
    else if ((which == SyscallException) && (type == SysCall_GetPA)) {
	    vaddr = machine->ReadRegister(4);
    	vpgno = (unsigned) vaddr / PageSize;
        offset = (unsigned) vaddr % PageSize;
	    // check error conditions
        if(vpgno < machine->pageTableSize) { //pageTableSize maybe is incorrect here
            entry = &(machine->KernelPageTable[vpgno]);
            pageFrame = entry->physicalPage;
            if( (entry->valid) && (pageFrame < NumPhysPages) ) {
			    machine->WriteRegister(2, pageFrame * PageSize + offset);
	    	}
	    	else{
	    		machine->WriteRegister(2, -1);
	    	}
    	}
    	else{
    		machine->WriteRegister(2, -1);
    	}
       // Advance program counters.
       machine->WriteRegister(PrevPCReg, machine->ReadRegister(PCReg));
       machine->WriteRegister(PCReg, machine->ReadRegister(NextPCReg));
       machine->WriteRegister(NextPCReg, machine->ReadRegister(NextPCReg)+4);

    }
    else if ((which == SyscallException) && (type == SysCall_GetPID)) {
        tpid = (unsigned) currentThread->getpid();
        machine->WriteRegister(2, tpid);
       
       // Advance program counters.
       machine->WriteRegister(PrevPCReg, machine->ReadRegister(PCReg));
       machine->WriteRegister(PCReg, machine->ReadRegister(NextPCReg));
       machine->WriteRegister(NextPCReg, machine->ReadRegister(NextPCReg)+4);

    }
    else if ((which == SyscallException) && (type == SysCall_GetPPID)) {
        tppid = (unsigned) currentThread->getppid();
        machine->WriteRegister(2, tppid);
       
       // Advance program counters.
       machine->WriteRegister(PrevPCReg, machine->ReadRegister(PCReg));
       machine->WriteRegister(PCReg, machine->ReadRegister(NextPCReg));
       machine->WriteRegister(NextPCReg, machine->ReadRegister(NextPCReg)+4);

    }
    else if ((which == SyscallException) && (type == SysCall_Exec)) {
       DEBUG('a', "Anant - Before vaddr = machine->ReadRegister(4); \n");
       vaddr = machine->ReadRegister(4);
       char *filename = new char[128];
       int i=0;

       machine->ReadMem(vaddr, 1, &memval);
       while ((*(char*)&memval) != '\0') {
          filename[i] = (char)memval;
          vaddr++;
          ++i;
          machine->ReadMem(vaddr, 1, &memval);
       }
       filename[i] = (char)memval;


       executable = fileSystem->Open(filename);
       if(executable!=NULL){
            space = new ProcessAddressSpace(executable);
            if(currentThread->space != NULL)
                delete currentThread->space;

            currentThread->space = space;
            DEBUG('a', "Anant - Inside executable block \n");

            delete executable;

            space->InitUserModeCPURegisters();
            space->RestoreContextOnSwitch();

            machine->Run();
            ASSERT(FALSE);
       }
       else 
            printf("Unabel to open the given file: %s.\n", filename);
        machine->WriteRegister(PrevPCReg, machine->ReadRegister(PCReg));
        machine->WriteRegister(PCReg, machine->ReadRegister(NextPCReg));
        machine->WriteRegister(NextPCReg, machine->ReadRegister(NextPCReg)+4);

    }

  
    
    else if ((which == SyscallException) && (type == SysCall_NumInstr)) {
        machine->WriteRegister(2, currentThread->getNumOfInstructions());
       
        machine->WriteRegister(PrevPCReg, machine->ReadRegister(PCReg));
        machine->WriteRegister(PCReg, machine->ReadRegister(NextPCReg));
        machine->WriteRegister(NextPCReg, machine->ReadRegister(NextPCReg)+4);


    }

    else if ((which == SyscallException) && (type == SysCall_Fork)) {

        machine->WriteRegister(PrevPCReg, machine->ReadRegister(PCReg));
        machine->WriteRegister(PCReg, machine->ReadRegister(NextPCReg));
        machine->WriteRegister(NextPCReg, machine->ReadRegister(NextPCReg)+4);

        NachOSThread * childThread = new NachOSThread("Forked Child");
        childThread->space = new ProcessAddressSpace(currentThread->space);
        currentThread->insertChild(childThread);

        machine->WriteRegister(2, 0);
        childThread->SaveUserState();

        childThread->ThreadFork(&forknew, 0);

        machine->WriteRegister(2, childThread->getpid());

    }

    else if ((which == SyscallException) && (type == SysCall_Join)) {

    	cpid = machine->ReadRegister(4);
    	exitcode = currentThread->searchChild(cpid);  //also checks if it is valid child_pid or not
    	machine->WriteRegister(2, exitcode);

    	machine->WriteRegister(PrevPCReg, machine->ReadRegister(PCReg));
        machine->WriteRegister(PCReg, machine->ReadRegister(NextPCReg));
        machine->WriteRegister(NextPCReg, machine->ReadRegister(NextPCReg)+4);

    }

    else if ((which == SyscallException) && (type == SysCall_Exit)) {
        exitcode = machine->ReadRegister(4);

        if(NachOSThread::getThreadCount() == 1) {    // if only one thread is left and is about to die.
        	interrupt -> Halt();
        }

        if(currentThread -> getParent() != NULL) {
        	currentThread -> actionForParentWaiting(exitcode);
        }
        
        currentThread->tellChildrenImDying();
        currentThread->FinishThread();

        machine->WriteRegister(PrevPCReg, machine->ReadRegister(PCReg));
        machine->WriteRegister(PCReg, machine->ReadRegister(NextPCReg));
        machine->WriteRegister(NextPCReg, machine->ReadRegister(NextPCReg)+4);
    }

    else {
	printf("Unexpected user mode exception %d %d\n", which, type);
	ASSERT(FALSE);
    }
}
