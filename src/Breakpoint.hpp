#pragma once

#include <assert.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <signal.h>
#include <syscall.h>
#include <sys/ptrace.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/reg.h>
#include <sys/user.h>
#include <unistd.h>
#include <errno.h>

#include <stdint.h>

class Breakpoint
{
public:
	void *addr;
	uint64_t orig_data;

	Breakpoint(pid_t pid, void *addr);

	bool stepOver(pid_t pid);

private:
	void enable(pid_t pid);
	void disable(pid_t pid);
};
