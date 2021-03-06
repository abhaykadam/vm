/*
 *  Multi2Sim Tools
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

#include <gpuvisual-private.h>


/*
 * vgpu Trace
 */

char vgpu_trace_err[MAX_STRING_SIZE];

#define VGPU_STATE_CHECKPOINT_INTERVAL  500

#define VGPU_TRACE_ERROR(MSG) snprintf(vgpu_trace_err, sizeof vgpu_trace_err, "%s:%d: %s", \
	vgpu->trace_file_name, vgpu->trace_line_number, (MSG))

#define VGPU_TRACE_LINE_MAX_TOKENS  30

struct vgpu_trace_line_token_t
{
	char *var;
	char *value;
};

struct vgpu_trace_line_t
{
	char line[MAX_LONG_STRING_SIZE];
	int num_tokens;
	char *command;
	struct vgpu_trace_line_token_t tokens[VGPU_TRACE_LINE_MAX_TOKENS];
};


/*
 * Return values:
 *   0 - OK
 *   1 - End of file reached
 *   2 - Other error - stored in 'vgpu_trace_err'
 */
int vgpu_trace_line_read(struct vgpu_t *vgpu, struct vgpu_trace_line_t *trace_line)
{
	char *line;
	int len;
	struct vgpu_trace_line_token_t *token;

	/* Read line from file */
	do
	{
		line = fgets(trace_line->line, sizeof trace_line->line, vgpu->trace_file);
		if (!line)
			return 1;
		vgpu->trace_line_number++;

		/* Remove spaces and '\n' */
		len = strlen(line);
		if (len >= sizeof trace_line->line - 1)
			fatal("%s: maximum trace line length exceeded", __FUNCTION__);
		while (len > 0 && (line[len - 1] == ' ' || line[len - 1] == '\n'))
			line[len--] = '\0';
		while (len > 0 && line[0] == ' ')
			line++;

	} while (!len);

	/* Get command */
	trace_line->command = line;
	while (isalnum(*line))
		line++;
	while (isspace(*line))
		*line++ = '\0';

	/* Get tokens */
	trace_line->num_tokens = 0;
	while (*line)
	{
		/* New token */
		if (trace_line->num_tokens >= VGPU_TRACE_LINE_MAX_TOKENS) {
			VGPU_TRACE_ERROR("maximum number of tokens exceeded");
			return 2;
		}
		token = &trace_line->tokens[trace_line->num_tokens++];

		/* Get variable */
		token->var = line;
		while (isalnum(*line) || *line == '_' || *line == '.')
			line++;
		while (isspace(*line))
			*line++ = '\0';

		/* An '=' sign is expected here */
		if (*line != '=') {
			VGPU_TRACE_ERROR("wrong syntax");
			return 2;
		}
		while (isspace(*line) || *line == '=')
			*line++ = '\0';

		/* Read value */
		if (*line == '"')
		{
			*line++ = '\0';
			token->value = line;
			while (*line && *line != '"')
				line++;
			if (*line == '"')
				*line++ = '\0';
		}
		else
		{
			token->value = line;
			while (!isspace(*line))
				line++;
		}
		while (isspace(*line))
			*line++ = '\0';
	}

	/* Return success */
	return 0;
}


static int vgpu_trace_line_token_index(struct vgpu_trace_line_t *trace_line, char *var)
{
	struct vgpu_trace_line_token_t *token;
	int i;

	for (i = 0; i < trace_line->num_tokens; i++)
	{
		token = &trace_line->tokens[i];
		if (!strcmp(token->var, var))
			return i;
	}
	return -1;
}


int vgpu_trace_line_token_int(struct vgpu_trace_line_t *trace_line, char *var)
{
	int index;

	index = vgpu_trace_line_token_index(trace_line, var);
	if (index < 0)
		return 0;
	return atoi(trace_line->tokens[index].value);
}


char *vgpu_trace_line_token(struct vgpu_trace_line_t *trace_line, char *var)
{
	int index;

	index = vgpu_trace_line_token_index(trace_line, var);
	if (index < 0)
		return "";
	return trace_line->tokens[index].value;
}


void vgpu_trace_line_read_vliw(struct vgpu_trace_line_t *trace_line, struct vgpu_uop_t *uop)
{
	char vliw_elems[5] = { 'x', 'y', 'z', 'w', 't' };
	char vliw_elem;

	char vliw_elem_name[MAX_STRING_SIZE];
	char str[MAX_LONG_STRING_SIZE];

	char *name_ptr;
	int name_size;

	int i;

	uop->name[0] = '\0';
	name_ptr = uop->name;
	name_size = sizeof uop->name;

	for (i = 0; i < 5; i++)
	{
		/* Get VLIW slot */
		vliw_elem = vliw_elems[i];
		snprintf(str, sizeof str, "inst.%c", vliw_elem);
		snprintf(vliw_elem_name, sizeof vliw_elem_name , "%s", vgpu_trace_line_token(trace_line, str));
		if (!vliw_elem_name[0])
			continue;

		/* Add to instruction name */
		snprintf(uop->vliw_slot[i], sizeof uop->vliw_slot[i], "%s", strtok(vliw_elem_name, " "));
		snprintf(uop->vliw_slot_args[i], sizeof uop->vliw_slot_args[i], "%s", strtok(NULL, ""));
		str_printf(&name_ptr, &name_size, "%c: %s %s   ", vliw_elems[i], uop->vliw_slot[i], uop->vliw_slot_args[i]);
	}
}


/* Return value:
 *   0 - OK
 *   1 - End of trace reached
 *   2 - Format error in line
 */
int vgpu_trace_line_process(struct vgpu_t *vgpu)
{
	struct vgpu_trace_line_t trace_line;
	char *command;
	int err;

	/* Read trace line */
	err = vgpu_trace_line_read(vgpu, &trace_line);
	if (err)
		return err;
	command = trace_line.command;

	/* Command 'cu' */
	if (!strcmp(command, "cu"))
	{
		struct vgpu_compute_unit_t *compute_unit;
		int compute_unit_id;
		char *action;
		
		/* Get compute unit */
		compute_unit_id = vgpu_trace_line_token_int(&trace_line, "cu");
		if (compute_unit_id < 0 || compute_unit_id >= vgpu->num_compute_units) {
			VGPU_TRACE_ERROR("compute unit id out of bounds");
			return 2;
		}
		compute_unit = list_get(vgpu->compute_unit_list, compute_unit_id);

		/* Get action */
		action = vgpu_trace_line_token(&trace_line, "a");

		/* Map work-group */
		if (!strcmp(action, "map"))
		{
			struct vgpu_work_group_t *work_group;
			int work_group_id;

			/* Remove work-group from pending list */
			work_group_id = vgpu_trace_line_token_int(&trace_line, "wg");
			work_group = list_remove_at(vgpu->pending_work_group_list, 0);
			if (!work_group || work_group->id != work_group_id)
			{
				VGPU_TRACE_ERROR("work-group not in head of pending list");
				return 2;
			}

			/* Add work-group to compute unit's list */
			if (list_index_of(compute_unit->work_group_list, work_group) >= 0)
			{
				VGPU_TRACE_ERROR("work-group is already in compute unit");
				return 2;
			}
			list_add(compute_unit->work_group_list, work_group);

			/* Record another mapped work-group */
			assert(vgpu->num_mapped_work_groups == work_group_id);
			vgpu->num_mapped_work_groups++;

			/* Status */
			vgpu_status_write(vgpu, "<b>WG-%d</b> mapped to <b>CU-%d</b>\n",
				work_group->id, compute_unit->id);
		}

		/* Unmap work-group */
		else if (!strcmp(action, "unmap"))
		{
			struct vgpu_work_group_t *work_group;
			int work_group_id;
			int found, i;

			/* Remove work-group from compute unit */
			work_group_id = vgpu_trace_line_token_int(&trace_line, "wg");
			found = 0;
			for (i = 0; i < list_count(compute_unit->work_group_list); i++)
			{
				work_group = list_get(compute_unit->work_group_list, i);
				if (work_group->id == work_group_id)
				{
					found = 1;
					list_remove_at(compute_unit->work_group_list, i);
					break;
				}
			}
			if (!found)
			{
				VGPU_TRACE_ERROR("work-group is not in compute unit");
				return 2;
			}

			/* Add work-group to finished list in order */
			if (list_index_of(vgpu->finished_work_group_list, work_group) >= 0)
			{
				VGPU_TRACE_ERROR("work-group is already in finished list");
				return 2;
			}
			for (i = list_count(vgpu->finished_work_group_list); i > 0; i--)
			{
				struct vgpu_work_group_t *work_group_curr;
				work_group_curr = list_get(vgpu->finished_work_group_list, i - 1);
				if (work_group_curr->id < work_group->id)
					break;
			}
			list_insert(vgpu->finished_work_group_list, i, work_group);

			/* Status */
			vgpu_status_write(vgpu, "<b>WG-%d</b> finished execution in <b>CU-%d</b>\n",
				work_group->id, compute_unit->id);
		}

		/* Unknown action */
		else
		{
			VGPU_TRACE_ERROR("invalid action");
			return 2;
		}
	}

	/* Command 'clk' */
	else if (!strcmp(command, "clk"))
	{
		int cycle;

		cycle = vgpu_trace_line_token_int(&trace_line, "c");
		if (cycle != vgpu->cycle + 1)
		{
			VGPU_TRACE_ERROR("invalid cycle number");
			return 2;
		}
		vgpu->cycle = cycle;
		if (cycle > vgpu->max_cycles)
			vgpu->max_cycles = cycle;
	}

	/* Create a 'uop' */
	else if (!strcmp(command, "cf") || !strcmp(command, "alu") || !strcmp(command, "tex"))
	{
		struct vgpu_compute_unit_t *compute_unit;
		struct vgpu_uop_t *uop;
		char *stage;
		int compute_unit_id;
		int uop_id;

		compute_unit_id = vgpu_trace_line_token_int(&trace_line, "cu");
		if (compute_unit_id < 0 || compute_unit_id >= vgpu->num_compute_units)
		{
			VGPU_TRACE_ERROR("invalid compute unit id");
			return 2;
		}
		compute_unit = list_get(vgpu->compute_unit_list, compute_unit_id);

		/* Stage 'fetch' for any engine */
		stage = vgpu_trace_line_token(&trace_line, "a");
		uop_id = vgpu_trace_line_token_int(&trace_line, "uop");
		if (!strcmp(stage, "fetch"))
		{
			/* Create uop */
			if (vgpu_uop_list_get(compute_unit->uop_list, uop_id))
			{
				VGPU_TRACE_ERROR("uop already exists");
				return 2;
			}
			uop = vgpu_uop_create(uop_id);
			uop->compute_unit_id = compute_unit_id;
			uop->work_group_id = vgpu_trace_line_token_int(&trace_line, "wg");
			uop->wavefront_id = vgpu_trace_line_token_int(&trace_line, "wf");
			str_single_spaces(uop->name, vgpu_trace_line_token(&trace_line, "inst"), sizeof uop->name);
			if (uop_id > compute_unit->max_uops)
				compute_unit->max_uops = uop_id;

			/* Read VLIW instruction */
			if (!strcmp(command, "alu"))
				vgpu_trace_line_read_vliw(&trace_line, uop);

			/* Add it to uop list */
			err = vgpu_uop_list_add(compute_unit->uop_list, uop);
			if (err)
			{
				VGPU_TRACE_ERROR("non-sequential uop id");
				return 2;
			}
		}

		/* Any stage other than 'fetch' for any engine */
		else
		{
			/* Get uop */
			uop = vgpu_uop_list_get(compute_unit->uop_list, uop_id);
			if (!uop)
			{
				VGPU_TRACE_ERROR("invalid uop");
				return 2;
			}
		}

		/* CF Engine */
		if (!strcmp(command, "cf"))
		{
			uop->engine = VGPU_ENGINE_CF;
			uop->stage_cycle = vgpu->cycle;
			if (!strcmp(stage, "fetch"))
			{
				uop->stage = VGPU_STAGE_FETCH;
				vgpu_status_write(vgpu, "<b>CU-%d</b>, <b>WG-%d</b>, <b>WF-%d</b>: "
					"CF uop <span color=\"darkgreen\"><b>I-%d</b></span> - Fetch - "
					"<span color=\"darkgreen\"><b>%s</b></span>\n",
					uop->compute_unit_id, uop->work_group_id, uop->wavefront_id,
					uop->id, uop->name);
			}
			else if (!strcmp(stage, "decode"))
			{
				uop->stage = VGPU_STAGE_DECODE;
				vgpu_status_write(vgpu, "<b>CU-%d</b>, <b>WG-%d</b>, <b>WF-%d</b>: "
					"CF uop <span color=\"darkgreen\"><b>I-%d</b></span> - Decode\n",
					uop->compute_unit_id, uop->work_group_id, uop->wavefront_id, uop->id);
			}
			else if (!strcmp(stage, "execute"))
			{
				uop->stage = VGPU_STAGE_EXECUTE;
				vgpu_status_write(vgpu, "<b>CU-%d</b>, <b>WG-%d</b>, <b>WF-%d</b>: "
					"CF uop <span color=\"darkgreen\"><b>I-%d</b></span> - Execute\n",
					uop->compute_unit_id, uop->work_group_id, uop->wavefront_id, uop->id);
			}
			else if (!strcmp(stage, "complete"))
			{
				uop->stage = VGPU_STAGE_COMPLETE;
				uop->finished = 1;
				vgpu_status_write(vgpu, "<b>CU-%d</b>, <b>WG-%d</b>, <b>WF-%d</b>: "
					"CF uop <span color=\"darkgreen\"><b>I-%d</b></span> - Complete\n",
					uop->compute_unit_id, uop->work_group_id, uop->wavefront_id, uop->id);
			}
			else
			{
				VGPU_TRACE_ERROR("invalid stage");
				return 2;
			}
		}

		/* ALU Engine */
		else if (!strcmp(command, "alu"))
		{
			uop->engine = VGPU_ENGINE_ALU;
			uop->stage_cycle = vgpu->cycle;
			if (!strcmp(stage, "fetch"))
			{
				uop->stage = VGPU_STAGE_FETCH;
				vgpu_status_write(vgpu, "<b>CU-%d</b>, <b>WG-%d</b>, <b>WF-%d</b>: "
					"ALU uop <span color=\"red\"><b>I-%d</b></span> - Fetch - "
					"<span color=\"red\"><b>%s</b></span>\n",
					uop->compute_unit_id, uop->work_group_id, uop->wavefront_id,
					uop->id, uop->name);
			}
			else if (!strcmp(stage, "decode"))
			{
				uop->stage = VGPU_STAGE_DECODE;
				vgpu_status_write(vgpu, "<b>CU-%d</b>, <b>WG-%d</b>, <b>WF-%d</b>: "
					"ALU uop <span color=\"red\"><b>I-%d</b></span> - Decode\n",
					uop->compute_unit_id, uop->work_group_id, uop->wavefront_id, uop->id);
			}
			else if (!strcmp(stage, "read"))
			{
				uop->stage = VGPU_STAGE_READ;
				vgpu_status_write(vgpu, "<b>CU-%d</b>, <b>WG-%d</b>, <b>WF-%d</b>: "
					"ALU uop <span color=\"red\"><b>I-%d</b></span> - Read\n",
					uop->compute_unit_id, uop->work_group_id, uop->wavefront_id, uop->id);
			}
			else if (!strcmp(stage, "exec"))
			{
				uop->stage = VGPU_STAGE_EXECUTE;
				vgpu_status_write(vgpu, "<b>CU-%d</b>, <b>WG-%d</b>, <b>WF-%d</b>: "
					"ALU uop <span color=\"red\"><b>I-%d</b></span> - Execute\n",
					uop->compute_unit_id, uop->work_group_id, uop->wavefront_id, uop->id);
			}
			else if (!strcmp(stage, "write"))
			{
				uop->stage = VGPU_STAGE_WRITE;
				uop->finished = 1;
				vgpu_status_write(vgpu, "<b>CU-%d</b>, <b>WG-%d</b>, <b>WF-%d</b>: "
					"ALU uop <span color=\"red\"><b>I-%d</b></span> - Write\n",
					uop->compute_unit_id, uop->work_group_id, uop->wavefront_id, uop->id);
			}
			else
			{
				VGPU_TRACE_ERROR("invalid stage");
				return 2;
			}
		}

		/* TEX Engine */
		else
		{
			uop->engine = VGPU_ENGINE_TEX;
			uop->stage_cycle = vgpu->cycle;
			if (!strcmp(stage, "fetch"))
			{
				uop->stage = VGPU_STAGE_FETCH;
				vgpu_status_write(vgpu, "<b>CU-%d</b>, <b>WG-%d</b>, <b>WF-%d</b>: "
					"TEX uop <span color=\"blue\"><b>I-%d</b></span> - Fetch - "
					"<span color=\"blue\"><b>%s</b></span>\n",
					uop->compute_unit_id, uop->work_group_id, uop->wavefront_id,
					uop->id, uop->name);
			}
			else if (!strcmp(stage, "decode"))
			{
				uop->stage = VGPU_STAGE_DECODE;
				vgpu_status_write(vgpu, "<b>CU-%d</b>, <b>WG-%d</b>, <b>WF-%d</b>: "
					"TEX uop <span color=\"blue\"><b>I-%d</b></span> - Decode\n",
					uop->compute_unit_id, uop->work_group_id, uop->wavefront_id, uop->id);
			}
			else if (!strcmp(stage, "read"))
			{
				uop->stage = VGPU_STAGE_READ;
				vgpu_status_write(vgpu, "<b>CU-%d</b>, <b>WG-%d</b>, <b>WF-%d</b>: "
					"TEX uop <span color=\"blue\"><b>I-%d</b></span> - Read\n",
					uop->compute_unit_id, uop->work_group_id, uop->wavefront_id, uop->id);
			}
			else if (!strcmp(stage, "write"))
			{
				uop->stage = VGPU_STAGE_WRITE;
				uop->finished = TRUE;
				vgpu_status_write(vgpu, "<b>CU-%d</b>, <b>WG-%d</b>, <b>WF-%d</b>: "
					"TEX uop <span color=\"blue\"><b>I-%d</b></span> - Write\n",
					uop->compute_unit_id, uop->work_group_id, uop->wavefront_id, uop->id);
			}
			else
			{
				VGPU_TRACE_ERROR("invalid stage");
				return 2;
			}
		}
	}

	/* Other command */
	else
	{
		VGPU_TRACE_ERROR("invalid command");
		return 2;
	}

	/* Success */
	return 0;
}


/* Return value
 *   0 - OK
 *   1 - Already in last cycle
 *   2 - Error found in some trace line
 */
int vgpu_trace_next_cycle(struct vgpu_t *vgpu)
{
	int cycle;
	int err;
	long trace_file_pos;
	int i;

	/* Parse commands until the next 'clk' command is found.
	 * This should only happen when skipping the initialization commands before cycle 1.
	 * Otherwise, the current trace line should contain the 'clk' command. */
	cycle = vgpu->cycle;
	do {
		err = vgpu_trace_line_process(vgpu);
		if (err)
			return err;
	} while (vgpu->cycle == cycle);

	/* First thing to do in a new cycle is remove all finished uops from the
	 * compute units' uop lists. */
	for (i = 0; i < vgpu->num_compute_units; i++)
	{
		struct vgpu_compute_unit_t *compute_unit;
		struct list_t *uop_list;
		struct vgpu_uop_t *uop;

		compute_unit = list_get(vgpu->compute_unit_list, i);
		uop_list = compute_unit->uop_list;
		while (list_count(uop_list)) {
			uop = list_get(uop_list, 0);
			if (uop->finished) {
				list_remove_at(uop_list, 0);
				compute_unit->last_completed_uop_id = uop->id;
				vgpu_uop_free(uop);
			} else
				break;
		}
	}

	/* Clear status text */
	vgpu_status_clear(vgpu);

	/* Parse line until the next 'clk' command. When it is found,
	 * go back to it so that next time it will be processed first. */
	cycle = vgpu->cycle;
	do {
		trace_file_pos = ftell(vgpu->trace_file);
		err = vgpu_trace_line_process(vgpu);
		if (err == 2)
			return 2;
	} while (vgpu->cycle == cycle && !err);
	fseek(vgpu->trace_file, trace_file_pos, SEEK_SET);
	vgpu->trace_line_number--;
	vgpu->cycle = cycle;

	/* File position should be at the end, or right at a 'clk' trace line. */
	return 0;
}


/* Return value
 *   0 - OK
 *   1 - Cycle exceeds end of file
 *   2 - Error in trace lines
 */
int vgpu_trace_cycle(struct vgpu_t *vgpu, int cycle)
{
	int err;
	int checkpoint_index;
	int load_checkpoint;
	int distance_from_checkpoint;
	struct vgpu_state_checkpoint_t *checkpoint;

	/* Check if it is better to load a checkpoint or to go to the target
	 * cycle from the current vgpu state. */
	load_checkpoint = TRUE;
	checkpoint_index = cycle / VGPU_STATE_CHECKPOINT_INTERVAL;
	distance_from_checkpoint = cycle - checkpoint_index * VGPU_STATE_CHECKPOINT_INTERVAL;
	if (vgpu->cycle <= cycle && distance_from_checkpoint >= cycle - vgpu->cycle)
		load_checkpoint = FALSE;

	/* Load checkpoint */
	if (load_checkpoint)
	{
		checkpoint = list_get(vgpu->state_checkpoint_list, checkpoint_index);
		if (!checkpoint)
			return 1;
		fseek(vgpu->state_file, checkpoint->state_file_pos, SEEK_SET);
		fseek(vgpu->trace_file, checkpoint->trace_file_pos, SEEK_SET);
		vgpu_load_state(vgpu);
		assert(vgpu->cycle == checkpoint->cycle);
	}

	/* Go to cycle */
	while (vgpu->cycle < cycle)
	{
		err = vgpu_trace_next_cycle(vgpu);
		if (err)
			return err;
	}

	/* Success */
	return 0;
}


/* Return value:
 *   0 - OK
 *   1 - End of trace reached before intro finished
 *   2 - Format error in line
 */
int vgpu_trace_parse_intro(struct vgpu_t *vgpu)
{
	int err;
	long int trace_file_pos;

	struct vgpu_trace_line_t trace_line;
	char *command;

	for (;;)
	{
		/* Get a trace line */
		trace_file_pos = ftell(vgpu->trace_file);
		err = vgpu_trace_line_read(vgpu, &trace_line);

		/* Errors */
		if (err == 1)
			VGPU_TRACE_ERROR("end of file reached in intro");
		if (err)
			return err;
		command = trace_line.command;

		/* Initialization command */
		if (!strcmp(command, "init"))
		{
			struct vgpu_compute_unit_t *compute_unit;
			int i;

			vgpu->num_work_groups = vgpu_trace_line_token_int(&trace_line, "group_count");
			vgpu->num_compute_units = vgpu_trace_line_token_int(&trace_line, "compute_units");
			for (i = 0; i < vgpu->num_compute_units; i++)
			{
				compute_unit = vgpu_compute_unit_create(vgpu, i);
				list_add(vgpu->compute_unit_list, compute_unit);
			}
		}

		/* New element */
		else if (!strcmp(command, "new"))
		{
			char *item;

			/* Get item */
			item = vgpu_trace_line_token(&trace_line, "item");
			
			/* New work-group */
			if (!strcmp(item, "wg"))
			{
				struct vgpu_work_group_t *work_group;
				int id;

				/* Get ID */
				id = vgpu_trace_line_token_int(&trace_line, "id");
				if (id != list_count(vgpu->work_group_list))
				{
					VGPU_TRACE_ERROR("unordered work-group id");
					return 2;
				}

				/* Create work_group */
				work_group = vgpu_work_group_create(id);
				list_add(vgpu->work_group_list, work_group);
				list_add(vgpu->pending_work_group_list, work_group);

				/* Fields */
				work_group->id = id;
				work_group->work_item_id_first = vgpu_trace_line_token_int(&trace_line, "wi_first");
				work_group->work_item_count = vgpu_trace_line_token_int(&trace_line, "wi_count");
				work_group->work_item_id_last = work_group->work_item_id_first +
					work_group->work_item_count - 1;
				work_group->wavefront_id_first = vgpu_trace_line_token_int(&trace_line, "wf_first");
				work_group->wavefront_count = vgpu_trace_line_token_int(&trace_line, "wf_count");
				work_group->wavefront_id_last = work_group->wavefront_id_first +
					work_group->wavefront_count - 1;
			}

			/* New wavefront */
			else if (!strcmp(item, "wf"))
			{
			}

			/* Not recognized */
			else
			{
				VGPU_TRACE_ERROR("invalid value for token 'item'");
				return 2;
			}
		}

		/* Assembly code */
		else if (!strcmp(command, "asm"))
		{
			int inst_idx;
			char *clause;
			char text[MAX_LONG_STRING_SIZE];

			/* Get disassembly line */
			inst_idx = vgpu_trace_line_token_int(&trace_line, "i");
			clause = vgpu_trace_line_token(&trace_line, "cl");
			if (!clause || inst_idx != list_count(vgpu->kernel_source_strings))
			{
				VGPU_TRACE_ERROR("invalid disassembly");
				return 2;
			}

			/* CF clause */
			if (!strcmp(clause, "cf"))
			{
				char *inst;
				int count;
				int loop_idx;

				char *text_ptr = text;
				int text_size = sizeof text;

				int i;

				inst = vgpu_trace_line_token(&trace_line, "inst");
				count = vgpu_trace_line_token_int(&trace_line, "cnt");
				loop_idx = vgpu_trace_line_token_int(&trace_line, "l");

				for (i = 0; i < 3 * loop_idx; i++)
					str_printf(&text_ptr, &text_size, " ");
				str_printf(&text_ptr, &text_size,
					"<span color=\"darkgreen\">%02d <b>%s</b></span>\n", count, inst);
				list_add(vgpu->kernel_source_strings, strdup(text));
			}

			/* ALU clause */
			else if (!strcmp(clause, "alu"))
			{
				char *text_ptr = text;
				int text_size = sizeof text;

				char *slot_name[5] = { "x", "y", "z", "w", "t" };
				char *slot_inst_name[5] = { "inst.x", "inst.y", "inst.z", "inst.w", "inst.t" };
				char *inst;

				int i, j;
				int first_printed_slot = 1;

				int count;
				int loop_idx;

				count = vgpu_trace_line_token_int(&trace_line, "cnt");
				loop_idx = vgpu_trace_line_token_int(&trace_line, "l");

				str_printf(&text_ptr, &text_size, "<span color=\"red\">");
				for (j = 0; j < 5; j++)
				{
					inst = vgpu_trace_line_token(&trace_line, slot_inst_name[j]);
					if (!*inst)
						continue;

					for (i = 0; i < 3 * loop_idx; i++)
						str_printf(&text_ptr, &text_size, " ");

					if (first_printed_slot)
						str_printf(&text_ptr, &text_size, "%4d", count);
					else
						str_printf(&text_ptr, &text_size, "%4s", "");
					first_printed_slot = 0;

					str_printf(&text_ptr, &text_size,
						" <b><span color=\"darkred\">%s</span>: %s</b>\n", slot_name[j], inst);
				}
				str_printf(&text_ptr, &text_size, "</span>");
				list_add(vgpu->kernel_source_strings, strdup(text));
			}

			/* TEX clause */
			else if (!strcmp(clause, "tex"))
			{
				char *text_ptr = text;
				int text_size = sizeof text;

				char *inst;
				int i;

				int count;
				int loop_idx;

				count = vgpu_trace_line_token_int(&trace_line, "cnt");
				loop_idx = vgpu_trace_line_token_int(&trace_line, "l");

				for (i = 0; i < 3 * loop_idx; i++)
					str_printf(&text_ptr, &text_size, " ");

				inst = vgpu_trace_line_token(&trace_line, "inst");
				str_printf(&text_ptr, &text_size, "<span color=\"blue\">");
				str_printf(&text_ptr, &text_size, "%4d", count);
				str_printf(&text_ptr, &text_size, " <b>%s</b>\n", inst);
				str_printf(&text_ptr, &text_size, "</span>");
				list_add(vgpu->kernel_source_strings, strdup(text));
			}

			/* Invalid */
			else
			{
				VGPU_TRACE_ERROR("invalid clause in disassembly");
				return 2;
			}
		}

		/* Command not belonging to intro anymore.
		 * Return to previous position and leave. */
		else
		{
			vgpu->trace_line_number--;
			fseek(vgpu->trace_file, trace_file_pos, SEEK_SET);
			break;
		}
	}

	/* Success */
	return 0;
}


int vgpu_trace_parse(struct vgpu_t *vgpu)
{
	int err;

	/* Parse initial trace line */
	err = vgpu_trace_parse_intro(vgpu);
	if (err)
		return err;

	/* Parse whole file */
	do
	{
		/* Make state checkpoint */
		if (vgpu->cycle % VGPU_STATE_CHECKPOINT_INTERVAL == 0)
		{
			struct vgpu_state_checkpoint_t *checkpoint;
			checkpoint = calloc(1, sizeof(struct vgpu_state_checkpoint_t));
			checkpoint->cycle = vgpu->cycle;
			checkpoint->state_file_pos = ftell(vgpu->state_file);
			checkpoint->trace_file_pos = ftell(vgpu->trace_file);
			list_add(vgpu->state_checkpoint_list, checkpoint);
			vgpu_store_state(vgpu);
		}

		/* Go to next cycle */
		err = vgpu_trace_next_cycle(vgpu);
		if (err == 2)
			return err;
	} while (!err);

	/* Go to first cycle */
	vgpu_trace_cycle(vgpu, 0);

	/* Return success */
	return 0;
}
