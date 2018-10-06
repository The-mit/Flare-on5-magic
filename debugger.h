#include <sys/ptrace.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <string.h>
#include <iostream>
#include <map>

using namespace std;

const int long_size = sizeof(long);
map<int, char> breakpoints;

// getdata dn putdata was taken from the following link, and fixed:
// https://theantway.com/2013/01/notes-for-playing-with-ptrace-on-64-bits-ubuntu-12-10/

void getdata(pid_t child, long addr, char *str, int len)
{
	char *laddr;
	int i, j;
	union u
	{
		long val;
		char chars[long_size];
	} data;

	i = 0;
	j = len / long_size;
	laddr = str;
	while(i < j)
	{
		data.val = ptrace(PTRACE_PEEKDATA, child, addr + i * long_size, NULL);
		memcpy(laddr, data.chars, long_size);
		++i;
		laddr += long_size;
	}

	j = len % long_size;
	if(j != 0)
	{
		data.val = ptrace(PTRACE_PEEKDATA, child, addr + i * long_size, NULL);
		memcpy(laddr, data.chars, j);
	}
	str[len] = '\0';
}

void putdata(pid_t child, long addr, const char *str, int len)
{
	char remains[long_size+1] = {0};
	const char *laddr;
	int i, j;
	union u
	{
		long val;
		char chars[long_size];
	} data;

	i = 0;
	j = len / long_size;
	laddr = str;
	while(i < j)
	{
		memcpy(data.chars, laddr, long_size);
		ptrace(PTRACE_POKEDATA, child, addr + i * long_size, data.val);
		++i;
		laddr += long_size;
	}

	j = len % long_size;
	getdata(child, addr, remains, long_size);
	if(j != 0)
	{
		memcpy(data.chars, laddr, j);
		memcpy(data.chars + j, remains + j, long_size-j);
		ptrace(PTRACE_POKEDATA, child, addr + i * long_size, data.val);
	}
}

void dumpRegisters(int pid, int waitStatus)
{
	char c[5] = {0};
	struct user_regs_struct currentRegs;
	int stopped;

	stopped = WIFSTOPPED(waitStatus);
	cout << "stopped: " << stopped << ", error code: " << hex << WSTOPSIG(waitStatus) << endl;
	ptrace(PTRACE_GETREGS, pid, NULL, NULL, &currentRegs);                	
	getdata(pid, currentRegs.rip, c, 4);

	cout << "[RIP]: " << hex << c[0]*256*256*256 + c[1]*256*256 + c[2]*256 + c[3] << endl;	
	cout << "rip: " << hex << currentRegs.rip << endl;
	cout << "rax: " << hex << currentRegs.rax << endl;
	cout << "rbx: " << hex << currentRegs.rbx << endl;
	cout << "rcx: " << hex << currentRegs.rcx << endl;
	cout << "rdx: " << hex << currentRegs.rdx << endl;
	cout << "rdi: " << hex << currentRegs.rdi << endl;
	cout << "rsi: " << hex << currentRegs.rsi << endl;
	cout << "rsp: " << hex << currentRegs.rsp << endl;
}

void placeBreakpoint(int pid, long address)
{
	char originalContent[2] = {0};
	getdata(pid, address, originalContent, 1);
	putdata(pid, address, "\xCC", 1);
	breakpoints[address] = originalContent[0];
}

void removeBreakpoint(int pid, long address)
{
	char c[4] = {breakpoints[address], 0};
	putdata(pid, address, c, 1);
	breakpoints.erase(address);
}

void resumeExecution(int pid)
{
	long address;
	struct user_regs_struct currentRegs;
	
	ptrace(PTRACE_GETREGS, pid, NULL, &currentRegs);
	address = currentRegs.rip - 1;

	if(breakpoints.find(address) == breakpoints.end())
	{
		if(currentRegs.rip != 0x40303C)	// start of test_key function, temporary bp
		{
			cout << "bp not hit@" << hex << currentRegs.rip << endl;
			dumpRegisters(pid, 0);
		}
	}
	else
	{
		// cout << "bp hit@" << hex << address << endl;
		currentRegs.rip -= 1;
		ptrace(PTRACE_SETREGS, pid, NULL, &currentRegs);
		//putdata(pid, address, &(breakpoints[address]), 1);
		removeBreakpoint(pid, address);
		ptrace(PTRACE_SINGLESTEP, pid, NULL, NULL);
		wait(NULL);
		//putdata(pid, address, "\xCC", 1);
		placeBreakpoint(pid, address);
	}
	ptrace(PTRACE_CONT, pid, NULL, NULL);
}
