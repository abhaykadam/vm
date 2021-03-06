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

#include <gpukernel.h>
#include <cpukernel.h>




/*
 * GPU Work-Group
 */


struct gpu_work_group_t *gpu_work_group_create()
{
	struct gpu_work_group_t *work_group;

	work_group = calloc(1, sizeof(struct gpu_work_group_t));
	work_group->local_mem = mem_create();
	work_group->local_mem->safe = 0;
	return work_group;
}


void gpu_work_group_free(struct gpu_work_group_t *work_group)
{
	mem_free(work_group->local_mem);
	free(work_group);
}


int gpu_work_group_get_status(struct gpu_work_group_t *work_group, enum gpu_work_group_status_t status)
{
	return (work_group->status & status) > 0;
}


void gpu_work_group_set_status(struct gpu_work_group_t *work_group, enum gpu_work_group_status_t status)
{
	struct gpu_ndrange_t *ndrange = work_group->ndrange;

	/* Get only the new bits */
	status &= ~work_group->status;

	/* Add work-group to lists */
	if (status & gpu_work_group_pending)
		DOUBLE_LINKED_LIST_INSERT_TAIL(ndrange, pending, work_group);
	if (status & gpu_work_group_running)
		DOUBLE_LINKED_LIST_INSERT_TAIL(ndrange, running, work_group);
	if (status & gpu_work_group_finished)
		DOUBLE_LINKED_LIST_INSERT_TAIL(ndrange, finished, work_group);

	/* Update it */
	work_group->status |= status;
}


void gpu_work_group_clear_status(struct gpu_work_group_t *work_group, enum gpu_work_group_status_t status)
{
	struct gpu_ndrange_t *ndrange = work_group->ndrange;

	/* Get only the bits that are set */
	status &= work_group->status;

	/* Remove work-group from lists */
	if (status & gpu_work_group_pending)
		DOUBLE_LINKED_LIST_REMOVE(ndrange, pending, work_group);
	if (status & gpu_work_group_running)
		DOUBLE_LINKED_LIST_REMOVE(ndrange, running, work_group);
	if (status & gpu_work_group_finished)
		DOUBLE_LINKED_LIST_REMOVE(ndrange, finished, work_group);
	
	/* Update status */
	work_group->status &= ~status;
}


void gpu_work_group_dump(struct gpu_work_group_t *work_group, FILE *f)
{
	struct gpu_ndrange_t *ndrange = work_group->ndrange;
	struct gpu_wavefront_t *wavefront;
	int wavefront_id;

	if (!f)
		return;
	
	fprintf(f, "[ NDRange[%d].WorkGroup[%d] ]\n\n", ndrange->id, work_group->id);
	fprintf(f, "Name = %s\n", work_group->name);
	fprintf(f, "WaveFrontFirst = %d\n", work_group->wavefront_id_first);
	fprintf(f, "WaveFrontLast = %d\n", work_group->wavefront_id_last);
	fprintf(f, "WaveFrontCount = %d\n", work_group->wavefront_count);
	fprintf(f, "WorkItemFirst = %d\n", work_group->work_item_id_first);
	fprintf(f, "WorkItemLast = %d\n", work_group->work_item_id_last);
	fprintf(f, "WorkItemCount = %d\n", work_group->work_item_count);
	fprintf(f, "\n");

	/* Dump wavefronts */
	FOREACH_WAVEFRONT_IN_WORK_GROUP(work_group, wavefront_id) {
		wavefront = ndrange->wavefronts[wavefront_id];
		gpu_wavefront_dump(wavefront, f);
	}
}
