# cs330_ass1_code

#SysCall_GetReg 

	We read the value of the register $4 using the function Machine::ReadRegister and write this value in register $2 as funtion return value using Machine::WriteRegister.

#SysCall_GetPA

	We first check the various conditions where our syscall could potentially fail, returning -1 if such a condition is found true.

	Else, we refer the code for Translate method to translate the given Virtual Address (in register $4) to the corresponding Physical Address (and put it in register $2).

#SysCall_GetPID

	To generate unique pid we keep a count of threads created and loop it around the limit of unsigned int to prevent overflow.
	We assign each thread a pid equal to the number of threads created so far.

#SysCall_GetPPID

	The PID of the current thread (or calling thread) is set as the PPID of the child thread.
	In case the current thread has no parent, we assign value -1 to PPID.

#SysCall_Time

	This system call required us to output the value of the variable totalTicks. This variable was defined in the Statistics class.

	An object of this class named stats was initialized in the system.cc file. We were required to print the value of totalTicks into Register 4.
	PC was then incremented.

#SysCall_Yield

	This system call required us to call the YieldCPU() function of the NachOSThread class.
	Note that PC was incremented before calling YieldCPU() as the current thread, once it has called YieldCPU(), will not be able to increment PC.

#SysCall_Sleep

	This system call takes input num_ticks. This is the amount of time (Ticks) for which the thread should go to sleep.

	If num_ticks is 0, we simply call the YieldCPU() function.

	Otherwise, we insert the thread into a List timerQueue. The insertion is done in sorted manner (in increasing order of wake-up time OR key).
	The wake-up time is the sum of present time (totalTicks) and sleep time (num_ticks).

	The current thread is then put to sleep (using PutThreadToSleep()) after disabling interrupts. Interrupts are re-enabled when the thread wakes up.
	Also, we have hardware timer (called timer) which generetes an interrupt every timerTicks number of ticks. The TimerInterruptHandler wakes up all threads 
	with key less than or equal to present time (totalTicks) and puts them on the ready list.

	Here again, PC is incremented at the beginning of the system call (same reasons as for SysCall_Yield).

#SC_Exec

    This system call requires us to execute the standard execv() command.
    To start with, we have to obtain the string passed to the syscall. To do so, we have to manually translate the addresses and obtain it just like it was done in PrintString.

    Once we have obtained the filename, we create an instance of OpenFile with this filename unless the filename is null.

    A change was made in the ProcessAddressSpace constructor.

    First, the mapping of the PageTable was changed as pointed out earlier.
    Then, the function bzero zeroed out the address space from the starting, it was modified so that it only zeroes out the address space from the alloted pages
    Finally, the noff function was also modified to reflect the change in the mappinf function.

    The registers of the new excutable were initialized with its state restored.
	Finally, we run the new program using the Machine::Run() command.

#SysCall_Fork

	This system call requires us to create a child thread and duplicate the address space of the calling thread (parent).

	Firstly, PC is incremented (as we don't know which thread will run at the end of the syscall).

	A new thread is created with parent (pointer to parent thread) set as the calling thread. The PID of new thread is stored in the ChildPid array, 
	the ChildStatus is set as CHILD_ALIVE and a pointer to the child thread is stored in ChildList. The IsWaiting element corresponding to the child is set as FALSE and the ChildCount is incremented.
  
    A new address space was created for the child. This was done by overloading the constructor of ProcessAddressSpace class to handle the case when we fork threads.
	In this new constructor, the Page Table mapped virtual address to physical address using numPagesAllocated, where numPagesAllocated is a global which stores the total number of physical pages which have been alloted as of yet.
	Next we duplicated the parent's physical memory exactly into the child's physical memory location.

    Now the we change the return value of child to zero and it's state is saved using SaveUserState() function of the NachOSThread class.

    The parent's return address is set to the child's pid and the child's stack is set to the function forknew which is defined in exception.cc. This essentially mimics the behaviour a normal thread has on returning from a sleep. This is done to ensure that a newly forked thread behaves like any other woken thread. The child thread on waking up starts running from the function which is passed to forknew. The forknew function also runs the Machine::Run() command.

    After this the child is scheduled to run.


#SysCall_Join

    This system call takes input cpid (the PID of the child which is to be joined with). We search the ChildPid for the presence of the input cpid.
    The search is done linearly (using the ChildCount variable which stores total number of children of a thread).
    If the child is not found, we return -1.

    Otherwise, we obtain the index at which the child thread is located from our earlier search. This index is used to check the status of the child in the
    ChildStatus array.

    If the status is CHILD_ALIVE, we put the parent thread to sleep (interrupts handled accordingly). When the child exits, it first wakes up its parent and saves its exit code in ChildStatus of parent before terminating. This part is handled (implemented) in SysCall_Exit.

    Otherwise, the child is already dead and the exit code is read from the ChildStatus array. The exit code of a child thread is always saved in the ChildStatus array of the parent before it exits. This is implemented in SysCall_Exit.

    PC is incremented at the beginning of the system call.

#SysCall_Exit

    Firstly, we read the exit code from Register 2. Then, we proceed as follows.

    A static variable called ThreadCount was created in the Thread class which store the number of live threads. This is increment when a thread is created and decremented when a thread is terminated.
    If ThreadCount is 1, we halt the machine.

    Otherwise. if the parent of the child is alive, we check the value of the current thread in the IsWaiting array (we have a pointer to the parent). If it is 1, then we wake the parent up.
    Then, we just set the exit code of the child in the parent's ChildStatus array.

    Then, we use the ChildList of the current thread to decide what is to be done with the children of the current thread.
    We simply change the PPID of all alive children to -1 and change the parent thread to NULL.

    Then we call the FinishThread() function and terminate the thread.

#SysCall_NumInstr

	The mipssim.cc file is modified to increment the number of instructions every time a new instruction is created. This value is then written to Register 4 when this system call is executed.