/*
 *  Multi2Sim
 *  Copyright (C) 2011  Rafael Ubal (ubal@ece.neu.edu)
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <cpukernel.h>
#include <misc.h>


/*
 * Global variables
 */

/* Configuration parameters */
long long ke_max_inst = 0;
long long ke_max_cycles = 0;
long long ke_max_time = 0;
enum cpu_sim_kind_t cpu_sim_kind = cpu_sim_functional;


/* Reason for simulation end */
enum ke_sim_finish_t ke_sim_finish = ke_sim_finish_none;
struct string_map_t ke_sim_finish_map =
{
	9, {
		{ "ContextsFinished", ke_sim_finish_ctx },
		{ "MaxCPUInst", ke_sim_finish_max_cpu_inst },
		{ "MaxCPUCycles", ke_sim_finish_max_cpu_cycles },
		{ "MaxGPUInst", ke_sim_finish_max_gpu_inst },
		{ "MaxGPUCycles", ke_sim_finish_max_gpu_cycles },
		{ "MaxGPUKernels", ke_sim_finish_max_gpu_kernels },
		{ "MaxTime", ke_sim_finish_max_time },
		{ "Signal", ke_sim_finish_signal },
		{ "Stall", ke_sim_finish_stall },
		{ "GPUNoFaults", ke_sim_finish_gpu_no_faults }  /* GPU-REL */
	}
};

/* CPU kernel */
struct kernel_t *ke;




/*
 * Public functions
 */


/* Initialization */

static uint64_t ke_init_time = 0;

void ke_init(void)
{
	union {
		unsigned int as_uint;
		unsigned char as_uchar[4];
	} endian;

	/* Endian check */
	endian.as_uint = 0x33221100;
	if (endian.as_uchar[0])
		fatal("%s: host machine is not little endian", __FUNCTION__);

	/* Host types */
	M2S_HOST_GUEST_MATCH(sizeof(long long), 8);
	M2S_HOST_GUEST_MATCH(sizeof(int), 4);
	M2S_HOST_GUEST_MATCH(sizeof(short), 2);

	/* Initialization */
	sys_init();
	isa_init();

	/* Allocate */
	ke = calloc(1, sizeof(struct kernel_t));
	if (!ke)
		fatal("%s: out of memory", __FUNCTION__);

	/* Event for context IPC reports */
	EV_CTX_IPC_REPORT = esim_register_event(ctx_ipc_report_handler);

	/* Initialize */
	ke->current_pid = 1000;  /* Initial assigned pid */
	
	/* Initialize mutex for variables controlling calls to 'ke_process_events()' */
	pthread_mutex_init(&ke->process_events_mutex, NULL);

	/* Initialize GPU */
	gk_init();

	/* Record start time */
	ke_init_time = ke_timer();
}


/* Finalization */
void ke_done(void)
{
	struct ctx_t *ctx;

	/* Finish all contexts */
	for (ctx = ke->context_list_head; ctx; ctx = ctx->context_list_next)
		if (!ctx_get_status(ctx, ctx_finished))
			ctx_finish(ctx, 0);

	/* Free contexts */
	while (ke->context_list_head)
		ctx_free(ke->context_list_head);
	
	/* Finalize GPU */
	gk_done();

	/* End */
	free(ke);
	isa_done();
	sys_done();
}


void ke_dump(FILE *f)
{
	struct ctx_t *ctx;
	int n = 0;
	ctx = ke->context_list_head;
	fprintf(f, "List of kernel contexts (arbitrary order):\n");
	while (ctx) {
		fprintf(f, "kernel context #%d:\n", n);
		ctx_dump(ctx, f);
		ctx = ctx->context_list_next;
		n++;
	}
}


void ke_list_insert_head(enum ke_list_kind_t list, struct ctx_t *ctx)
{
	assert(!ke_list_member(list, ctx));
	switch (list)
	{
	case ke_list_context:

		DOUBLE_LINKED_LIST_INSERT_HEAD(ke, context, ctx);
		break;

	case ke_list_running:

		DOUBLE_LINKED_LIST_INSERT_HEAD(ke, running, ctx);
		break;

	case ke_list_finished:

		DOUBLE_LINKED_LIST_INSERT_HEAD(ke, finished, ctx);
		break;

	case ke_list_zombie:

		DOUBLE_LINKED_LIST_INSERT_HEAD(ke, zombie, ctx);
		break;

	case ke_list_suspended:

		DOUBLE_LINKED_LIST_INSERT_HEAD(ke, suspended, ctx);
		break;

	case ke_list_alloc:

		DOUBLE_LINKED_LIST_INSERT_HEAD(ke, alloc, ctx);
		break;
	}
}


void ke_list_insert_tail(enum ke_list_kind_t list, struct ctx_t *ctx)
{
	assert(!ke_list_member(list, ctx));
	switch (list) {
	case ke_list_context: DOUBLE_LINKED_LIST_INSERT_TAIL(ke, context, ctx); break;
	case ke_list_running: DOUBLE_LINKED_LIST_INSERT_TAIL(ke, running, ctx); break;
	case ke_list_finished: DOUBLE_LINKED_LIST_INSERT_TAIL(ke, finished, ctx); break;
	case ke_list_zombie: DOUBLE_LINKED_LIST_INSERT_TAIL(ke, zombie, ctx); break;
	case ke_list_suspended: DOUBLE_LINKED_LIST_INSERT_TAIL(ke, suspended, ctx); break;
	case ke_list_alloc: DOUBLE_LINKED_LIST_INSERT_TAIL(ke, alloc, ctx); break;
	}
}


void ke_list_remove(enum ke_list_kind_t list, struct ctx_t *ctx)
{
	assert(ke_list_member(list, ctx));
	switch (list) {
	case ke_list_context: DOUBLE_LINKED_LIST_REMOVE(ke, context, ctx); break;
	case ke_list_running: DOUBLE_LINKED_LIST_REMOVE(ke, running, ctx); break;
	case ke_list_finished: DOUBLE_LINKED_LIST_REMOVE(ke, finished, ctx); break;
	case ke_list_zombie: DOUBLE_LINKED_LIST_REMOVE(ke, zombie, ctx); break;
	case ke_list_suspended: DOUBLE_LINKED_LIST_REMOVE(ke, suspended, ctx); break;
	case ke_list_alloc: DOUBLE_LINKED_LIST_REMOVE(ke, alloc, ctx); break;
	}
}


int ke_list_member(enum ke_list_kind_t list, struct ctx_t *ctx)
{
	switch (list) {
	case ke_list_context: return DOUBLE_LINKED_LIST_MEMBER(ke, context, ctx);
	case ke_list_running: return DOUBLE_LINKED_LIST_MEMBER(ke, running, ctx);
	case ke_list_finished: return DOUBLE_LINKED_LIST_MEMBER(ke, finished, ctx);
	case ke_list_zombie: return DOUBLE_LINKED_LIST_MEMBER(ke, zombie, ctx);
	case ke_list_suspended: return DOUBLE_LINKED_LIST_MEMBER(ke, suspended, ctx);
	case ke_list_alloc: return DOUBLE_LINKED_LIST_MEMBER(ke, alloc, ctx);
	}
	return 0;
}


/* Return a counter of microseconds. */
long long ke_timer()
{
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return (long long) tv.tv_sec * 1000000 + tv.tv_usec - ke_init_time;
}


/* Schedule a call to 'ke_process_events' */
void ke_process_events_schedule()
{
	pthread_mutex_lock(&ke->process_events_mutex);
	ke->process_events_force = 1;
	pthread_mutex_unlock(&ke->process_events_mutex);
}


/* Function that suspends the host thread waiting for an event to occur.
 * When the event finally occurs (i.e., before the function finishes, a
 * call to 'ke_process_events' is scheduled.
 * The argument 'arg' is the associated guest context. */
void *ke_host_thread_suspend(void *arg)
{
	struct ctx_t *ctx = (struct ctx_t *) arg;
	uint64_t now = ke_timer();

	/* Detach this thread - we don't want the parent to have to join it to release
	 * its resources. The thread termination can be observed by atomically checking
	 * the 'ctx->host_thread_suspend_active' flag. */
	pthread_detach(pthread_self());

	/* Context suspended in 'poll' system call */
	if (ctx_get_status(ctx, ctx_nanosleep)) {
		
		uint64_t timeout;
		
		/* Calculate remaining sleep time in microseconds */
		timeout = ctx->wakeup_time > now ? ctx->wakeup_time - now : 0;
		usleep(timeout);
	
	} else if (ctx_get_status(ctx, ctx_poll)) {

		struct file_desc_t *fd;
		struct pollfd host_fds;
		int err, timeout;
		
		/* Get file descriptor */
		fd = file_desc_table_entry_get(ctx->file_desc_table, ctx->wakeup_fd);
		if (!fd)
			fatal("syscall 'poll': invalid 'wakeup_fd'");

		/* Calculate timeout for host call in milliseconds from now */
		if (!ctx->wakeup_time)
			timeout = -1;
		else if (ctx->wakeup_time < now)
			timeout = 0;
		else
			timeout = (ctx->wakeup_time - now) / 1000;

		/* Perform blocking host 'poll' */
		host_fds.fd = fd->host_fd;
		host_fds.events = ((ctx->wakeup_events & 4) ? POLLOUT : 0) | ((ctx->wakeup_events & 1) ? POLLIN : 0);
		err = poll(&host_fds, 1, timeout);
		if (err < 0)
			fatal("syscall 'poll': unexpected error in host 'poll'");
	
	} else if (ctx_get_status(ctx, ctx_read)) {
		
		struct file_desc_t *fd;
		struct pollfd host_fds;
		int err;

		/* Get file descriptor */
		fd = file_desc_table_entry_get(ctx->file_desc_table, ctx->wakeup_fd);
		if (!fd)
			fatal("syscall 'read': invalid 'wakeup_fd'");

		/* Perform blocking host 'poll' */
		host_fds.fd = fd->host_fd;
		host_fds.events = POLLIN;
		err = poll(&host_fds, 1, -1);
		if (err < 0)
			fatal("syscall 'read': unexpected error in host 'poll'");
	
	} else if (ctx_get_status(ctx, ctx_write)) {
		
		struct file_desc_t *fd;
		struct pollfd host_fds;
		int err;

		/* Get file descriptor */
		fd = file_desc_table_entry_get(ctx->file_desc_table, ctx->wakeup_fd);
		if (!fd)
			fatal("syscall 'write': invalid 'wakeup_fd'");

		/* Perform blocking host 'poll' */
		host_fds.fd = fd->host_fd;
		host_fds.events = POLLOUT;
		err = poll(&host_fds, 1, -1);
		if (err < 0)
			fatal("syscall 'write': unexpected error in host 'write'");

	}

	/* Event occurred - thread finishes */
	pthread_mutex_lock(&ke->process_events_mutex);
	ke->process_events_force = 1;
	ctx->host_thread_suspend_active = 0;
	pthread_mutex_unlock(&ke->process_events_mutex);
	return NULL;
}


/* Function that suspends the host thread waiting for a timer to expire,
 * and then schedules a call to 'ke_process_events'. */
void *ke_host_thread_timer(void *arg)
{
	struct ctx_t *ctx = (struct ctx_t *) arg;
	uint64_t now = ke_timer();
	struct timespec ts;
	uint64_t sleep_time;  /* In usec */

	/* Detach this thread - we don't want the parent to have to join it to release
	 * its resources. The thread termination can be observed by thread-safely checking
	 * the 'ctx->host_thread_timer_active' flag. */
	pthread_detach(pthread_self());

	/* Calculate sleep time, and sleep only if it is greater than 0 */
	if (ctx->host_thread_timer_wakeup > now) {
		sleep_time = ctx->host_thread_timer_wakeup - now;
		ts.tv_sec = sleep_time / 1000000;
		ts.tv_nsec = (sleep_time % 1000000) * 1000;  /* nsec */
		nanosleep(&ts, NULL);
	}

	/* Timer expired, schedule call to 'ke_process_events' */
	pthread_mutex_lock(&ke->process_events_mutex);
	ke->process_events_force = 1;
	ctx->host_thread_timer_active = 0;
	pthread_mutex_unlock(&ke->process_events_mutex);
	return NULL;
}


/* Check for events detected in spawned host threads, like waking up contexts or
 * sending signals.
 * The list is only processed if flag 'ke->process_events_force' is set. */
void ke_process_events()
{
	struct ctx_t *ctx, *next;
	uint64_t now = ke_timer();
	
	/* Check if events need actually be checked. */
	pthread_mutex_lock(&ke->process_events_mutex);
	if (!ke->process_events_force)
	{
		pthread_mutex_unlock(&ke->process_events_mutex);
		return;
	}
	
	/* By default, no subsequent call to 'ke_process_events' is assumed */
	ke->process_events_force = 0;

	/*
	 * LOOP 1
	 * Look at the list of suspended contexts and try to find
	 * one that needs to be waken up.
	 */
	for (ctx = ke->suspended_list_head; ctx; ctx = next)
	{
		/* Save next */
		next = ctx->suspended_list_next;

		/* Context is suspended in 'nanosleep' system call. */
		if (ctx_get_status(ctx, ctx_nanosleep))
		{
			uint32_t rmtp = ctx->regs->ecx;
			uint64_t zero = 0;
			uint32_t sec, usec;
			uint64_t diff;

			/* If 'ke_host_thread_suspend' is still running for this context, do nothing. */
			if (ctx->host_thread_suspend_active)
				continue;

			/* Timeout expired */
			if (ctx->wakeup_time <= now)
			{
				if (rmtp)
					mem_write(ctx->mem, rmtp, 8, &zero);
				sys_debug("syscall 'nanosleep' - continue (pid %d)\n", ctx->pid);
				sys_debug("  return=0x%x\n", ctx->regs->eax);
				ctx_clear_status(ctx, ctx_suspended | ctx_nanosleep);
				continue;
			}

			/* Context received a signal */
			if (ctx->signal_mask_table->pending & ~ctx->signal_mask_table->blocked)
			{
				if (rmtp)
				{
					diff = ctx->wakeup_time - now;
					sec = diff / 1000000;
					usec = diff % 1000000;
					mem_write(ctx->mem, rmtp, 4, &sec);
					mem_write(ctx->mem, rmtp + 4, 4, &usec);
				}
				ctx->regs->eax = -EINTR;
				sys_debug("syscall 'nanosleep' - interrupted by signal (pid %d)\n", ctx->pid);
				ctx_clear_status(ctx, ctx_suspended | ctx_nanosleep);
				continue;
			}

			/* No event available, launch 'ke_host_thread_suspend' again */
			ctx->host_thread_suspend_active = 1;
			if (pthread_create(&ctx->host_thread_suspend, NULL, ke_host_thread_suspend, ctx))
				fatal("syscall 'poll': could not create child thread");
			continue;
		}

		/* Context suspended in 'rt_sigsuspend' system call */
		if (ctx_get_status(ctx, ctx_sigsuspend))
		{
			/* Context received a signal */
			if (ctx->signal_mask_table->pending & ~ctx->signal_mask_table->blocked)
			{
				signal_handler_check_intr(ctx);
				ctx->signal_mask_table->blocked = ctx->signal_mask_table->backup;
				sys_debug("syscall 'rt_sigsuspend' - interrupted by signal (pid %d)\n", ctx->pid);
				ctx_clear_status(ctx, ctx_suspended | ctx_sigsuspend);
				continue;
			}

			/* No event available. The context will never awake on its own, so no
			 * 'ke_host_thread_suspend' is necessary. */
			continue;
		}

		/* Context suspended in 'poll' system call */
		if (ctx_get_status(ctx, ctx_poll))
		{
			uint32_t prevents = ctx->regs->ebx + 6;
			uint16_t revents = 0;
			struct file_desc_t *fd;
			struct pollfd host_fds;
			int err;

			/* If 'ke_host_thread_suspend' is still running for this context, do nothing. */
			if (ctx->host_thread_suspend_active)
				continue;

			/* Get file descriptor */
			fd = file_desc_table_entry_get(ctx->file_desc_table, ctx->wakeup_fd);
			if (!fd)
				fatal("syscall 'poll': invalid 'wakeup_fd'");

			/* Context received a signal */
			if (ctx->signal_mask_table->pending & ~ctx->signal_mask_table->blocked)
			{
				signal_handler_check_intr(ctx);
				sys_debug("syscall 'poll' - interrupted by signal (pid %d)\n", ctx->pid);
				ctx_clear_status(ctx, ctx_suspended | ctx_poll);
				continue;
			}

			/* Perform host 'poll' call */
			host_fds.fd = fd->host_fd;
			host_fds.events = ((ctx->wakeup_events & 4) ? POLLOUT : 0) | ((ctx->wakeup_events & 1) ? POLLIN : 0);
			err = poll(&host_fds, 1, 0);
			if (err < 0)
				fatal("syscall 'poll': unexpected error in host 'poll'");

			/* POLLOUT event available */
			if (ctx->wakeup_events & host_fds.revents & POLLOUT)
			{
				revents = POLLOUT;
				mem_write(ctx->mem, prevents, 2, &revents);
				ctx->regs->eax = 1;
				sys_debug("syscall poll - continue (pid %d) - POLLOUT occurred in file\n", ctx->pid);
				sys_debug("  retval=%d\n", ctx->regs->eax);
				ctx_clear_status(ctx, ctx_suspended | ctx_poll);
				continue;
			}

			/* POLLIN event available */
			if (ctx->wakeup_events & host_fds.revents & POLLIN)
			{
				revents = POLLIN;
				mem_write(ctx->mem, prevents, 2, &revents);
				ctx->regs->eax = 1;
				sys_debug("syscall poll - continue (pid %d) - POLLIN occurred in file\n", ctx->pid);
				sys_debug("  retval=%d\n", ctx->regs->eax);
				ctx_clear_status(ctx, ctx_suspended | ctx_poll);
				continue;
			}

			/* Timeout expired */
			if (ctx->wakeup_time && ctx->wakeup_time < now)
			{
				revents = 0;
				mem_write(ctx->mem, prevents, 2, &revents);
				sys_debug("syscall poll - continue (pid %d) - time out\n", ctx->pid);
				sys_debug("  return=0x%x\n", ctx->regs->eax);
				ctx_clear_status(ctx, ctx_suspended | ctx_poll);
				continue;
			}

			/* No event available, launch 'ke_host_thread_suspend' again */
			ctx->host_thread_suspend_active = 1;
			if (pthread_create(&ctx->host_thread_suspend, NULL, ke_host_thread_suspend, ctx))
				fatal("syscall 'poll': could not create child thread");
			continue;
		}


		/* Context suspended in a 'write' system call  */
		if (ctx_get_status(ctx, ctx_write))
		{
			struct file_desc_t *fd;
			int count, err;
			uint32_t pbuf;
			void *buf;
			struct pollfd host_fds;

			/* If 'ke_host_thread_suspend' is still running for this context, do nothing. */
			if (ctx->host_thread_suspend_active)
				continue;

			/* Context received a signal */
			if (ctx->signal_mask_table->pending & ~ctx->signal_mask_table->blocked)
			{
				signal_handler_check_intr(ctx);
				sys_debug("syscall 'write' - interrupted by signal (pid %d)\n", ctx->pid);
				ctx_clear_status(ctx, ctx_suspended | ctx_write);
				continue;
			}

			/* Get file descriptor */
			fd = file_desc_table_entry_get(ctx->file_desc_table, ctx->wakeup_fd);
			if (!fd)
				fatal("syscall 'write': invalid 'wakeup_fd'");

			/* Check if data is ready in file by polling it */
			host_fds.fd = fd->host_fd;
			host_fds.events = POLLOUT;
			err = poll(&host_fds, 1, 0);
			if (err < 0)
				fatal("syscall 'write': unexpected error in host 'poll'");

			/* If data is ready in the file, wake up context */
			if (host_fds.revents) {
				pbuf = ctx->regs->ecx;
				count = ctx->regs->edx;
				buf = malloc(count);
				mem_read(ctx->mem, pbuf, count, buf);

				count = write(fd->host_fd, buf, count);
				if (count < 0)
					fatal("syscall 'write': unexpected error in host 'write'");

				ctx->regs->eax = count;
				free(buf);

				sys_debug("syscall write - continue (pid %d)\n", ctx->pid);
				sys_debug("  return=0x%x\n", ctx->regs->eax);
				ctx_clear_status(ctx, ctx_suspended | ctx_write);
				continue;
			}

			/* Data is not ready to be written - launch 'ke_host_thread_suspend' again */
			ctx->host_thread_suspend_active = 1;
			if (pthread_create(&ctx->host_thread_suspend, NULL, ke_host_thread_suspend, ctx))
				fatal("syscall 'write': could not create child thread");
			continue;
		}

		/* Context suspended in 'read' system call */
		if (ctx_get_status(ctx, ctx_read))
		{
			struct file_desc_t *fd;
			uint32_t pbuf;
			int count, err;
			void *buf;
			struct pollfd host_fds;

			/* If 'ke_host_thread_suspend' is still running for this context, do nothing. */
			if (ctx->host_thread_suspend_active)
				continue;

			/* Context received a signal */
			if (ctx->signal_mask_table->pending & ~ctx->signal_mask_table->blocked)
			{
				signal_handler_check_intr(ctx);
				sys_debug("syscall 'read' - interrupted by signal (pid %d)\n", ctx->pid);
				ctx_clear_status(ctx, ctx_suspended | ctx_read);
				continue;
			}

			/* Get file descriptor */
			fd = file_desc_table_entry_get(ctx->file_desc_table, ctx->wakeup_fd);
			if (!fd)
				fatal("syscall 'read': invalid 'wakeup_fd'");

			/* Check if data is ready in file by polling it */
			host_fds.fd = fd->host_fd;
			host_fds.events = POLLIN;
			err = poll(&host_fds, 1, 0);
			if (err < 0)
				fatal("syscall 'read': unexpected error in host 'poll'");

			/* If data is ready, perform host 'read' call and wake up */
			if (host_fds.revents)
			{
				pbuf = ctx->regs->ecx;
				count = ctx->regs->edx;
				buf = malloc(count);
				
				count = read(fd->host_fd, buf, count);
				if (count < 0)
					fatal("syscall 'read': unexpected error in host 'read'");

				ctx->regs->eax = count;
				mem_write(ctx->mem, pbuf, count, buf);
				free(buf);

				sys_debug("syscall 'read' - continue (pid %d)\n", ctx->pid);
				sys_debug("  return=0x%x\n", ctx->regs->eax);
				ctx_clear_status(ctx, ctx_suspended | ctx_read);
				continue;
			}

			/* Data is not ready. Launch 'ke_host_thread_suspend' again */
			ctx->host_thread_suspend_active = 1;
			if (pthread_create(&ctx->host_thread_suspend, NULL, ke_host_thread_suspend, ctx))
				fatal("syscall 'read': could not create child thread");
			continue;
		}

		/* Context suspended in a 'waitpid' system call */
		if (ctx_get_status(ctx, ctx_waitpid))
		{
			struct ctx_t *child;
			uint32_t pstatus;

			/* A zombie child is available to 'waitpid' it */
			child = ctx_get_zombie(ctx, ctx->wakeup_pid);
			if (child)
			{
				/* Continue with 'waitpid' system call */
				pstatus = ctx->regs->ecx;
				ctx->regs->eax = child->pid;
				if (pstatus)
					mem_write(ctx->mem, pstatus, 4, &child->exit_code);
				ctx_set_status(child, ctx_finished);

				sys_debug("syscall waitpid - continue (pid %d)\n", ctx->pid);
				sys_debug("  return=0x%x\n", ctx->regs->eax);
				ctx_clear_status(ctx, ctx_suspended | ctx_waitpid);
				continue;
			}

			/* No event available. Since this context won't wake up on its own, no
			 * 'ke_host_thread_suspend' is needed. */
			continue;
		}
	}


	/*
	 * LOOP 2
	 * Check list of all contexts for expired timers.
	 */
	for (ctx = ke->context_list_head; ctx; ctx = ctx->context_list_next)
	{
		int sig[3] = { 14, 26, 27 };  /* SIGALRM, SIGVTALRM, SIGPROF */
		int i;

		/* If there is already a 'ke_host_thread_timer' running, do nothing. */
		if (ctx->host_thread_timer_active)
			continue;

		/* Check for any expired 'itimer': itimer_value < now
		 * In this case, send corresponding signal to process.
		 * Then calculate next 'itimer' occurrence: itimer_value = now + itimer_interval */
		for (i = 0; i < 3; i++ )
		{
			/* Timer inactive or not expired yet */
			if (!ctx->itimer_value[i] || ctx->itimer_value[i] > now)
				continue;

			/* Timer expired - send a signal.
			 * The target process might be suspended, so the host thread is canceled, and a new
			 * call to 'ke_process_events' is scheduled. Since 'ke_process_events_mutex' is
			 * already locked, the thread-unsafe version of 'ctx_host_thread_suspend_cancel' is used. */
			__ctx_host_thread_suspend_cancel(ctx);
			ke->process_events_force = 1;
			sim_sigset_add(&ctx->signal_mask_table->pending, sig[i]);

			/* Calculate next occurrence */
			ctx->itimer_value[i] = 0;
			if (ctx->itimer_interval[i])
				ctx->itimer_value[i] = now + ctx->itimer_interval[i];
		}

		/* Calculate the time when next wakeup occurs. */
		ctx->host_thread_timer_wakeup = 0;
		for (i = 0; i < 3; i++)
		{
			if (!ctx->itimer_value[i])
				continue;
			assert(ctx->itimer_value[i] >= now);
			if (!ctx->host_thread_timer_wakeup || ctx->itimer_value[i] < ctx->host_thread_timer_wakeup)
				ctx->host_thread_timer_wakeup = ctx->itimer_value[i];
		}

		/* If a new timer was set, launch ke_host_thread_timer' again */
		if (ctx->host_thread_timer_wakeup)
		{
			ctx->host_thread_timer_active = 1;
			if (pthread_create(&ctx->host_thread_timer, NULL, ke_host_thread_timer, ctx))
				fatal("%s: could not create child thread", __FUNCTION__);
		}
	}


	/*
	 * LOOP 3
	 * Process pending signals in running contexts to launch signal handlers
	 */
	for (ctx = ke->running_list_head; ctx; ctx = ctx->running_list_next)
	{
		signal_handler_check(ctx);
	}

	
	/* Unlock */
	pthread_mutex_unlock(&ke->process_events_mutex);
}




/*
 * Functional simulation loop
 */


/* Signal handler while functional simulation loop is running */
static void ke_signal_handler(int signum)
{
	switch (signum)
	{
	
	case SIGINT:
		if (ke_sim_finish)
			abort();
		ke_sim_finish = ke_sim_finish_signal;
		fprintf(stderr, "SIGINT received\n");
		break;
	
	case SIGABRT:
		signal(SIGABRT, SIG_DFL);
		fprintf(stderr, "Aborted\n");
		isa_dump(stderr);
		ke_dump(stderr);
		exit(1);
		break;
	}
}


/* CPU Functional simulation loop */
void ke_run(void)
{
	struct ctx_t *ctx;
	uint64_t cycle = 0;

	/* Install signal handlers */
	signal(SIGINT, &ke_signal_handler);
	signal(SIGABRT, &ke_signal_handler);

	/* Functional simulation loop */
	for (;;)
	{
		/* Stop if all contexts finished */
		if (ke->finished_list_count >= ke->context_list_count)
			ke_sim_finish = ke_sim_finish_ctx;

		/* Stop if maximum number of CPU instructions exceeded */
		if (ke_max_inst && ke->inst_count >= ke_max_inst)
			ke_sim_finish = ke_sim_finish_max_cpu_inst;

		/* Stop if maximum number of cycles exceeded */
		if (ke_max_cycles && cycle >= ke_max_cycles)
			ke_sim_finish = ke_sim_finish_max_cpu_cycles;

		/* Stop if maximum time exceeded (check only every 10k cycles) */
		if (ke_max_time && !(cycle % 10000) && ke_timer() > ke_max_time * 1000000)
			ke_sim_finish = ke_sim_finish_max_time;

		/* Stop if any previous reason met */
		if (ke_sim_finish)
			break;

		/* Next cycle */
		cycle++;

		/* Run an instruction from every running process */
		for (ctx = ke->running_list_head; ctx; ctx = ctx->running_list_next)
			ctx_execute_inst(ctx);
	
		/* Free finished contexts */
		while (ke->finished_list_head)
			ctx_free(ke->finished_list_head);
	
		/* Process list of suspended contexts */
		ke_process_events();
	}

	/* Restore signal handlers */
	signal(SIGABRT, SIG_DFL);
	signal(SIGINT, SIG_DFL);
}


/* CPU disassembler */
void ke_disasm(char *file_name)
{
	struct elf_file_t *elf_file;
	struct elf_section_t *section;
	struct elf_buffer_t *buffer;
	struct elf_symbol_t *symbol;

	struct x86_inst_t inst;
	int curr_sym;
	int i;

	/* Open ELF file */
	x86_disasm_init();
	elf_file = elf_file_create_from_path(file_name);

	/* Read sections */
	for (i = 0; i < list_count(elf_file->section_list); i++)
	{
		/* Get section and skip if it does not contain code */
		section = list_get(elf_file->section_list, i);
		if (!(section->header->sh_flags & SHF_EXECINSTR))
			continue;
		buffer = &section->buffer;
		
		/* Title */
		printf("**\n** Disassembly for section '%s'\n**\n\n", section->name);

		/* Disassemble */
		curr_sym = 0;
		symbol = list_get(elf_file->symbol_table, curr_sym);
		while (buffer->pos < buffer->size)
		{
			uint32_t eip;
			char str[MAX_STRING_SIZE];

			/* Read instruction */
			eip = section->header->sh_addr + buffer->pos;
			x86_disasm(elf_buffer_tell(buffer), eip, &inst);
			if (inst.size)
			{
				elf_buffer_read(buffer, NULL, inst.size);
				x86_inst_dump_buf(&inst, str, MAX_STRING_SIZE);
			}
			else
			{
				elf_buffer_read(buffer, NULL, 1);
				strcpy(str, "???");
			}

			/* Symbol */
			while (symbol && symbol->value < eip)
			{
				curr_sym++;
				symbol = list_get(elf_file->symbol_table, curr_sym);
			}
			if (symbol && symbol->value == eip)
				printf("\n%08x <%s>:\n", eip, symbol->name);

			/* Print */
			printf("%8x:  %s\n", eip, str);
		}

		/* Pad */
		printf("\n\n");
	}

	/* Free ELF */
	elf_file_free(elf_file);
	x86_disasm_done();

	/* End */
	mhandle_done();
	exit(0);
}

