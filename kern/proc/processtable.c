/**
 * processtable.c
 *
 * Implements methods defined in processtable.h
 *
 */
#include <processtable.h>
#include <array.h>
#include <kern/errno.h>

static struct process_table {
	struct array* pt_processes; // holds the processes in the process table
	int pt_pidcounter; // counter for tracking process ids
	struct array* pt_freepids; // list holding free process ids that can be reused
} s_processtable;

int init_processtable() {
	s_processtable.pt_processes = array_create();
	if(s_processtable.pt_processes == NULL) {
			return -1;
	}
	if(array_preallocate(s_processtable.pt_processes, 1024) == ENOMEM) {
		array_destroy(s_processtable.pt_processes);
		return -1;
	}
	s_processtable.pt_pidcounter = 2;
	s_processtable.pt_freepids = array_create();
	if(s_processtable.pt_freepids == NULL) {
		array_destroy(s_processtable.pt_processes);
		return -1;
	}
#if OPT_DUMBVM
#else
	if(array_preallocate(s_processtable.pt_freepids, 1024) == ENOMEM) {
		array_destroy(s_processtable.pt_freepids);
		array_destroy(s_processtable.pt_processes);
		return -1;
	}
#endif

	return 0;
}

static int fetchPid() {
	int arrnum = array_num(s_processtable.pt_freepids);
	if (arrnum > 0) {
		void *pid =  array_get(s_processtable.pt_freepids, arrnum - 1);
		int ret = (int)pid;
		//TODO hold the freed int's in another free list to avoid repeated malloc's
		array_remove(s_processtable.pt_freepids, arrnum - 1);
		return ret;
	}
	return s_processtable.pt_pidcounter++;
}

int addTo_processtable(struct proc* process) {
	int result = 0;
	// TODO add checks
	unsigned int idx;
	process->p_pid = fetchPid();
	if (array_add(s_processtable.pt_processes, process, &idx)) {
		result = -1;
	}
	return result;
}

static int reclaimpid(int pid) {
	unsigned int idx;
	//int* val = kmalloc(sizeof(int));
	//*val = pid;
	if (array_add(s_processtable.pt_freepids, (void*)pid, &idx)) {
		return 1;
	}
	return 0;
}

static unsigned int getarrayindex(int pid) {
	unsigned int i;
	for (i = 0; i < array_num(s_processtable.pt_processes); i++) {
		if (((struct proc*) array_get(s_processtable.pt_processes, i))->p_pid
				== pid) {
			return i;
		}
	}
	return -1;
}

int removeFrom_processtable(int pid) {
	int result = 0;

	int index = getarrayindex(pid);
	if (index < 0) {
		return -1;
	}
	struct proc *process = (struct proc*) array_get(s_processtable.pt_processes,
			index);
	reclaimpid(process->p_pid);
	array_remove(s_processtable.pt_processes, index);
	return result;
}

int lookup_processtable(int pid, struct proc** process) {
	if (process == 0) {
		return -1;
	}
	int index = getarrayindex(pid);
	if (index < 0) {
		*process = 0;
		return -1;
	}
	*process = (struct proc*) array_get(s_processtable.pt_processes, index);
	return 0;
}
