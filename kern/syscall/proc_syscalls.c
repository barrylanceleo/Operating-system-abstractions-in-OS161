/**
 * proc_syscalls.c
 *
 * Implementation for process related system calls
 *
 */

#include <types.h>
#include <current.h>
#include <proc.h>
#include <thread.h>
#include <kern/errno.h>
#include <processtable.h>
#include <synch.h>
#include <cpu.h>
#include <syscall.h>
#include <copyinout.h>
#include <limits.h>
#include <mips/trapframe.h>
#include <addrspace.h>
#include <kern/wait.h>

int sys_getpid(pid_t* retval) {
	*retval = curproc->p_pid;
	return 0;
}

int sys_fork(struct trapframe* tf, pid_t* retval) {
	int result;
	struct addrspace *ad;
	struct proc *child = proc_createchild(curproc, &ad);
	if(child == NULL)
	{
		*retval = -1;
		return ENOMEM;
	}

	result = as_copy(curproc->p_addrspace, &(child->p_addrspace));
	if(result){
		proc_destroy(child);
		*retval = -1;
		return ENOMEM;
	}

	struct trapframe* child_tf = (struct trapframe*)kmalloc(sizeof(struct trapframe));
	if(child_tf == NULL){
		proc_destroy(child);
		kfree(child_tf);
		*retval = -1;
		return ENOMEM;
	}
	*child_tf = *tf;
//	kprintf("TEMPPPP:In fork Child PID IS %d\n", child->p_pid);

	lock_acquire(child->p_opslock);
	result = thread_fork("Child proc", child, enter_forked_process,
		(struct trapframe *)child_tf,(unsigned long)(child->p_addrspace));

	if(result){
		kfree(child_tf);
		*retval = -1;
		lock_release(child->p_opslock);
		proc_destroy(child);
		return ENOMEM;
	}

	*retval = child->p_pid;
	lock_release(child->p_opslock);
	return 0;
/*	int ret = 0;
	struct proc* parent = curproc;
	kprintf("TEMPPPP:HERE FORK\n");
	struct addrspace *as;
	struct proc* child = proc_createchild(parent, &as);
	if (child == NULL) {
		return ENOMEM;
	}
	struct trapframe* copytf =  kmalloc(sizeof(struct trapframe));
	*copytf = *tf;
	kprintf("TEMPPPP:after create child\n");
	struct thread* currentthread = curthread;
	ret = thread_fork(currentthread->t_name, child, enter_forked_process,
			(void*) copytf, (unsigned long) as);

	if (ret) {
		proc_destroy(child);
		return ENOMEM;
	}
	kprintf("TEMPPPP:after thread fork\n");
	*pid = child->p_pid;
	return ret;*/
}

static void copyargstokernel(userptr_t uargs, char*** argv, unsigned long* argc) {
	*argv = (char**)kmalloc(ARG_MAX);
	char ** kargv = *argv;

	userptr_t uarg_iter = uargs;
	userptr_t uaddress;
	*argc = 0;
	char buf[1024];//[ARG_MAX];
	while (uarg_iter != NULL) {

		copyin(uarg_iter, &uaddress, sizeof(char*));
		size_t len = 0;
		copyinstr(uaddress, buf, 1024/*ARG_MAX*/, &len);
		if(len == 0) {
			kprintf("TEMPPPP: %lu = argc!!!\n", *argc);
			return;
		}
		kargv[*argc] = (char*) kmalloc(sizeof(char) * len);
		strcpy(kargv[*argc], buf);
		(*argc)++;
		/*if(*argc == 399) {
			char** temp = (char**)kmalloc(ARG_MAX*sizeof(char*));
			*argv = temp;
			memmove(argv, kargv, sizeof(char*)*(*argc));
			kargv = *argv;
		}*/
		uarg_iter+= sizeof(userptr_t);

	}
}

int sys_execv(userptr_t program, userptr_t args) {
	int result;

	char k_progname[FILE_NAME_MAXLEN];
	if ((result = copyinstr(program, k_progname, FILE_NAME_MAXLEN, 0)) != 0) {
		return result;
	}

	char* progname = k_progname;
	unsigned long argc;
	char** argv;

	copyargstokernel(args, &argv, &argc);

	if(argc == 0) {
		runprogram2(progname, NULL, 0);
	} else {
		runprogram2(progname, argv, argc);
	}
	return result;
}

static int isParent(pid_t k_pid) {
	struct proc * p;
	lookup_processtable(k_pid, &p);
	if(p == NULL) {
		return 2;
	}
	if(p->p_ppid != curproc->p_pid) {
		return 1;
	}
	return 0;
}

int k_waitpid(pid_t k_pid, int* status, pid_t* retval) {
	int result = 0;
	struct proc* targetprocess;
	switch(isParent(k_pid)) {
	case 1:
		*retval = -1;
		return ECHILD;
	case 2:
		*retval = -1;
		return ESRCH;
	}
	lookup_processtable(k_pid, &targetprocess);
	lock_acquire(targetprocess->p_waitcvlock);
	while (true) {
		if (targetprocess->p_state == PS_COMPLETED) {
//			kprintf("TEMPPPP PS Completed:\n");
			*status = targetprocess->p_returnvalue;
			lock_release(targetprocess->p_waitcvlock);
			proc_destroy(targetprocess);
			*retval = k_pid;
			return result;
		}
		cv_wait(targetprocess->p_waitcv, targetprocess->p_waitcvlock);
	}
	return -1; // this code must be unreachable
}

int sys_waitpid(userptr_t userpid, userptr_t status, userptr_t options,
		pid_t* retval) {

	int k_options = (int) options;

	if (k_options != 0) {
		*retval = -1;
		return -1;
	}

	pid_t k_pid = (pid_t) userpid;
	int result = 0;

	int k_status = 0;
	result = k_waitpid(k_pid, &k_status, retval);
	k_status = _MKWAIT_EXIT(k_status);
	copyout(&k_status, status, sizeof(int));


	return result;

}

int sys__exit(int exitcode) {
	int result = 0;
	struct proc* curprocess = curproc;
	lock_acquire(curprocess->p_waitcvlock);
	curprocess->p_returnvalue = _MKWAIT_EXIT(exitcode);
	curprocess->p_state = PS_COMPLETED;
//	kprintf("TEMPPPP: PS Set to completed %d in pid: %d\n", exitcode, curprocess->p_pid);
	cv_broadcast(curprocess->p_waitcv, curprocess->p_waitcvlock);
	lock_release(curprocess->p_waitcvlock);

	thread_exit(); // stop execution of current thread

	return result; // Will not be executed
}

