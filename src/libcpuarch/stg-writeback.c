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

#include <cpuarch.h>



void writeback_core(int core)
{
	struct uop_t *uop;
	int thread, recover = 0;

	for (;;)
	{
		/* Pick element from the head of the event queue */
		linked_list_head(CORE.eventq);
		uop = linked_list_get(CORE.eventq);
		if (!uop)
			break;

		/* A memory uop placed in the event queue is always complete.
		 * Other uops are complete when uop->when is equals to current cycle. */
		if (uop->flags & X86_UINST_MEM)
			uop->when = cpu->cycle;
		if (uop->when > cpu->cycle)
			break;
		
		/* Check element integrity */
		assert(uop_exists(uop));
		assert(uop->when == cpu->cycle);
		assert(uop->core == core);
		assert(uop->ready);
		assert(!uop->completed);
		
		/* Extract element from event queue. */
		linked_list_remove(CORE.eventq);
		uop->in_eventq = 0;
		thread = uop->thread;
		
		/* If a mispredicted branch is solved and recovery is configured to be
		 * performed at writeback, schedule it for the end of the iteration. */
		if (cpu_recover_kind == cpu_recover_kind_writeback &&
			(uop->flags & X86_UINST_CTRL) && !uop->specmode &&
			uop->neip != uop->pred_neip)
			recover = 1;

		/* Debug */
		esim_debug("uop action=\"update\", core=%d, seq=%llu,"
			" stg_writeback=1, completed=1\n",
			uop->core, (long long unsigned) uop->di_seq);

		/* Writeback */
		uop->completed = 1;
		rf_write(uop);
		CORE.rf_int_writes += uop->ph_int_odep_count;
		CORE.rf_fp_writes += uop->ph_fp_odep_count;
		CORE.iq_wakeup_accesses++;
		THREAD.rf_int_writes += uop->ph_int_odep_count;
		THREAD.rf_fp_writes += uop->ph_fp_odep_count;
		THREAD.iq_wakeup_accesses++;
		uop_free_if_not_queued(uop);

		/* Recovery. This must be performed at last, because lots of uops might be
		 * freed, which interferes with the temporary extraction from the eventq. */
		if (recover)
			cpu_recover(core, thread);
	}
}


void cpu_writeback()
{
	int core;
	cpu->stage = "writeback";
	FOREACH_CORE
		writeback_core(core);
}

