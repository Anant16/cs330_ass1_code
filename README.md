# cs330_ass1_code

#SysCall_GetReg 
We read the value of the register $4 using the function Machine::ReadRegister and Write this value in register $2 as funtion return value using Machine::WriteRegister.

#SysCall_GetPA
We first check the various conditions where our syscall could potentially fail, returning -1 if such a condition is found true.

Else, we refer the code for Translate method to translate the given Virtual Address (in register $4) to the corresponding Physical Address (and put it in register $2).

#SysCall_GetPID
To generate unique pid we keep a count of threads created and loop it around the limit of unsigned int to prevent overflow.
We assign each thread a pid equal to the number of threads created so far.

