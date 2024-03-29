	/*
 * Copyright (c) 2000, 2001, 2002, 2003, 2004, 2005, 2008, 2009
 *	The President and Fellows of Harvard College.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE UNIVERSITY AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE UNIVERSITY OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef _SYSCALL_H_
#define _SYSCALL_H_


#include <cdefs.h> /* for __DEAD */
struct trapframe; /* from <machine/trapframe.h> */

#define FILE_NAME_MAXLEN 400

/*
 * The system call dispatcher.
 */

void syscall(struct trapframe *tf);

/*
 * Support functions.
 */

/* Helper for fork(). You write this. */
void enter_forked_process(void* tf, unsigned long n);

/* Enter user mode. Does not return. */
__DEAD void enter_new_process(int argc, userptr_t argv, userptr_t env,
		       vaddr_t stackptr, vaddr_t entrypoint);


/*
 * Prototypes for IN-KERNEL entry points for system call implementations.
 */

int sys_reboot(int code);
int sys___time(userptr_t user_seconds, userptr_t user_nanoseconds);


// file system calls

int sys_open(userptr_t file_name, int arguments, int mode, int32_t* retval);
int sys_read(int fd, userptr_t user_buf_ptr, int buflen, int32_t* retval);
int sys_write(int fd, userptr_t user_buf_ptr, int nbytes, int32_t* retval);
int sys_close(userptr_t fd, int32_t* retval);
int sys_lseek(userptr_t fd, off_t seek_pos, userptr_t whence, off_t* retval);
int sys_dup2(userptr_t oldfd, userptr_t newfd , int32_t* retval);

// memory system calls
int sys_sbrk(userptr_t npages, int32_t* retval);
// process system calls

int sys_fork(struct trapframe* tf, pid_t* pid);
int sys_getpid(pid_t* retval);
int sys_waitpid(userptr_t userpid, userptr_t status, userptr_t options, pid_t* retval);
int sys_execv(userptr_t program, userptr_t args, int32_t* retval);
int sys__exit(int exitcode);

#endif /* _SYSCALL_H_ */
