#include <sys/ptrace.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <sys/reg.h>
#include <sys/user.h>
#include <stdio.h>
#include <iostream>
#include <map>
#include <stdlib.h>

#include "debugger.h"

using namespace std;

#define DEBUGEE_NAME "magic"
#define FLAG_LENGTH 80
#define WRITE 1
#define READ 0

// pre-initialized using a python script
char flag[FLAG_LENGTH] = {' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', '\n'};

int fd[2];
int childID = 0;
int successes = 0;

map<long, char *> decoded;

int createAndInitChildProcess(int pid);
int createNewChildProcess();
void startDebugger();
void printFlag();
void sendFlagTest();
void overrideFlagInMemory(int pid, long flagAddress);
char* getCharsCopy(int offset, int length);
void calculateNextFlag(int offset, int length);

void clearFlag()
{
	for(int i = 0; i < FLAG_LENGTH-1; i++)
	{
		flag[i] = ' ';
	}
	flag[FLAG_LENGTH-1] = '\n';
}

void startDebugger()
{
	int waitStatus;
	int childPid;
	int doneFirstPermutation = 0;
	long currentKey = 0;
	char *tempInput = NULL;
	char c[5];
	char spaceBuffer[3] = {' ', ' ', ' '};
	struct user_regs_struct currentRegs, startingRegs;

	childPid = createAndInitChildProcess(0);
	ptrace(PTRACE_GETREGS, childPid, NULL, &startingRegs);
	while(!doneFirstPermutation)
	{
		//printFlag();
		overrideFlagInMemory(childPid, startingRegs.rdi);
		resumeExecution(childPid);

		wait(&waitStatus);	// should hit one of the breakpoints
		ptrace(PTRACE_GETREGS, childPid, NULL, &currentRegs);
		//cout << "rip: " << hex << currentRegs.rip << endl;
		//cout << "rdi: " << hex << currentRegs.rdi << endl;
		switch(currentRegs.rip)
		{
		case 0x0402FCA + 1:	// the wanted breakpoint. accounted for the int 3
			successes += 1;
			cout << "key: " << currentKey << ", content: " << tempInput << endl;
			if(tempInput == NULL)
			{
				decoded[currentKey] = spaceBuffer;
			}
			else
			{
				decoded[currentKey] = tempInput;
			}
			printFlag();
			tempInput = NULL;	// free(NULL) is defined and OK
			cout << "##########first bp hit##########" << endl;
			break;
		case 0x0402F66 + 1:	// unwanted breakpoint, char sequence is wrong
			getdata(childPid, currentRegs.rbp - 4, c, 4);
			//cout << "i=" << hex << *((int *)c) << endl;
			//cout  << "second bp hit" << endl;
			ptrace(PTRACE_SETREGS, childPid, NULL, &startingRegs);	//restart execution
			break;
		case 0x0402F06 + 1:	// call rcx, calculate next flag. accounting to index and size
			//cout << "called" << endl;
			{
				int offset = currentRegs.rdi - startingRegs.rdi;
				int length = currentRegs.rsi;
				
				calculateNextFlag(offset, length);
				free(tempInput);
				currentKey = ptrace(PTRACE_PEEKDATA, childPid, currentRegs.rdx, NULL);	// get key
				tempInput = getCharsCopy(offset, length);							// current test to decode
			}
			break;
		case 0x0403B62 + 1:	// premutation done.
			cout << "Final flag: " << flag << endl << endl;
			clearFlag();
			successes = 0;
			placeBreakpoint(childPid, 0x040303C);
			resumeExecution(childPid);
			sendFlagTest();
			wait(NULL);	//go to start of test_key
			removeBreakpoint(childPid, 0x040303C);
			ptrace(PTRACE_GETREGS, childPid, NULL, &startingRegs);
			startingRegs.rip -= 1;
			ptrace(PTRACE_SETREGS, childPid, NULL, &startingRegs);
			doneFirstPermutation = true;
			break;
		default:
			cout << "Something went wrong, seems like process crashed with rip=" << currentRegs.rip << endl;
			dumpRegisters(childPid, waitStatus);
		}
	}

	cout << "first permutation done!" << endl;
	while(1)	// run on other permutations using the decoded values
	{
		//printFlag();
		overrideFlagInMemory(childPid, startingRegs.rdi);
		resumeExecution(childPid);

		wait(&waitStatus);	// should hit one of the breakpoints
		ptrace(PTRACE_GETREGS, childPid, NULL, &currentRegs);
		//cout << "rip: " << hex << currentRegs.rip << endl;
		//cout << "rdi: " << hex << currentRegs.rdi << endl;
		switch(currentRegs.rip)
		{
		case 0x0402FCA + 1:	// the wanted breakpoint. accounted for the int 3
			successes += 1;
			printFlag();
			cout << "##########first bp hit##########" << endl;
			break;
		case 0x0402F66 + 1:	// unwanted breakpoint, char sequence is wrong
			getdata(childPid, currentRegs.rbp - 4, c, 4);
			//cout << "i=" << hex << *((int *)c) << endl;
			cout  << "second bp hit" << endl;
			ptrace(PTRACE_SETREGS, childPid, NULL, &startingRegs);	//restart execution
			break;
		case 0x0402F06 + 1:	// call rcx, calculate next flag. accounting to index and size
			//cout << "called" << endl;
			{
				int length = currentRegs.rsi;
				int offset = currentRegs.rdi - startingRegs.rdi;
				int i;
				currentKey = ptrace(PTRACE_PEEKDATA, childPid, currentRegs.rdx, NULL);	// get key
				
				for(i = 0; i < length; i++)
				{
					flag[offset + i] = decoded[currentKey][i];
				}
			}
			break;
		case 0x0403B62 + 1:	// premutation done.
			cout << "Final flag: " << flag << endl << endl;
                        clearFlag();
                        successes = 0;
                        placeBreakpoint(childPid, 0x040303C);
                        resumeExecution(childPid);
                        sendFlagTest();
                        wait(NULL);     //go to start of test_key
                        removeBreakpoint(childPid, 0x040303C);
                        ptrace(PTRACE_GETREGS, childPid, NULL, &startingRegs);
                        startingRegs.rip -= 1;
                        ptrace(PTRACE_SETREGS, childPid, NULL, &startingRegs);
			break;
		default:
			cout << "Something went wrong, seems like process crashed with rip=" << currentRegs.rip << endl;
			dumpRegisters(childPid, waitStatus);
		}
	}
}

void printFlag()
{
	cout << "flag so far: \"" << flag << "\", successes:" << successes << endl;
}

void sendFlagTest()
{
	dprintf(fd[WRITE], "%s", flag);
}

void overrideFlagInMemory(int pid, long bufferAddress)
{
	putdata(pid, bufferAddress, flag, FLAG_LENGTH);
}

char* getCharsCopy(int offset, int length)
{
	int i = 0;
	char *p = (char *) malloc(length*sizeof(char));
	for(i = 0; i < length; i++)
	{
		p[i] = flag[offset+i];
	}
	
	return p;
}

void calculateNextFlag(int offset, int length)
{
	int i, sum = 0, index;
	//index = indexes[successes];
	//length = lengths[successes];
	index = offset;

	//cout << "modifing at offset: " << index << " " << length << " bytes" << endl;

	for(i = 0; i < length; i++)
	{
		sum *= 256;
		sum += flag[index + i];
	}
	sum += 1;

	for(i = length-1; i >= 0; i--)
	{
		if(sum%256 == 0)
		{
			sum += 0x20;	// skip the none printable
		}
		if(sum%256 == 0x7F)
		{
			sum += 256-0x7F + 0x20;	// if past max char, go back to space
		}
		flag[index + i] = sum%256;
		sum = sum/256;
	}
}

int createAndInitChildProcess(int currentChildPid)
{
	int waitStatus, pid;
	struct user_regs_struct currentRegs;
	
	if(currentChildPid != 0)
	{
		kill(currentChildPid, SIGKILL);
	}
	pid = createNewChildProcess();
	wait(&waitStatus);	//wait for process to exec
	cout << "stopped: " << WIFSTOPPED(waitStatus) << ", error code: " << hex << WSTOPSIG(waitStatus) << endl;

	breakpoints.clear();
	placeBreakpoint(pid, 0x040303C);	// bp for the start of test_key
	placeBreakpoint(pid, 0x0402FCA);	// correct sequence
	placeBreakpoint(pid, 0x0402F66);	// wrong sequence
	placeBreakpoint(pid, 0x0402F06);	// call rcx, get the index and length
	placeBreakpoint(pid, 0x0403B62);	// done the current pemutation

	resumeExecution(pid);
	sendFlagTest();
	wait(&waitStatus);
	cout << "stopped: " << WIFSTOPPED(waitStatus) << ", error code: " << hex << WSTOPSIG(waitStatus) << endl;

	removeBreakpoint(pid, 0x040303C);
	
	//return rip one byte backwards, to point to the original instruction
	ptrace(PTRACE_GETREGS, pid, NULL, &currentRegs);
	currentRegs.rip -= 1;

	cout << "start rip: " << hex << currentRegs.rip << endl << endl;
	ptrace(PTRACE_SETREGS, pid, NULL, &currentRegs);

	return pid;
}

int createNewChildProcess()
{
	int pid;
	if(pipe(fd))
	{
		perror("pipe");
		return -1;
	}

	pid = fork();
	if(!pid) // child
	{
		if(ptrace(PTRACE_TRACEME, 0, 0, 0) < 0)
		{
			perror("failed to ptrace");
		}
		//close(STDOUT_FILENO);
		close(fd[WRITE]);
		dup2(fd[READ], STDIN_FILENO);
		
		execl(DEBUGEE_NAME, DEBUGEE_NAME, (char *) NULL);
		printf("Failed to create child process\n");
	}
	else
	{
		close(fd[READ]);
	}

	return pid;
}

int main()
{
	startDebugger();
	return 0;
}
