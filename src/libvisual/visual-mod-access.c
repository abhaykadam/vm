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

#include <visual-private.h>


/*
 * Public Functions
 */

struct visual_mod_access_t *visual_mod_access_create(char *name, unsigned int address)
{
	struct visual_mod_access_t *access;

	/* Allocate */
	access = calloc(1, sizeof(struct visual_mod_access_t));
	if (!access)
		fatal("%s: out of memory", __FUNCTION__);

	/* Initialize */
	access->name = str_set(access->name, name);
	access->address = address;
	access->creation_cycle = state_file_get_cycle(visual_state_file);

	/* Return */
	return access;
}


void visual_mod_access_free(struct visual_mod_access_t *access)
{
	str_free(access->name);
	str_free(access->state);
	free(access);
}


void visual_mod_access_set_state(struct visual_mod_access_t *access, char *state)
{
	access->state = str_set(access->state, state);
	access->state_update_cycle = state_file_get_cycle(visual_state_file);
}


void visual_mod_access_read_checkpoint(struct visual_mod_access_t *access, FILE *f)
{
	char name[MAX_STRING_SIZE];
	char state[MAX_STRING_SIZE];

	int count;

	/* Read name */
	str_read_from_file(f, name, sizeof name);
	access->name = str_set(access->name, name);

	/* Read address */
	count = fread(&access->address, 1, sizeof access->address, f);
	if (count != sizeof access->address)
		panic("%s: cannot read checkpoint", __FUNCTION__);

	/* Read state */
	str_read_from_file(f, state, sizeof state);
	access->state = str_set(access->state, state);

	/* Read creation cycle */
	count = fread(&access->creation_cycle, 1, sizeof access->creation_cycle, f);
	if (count != sizeof access->creation_cycle)
		panic("%s: cannot read checkpoint", __FUNCTION__);

	/* Read state update cycle */
	count = fread(&access->state_update_cycle, 1, sizeof access->state_update_cycle, f);
	if (count != sizeof access->state_update_cycle)
		panic("%s: cannot read checkpoint", __FUNCTION__);
}


void visual_mod_access_write_checkpoint(struct visual_mod_access_t *access, FILE *f)
{
	int count;

	/* Write name */
	str_write_to_file(f, access->name);

	/* Write address */
	count = fwrite(&access->address, 1, sizeof access->address, f);
	if (count != sizeof access->address)
		panic("%s: cannot write checkpoint", __FUNCTION__);

	/* Write state */
	str_write_to_file(f, access->state);

	/* Write creation cycle */
	count = fwrite(&access->creation_cycle, 1, sizeof access->creation_cycle, f);
	if (count != sizeof access->creation_cycle)
		panic("%s: cannot write checkpoint", __FUNCTION__);

	/* Write state update cycle */
	count = fwrite(&access->state_update_cycle, 1, sizeof access->state_update_cycle, f);
	if (count != sizeof access->state_update_cycle)
		panic("%s: cannot write checkpoint", __FUNCTION__);
}

/* Return the access name in the current cycle set in the state file */
void visual_mod_access_get_name_long(char *access_name, char *buf, int size)
{
	struct visual_mod_access_t *access;

	/* Look for access */
	access = hash_table_get(visual_mem_system->access_table, access_name);
	if (!access)
		panic("%s: %s: invalid access", __FUNCTION__, access_name);

	/* Name */
	str_printf(&buf, &size, "<b>%s</b>", access->name);

	/* State */
	if (access->state && *access->state)
		str_printf(&buf, &size, " (%s:%lld)", access->state,
			state_file_get_cycle(visual_state_file) - access->state_update_cycle);
}


void visual_mod_access_get_name_short(char *access_name, char *buf, int size)
{
	struct visual_mod_access_t *access;

	/* Look for access */
	access = hash_table_get(visual_mem_system->access_table, access_name);
	if (!access)
		panic("%s: %s: invalid access", __FUNCTION__, access_name);

	/* Name */
	str_printf(&buf, &size, "%s", access->name);
}


void visual_mod_access_get_desc(char *access_name, char *buf, int size)
{
	char *title_format_begin = "<span color=\"blue\"><b>";
	char *title_format_end = "</b></span>";

	struct visual_mod_access_t *access;

	struct trace_line_t *trace_line;

	long long cycle;
	long long current_cycle;

	int i;

	/* Look for access */
	access = hash_table_get(visual_mem_system->access_table, access_name);
	if (!access)
		panic("%s: %s: invalid access", __FUNCTION__, access_name);

	/* Title */
	str_printf(&buf, &size, "%sDescription for access %s%s\n\n",
		title_format_begin, access->name, title_format_end);

	/* Fields */
	str_printf(&buf, &size, "%sName:%s %s\n", title_format_begin,
		title_format_end, access->name);
	str_printf(&buf, &size, "%sAddress:%s 0x%x\n", title_format_begin,
		title_format_end, access->address);
	str_printf(&buf, &size, "%sCreation cycle:%s %lld\n", title_format_begin,
		title_format_end, access->creation_cycle);

	/* State */
	current_cycle = state_file_get_cycle(visual_state_file);
	if (access->state && *access->state)
	{
		str_printf(&buf, &size, "%sState:%s %s\n", title_format_begin,
			title_format_end, access->state);
		str_printf(&buf, &size, "%sState update cycle:%s %lld (%lld cycles ago)\n",
			title_format_begin, title_format_end, access->state_update_cycle,
			current_cycle - access->state_update_cycle);
	}

	/* Log header */
	str_printf(&buf, &size, "\n%sState Log:%s\n", title_format_begin, title_format_end);
	str_printf(&buf, &size, "%10s %6s %s\n", "Cycle", "Rel.", "State");
	for (i = 0; i < 50; i++)
		str_printf(&buf, &size, "-");
	str_printf(&buf, &size, "\n");
	cycle = access->creation_cycle;

	/* Log */
	for (trace_line = state_file_trace_line_first(visual_state_file, cycle);
		trace_line; trace_line = state_file_trace_line_next(visual_state_file))
	{
		char *command;
		char *access_name;
		char *state;

		/* Get command */
		command = trace_line_get_command(trace_line);
		access_name = trace_line_get_symbol_value(trace_line, "name");

		/* Access starts */
		if ((!strcmp(command, "mem.new_access") && !strcmp(access_name, access->name)) ||
			(!strcmp(command, "mem.access") && !strcmp(access_name, access->name)))
		{
			state = trace_line_get_symbol_value(trace_line, "state");
			str_printf(&buf, &size, "%10lld %6lld %s\n",
				cycle, cycle - current_cycle, state);
		}

		/* Access ends */
		if (!strcmp(command, "mem.end_access") && !strcmp(access_name, access->name))
			break;

		/* Cycle */
		if (!strcmp(command, "c"))
			cycle = trace_line_get_symbol_value_long_long(trace_line, "clk");
	}
}
