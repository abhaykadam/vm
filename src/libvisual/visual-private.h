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


#ifndef MEMVISUAL_PRIVATE_H
#define MEMVISUAL_PRIVATE_H

#include <assert.h>
#include <math.h>
#include <gtk/gtk.h>

#include <hash-table.h>
#include <linked-list.h>
#include <list.h>
#include <misc.h>
#include <stdlib.h>
#include <visual.h>



/*
 * Trace File
 */

struct trace_file_t;

struct trace_file_t *trace_file_create(char *file_name);
void trace_file_free(struct trace_file_t *file);


struct trace_line_t;

struct trace_line_t *trace_line_create_from_file(FILE *f);
struct trace_line_t *trace_line_create_from_trace_file(struct trace_file_t *f);
void trace_line_free(struct trace_line_t *line);

void trace_line_dump(struct trace_line_t *line, FILE *f);
void trace_line_dump_plain_text(struct trace_line_t *line, FILE *f);

long int trace_line_get_offset(struct trace_line_t *line);

char *trace_line_get_command(struct trace_line_t *line);
char *trace_line_get_symbol_value(struct trace_line_t *line, char *symbol_name);
int trace_line_get_symbol_value_int(struct trace_line_t *line, char *symbol_name);
long long trace_line_get_symbol_value_long_long(struct trace_line_t *line, char *symbol_name);
unsigned int trace_line_get_symbol_value_hex(struct trace_line_t *line, char *symbol_name);




/*
 * State File
 */

struct state_file_t;

typedef void (*state_file_write_checkpoint_func_t)(void *user_data, FILE *f);
typedef void (*state_file_read_checkpoint_func_t)(void *user_data, FILE *f);
typedef void (*state_file_process_trace_line_func_t)(void *user_data, struct trace_line_t *trace_line);
typedef void (*state_file_refresh_func_t)(void *user_data);

#define STATE_FILE_FOR_EACH_HEADER(state_file, trace_line) \
	for ((trace_line) = state_file_header_first((state_file)); \
	(trace_line); (trace_line) = state_file_header_next((state_file)))

struct state_file_t *state_file_create(char *trace_file_name);
void state_file_free(struct state_file_t *file);

long long state_file_get_num_cycles(struct state_file_t *file);
long long state_file_get_cycle(struct state_file_t *file);

void state_file_create_checkpoints(struct state_file_t *file);

void state_file_new_category(struct state_file_t *file, char *name,
	state_file_read_checkpoint_func_t read_checkpoint_func,
	state_file_write_checkpoint_func_t write_checkpoint_func,
	state_file_refresh_func_t refresh_func,
	void *user_data);
void state_file_new_command(struct state_file_t *file, char *command_name,
	state_file_process_trace_line_func_t process_trace_line_func,
	void *user_data);

struct trace_line_t *state_file_header_first(struct state_file_t *file);
struct trace_line_t *state_file_header_next(struct state_file_t *file);

struct trace_line_t *state_file_trace_line_first(struct state_file_t *file, long long cycle);
struct trace_line_t *state_file_trace_line_next(struct state_file_t *file);

void state_file_refresh(struct state_file_t *file);
void state_file_go_to_cycle(struct state_file_t *file, long long cycle);



/*
 * Memory System
 */

struct visual_mem_system_t
{
	struct hash_table_t *mod_table;
	struct hash_table_t *net_table;
	struct hash_table_t *access_table;
	struct list_t *mod_level_list;
};

void visual_mem_system_init(void);
void visual_mem_system_done(void);




/*
 * Memory Module Access
 */

struct visual_mod_access_t
{
	char *name;
	char *state;

	unsigned int address;

	long long creation_cycle;
	long long state_update_cycle;
};

struct visual_mod_access_t *visual_mod_access_create(char *name, unsigned int address);
void visual_mod_access_free(struct visual_mod_access_t *access);

void visual_mod_access_set_state(struct visual_mod_access_t *access, char *state);

void visual_mod_access_read_checkpoint(struct visual_mod_access_t *access, FILE *f);
void visual_mod_access_write_checkpoint(struct visual_mod_access_t *access, FILE *f);

void visual_mod_access_get_name_short(char *access_name, char *buf, int size);
void visual_mod_access_get_name_long(char *access_name, char *buf, int size);
void visual_mod_access_get_desc(char *access_name, char *buf, int size);




/*
 * Memory Module
 */

#define VISUAL_MOD_DIR_ENTRY_SHARERS_SIZE(mod) (((mod)->num_sharers + 7) / 8)
#define VISUAL_MOD_DIR_ENTRY_SIZE(mod) (sizeof(struct visual_mod_dir_entry_t) + \
	VISUAL_MOD_DIR_ENTRY_SHARERS_SIZE((mod)))

struct visual_mod_dir_entry_t
{
	int owner;
	int num_sharers;

	/* Bit map of sharers (last field in variable-size structure) */
	unsigned char sharers[0];
};

struct visual_mod_block_t
{
	struct visual_mod_t *mod;

	int set;
	int way;
	int state;
	unsigned int tag;

	struct linked_list_t *access_list;

	struct visual_mod_dir_entry_t *dir_entries;
};

struct visual_mod_t
{
	char *name;

	int num_sets;
	int assoc;
	int block_size;
	int sub_block_size;
	int num_sub_blocks;
	int num_sharers;
	int level;

	int high_net_node_index;
	int low_net_node_index;

	struct visual_net_t *high_net;
	struct visual_net_t *low_net;

	struct visual_mod_block_t *blocks;

	struct hash_table_t *access_table;
};

struct visual_mod_t *visual_mod_create(struct trace_line_t *trace_line);
void visual_mod_free(struct visual_mod_t *mod);

void visual_mod_add_access(struct visual_mod_t *mod, int set, int way,
	struct visual_mod_access_t *access);
struct visual_mod_access_t *visual_mod_find_access(struct visual_mod_t *mod,
	int set, int way, char *access_name);
struct visual_mod_access_t *visual_mod_remove_access(struct visual_mod_t *mod,
	int set, int way, char *access_name);
struct linked_list_t *visual_mod_get_access_list(struct visual_mod_t *mod,
	int set, int way);

void visual_mod_block_set(struct visual_mod_t *mod, int set, int way,
	unsigned int tag, char *state);
int visual_mod_block_get_num_sharers(struct visual_mod_t *mod, int set, int way);

void visual_mod_read_checkpoint(struct visual_mod_t *mod, FILE *f);
void visual_mod_write_checkpoint(struct visual_mod_t *mod, FILE *f);

struct visual_mod_dir_entry_t *visual_mod_dir_entry_get(struct visual_mod_t *mod,
	int set, int way, int sub_block);
void visual_mod_dir_entry_set_sharer(struct visual_mod_t *mod,
	int x, int y, int z, int sharer);
void visual_mod_dir_entry_clear_sharer(struct visual_mod_t *mod,
	int x, int y, int z, int sharer);
void visual_mod_dir_entry_clear_all_sharers(struct visual_mod_t *mod,
	int x, int y, int z);
int visual_mod_dir_entry_is_sharer(struct visual_mod_t *mod,
	int x, int y, int z, int sharer);
void visual_mod_dir_entry_set_owner(struct visual_mod_t *mod,
	int x, int y, int z, int owner);

void visual_mod_dir_entry_read_checkpoint(struct visual_mod_t *mod, int x, int y, int z, FILE *f);
void visual_mod_dir_entry_write_checkpoint(struct visual_mod_t *mod, int x, int y, int z, FILE *f);




/*
 * Network
 */

struct visual_net_t
{
	char *name;

	struct list_t *node_list;
};

struct visual_net_t *visual_net_create(struct trace_line_t *trace_line);
void visual_net_free(struct visual_net_t *net);

void visual_net_attach_mod(struct visual_net_t *net, struct visual_mod_t *mod, int node_index);
struct visual_mod_t *visual_net_get_mod(struct visual_net_t *net, int node_index);




/*
 * Visual Memory System Widget
 */

struct visual_mem_system_widget_t;

struct visual_mem_system_widget_t *visual_mem_system_widget_create(void);
void visual_mem_system_widget_free(struct visual_mem_system_widget_t *widget);

void visual_mem_system_widget_refresh(struct visual_mem_system_widget_t *widget);

GtkWidget *visual_mem_system_widget_get_widget(struct visual_mem_system_widget_t *widget);




/*
 * Visual Module Widget
 */

struct visual_mod_widget_t;

struct visual_mod_widget_t *visual_mod_widget_create(char *name);
void visual_mod_widget_free(struct visual_mod_widget_t *widget);

void visual_mod_widget_refresh(struct visual_mod_widget_t *visual_mod_widget);

GtkWidget *visual_mod_widget_get_widget(struct visual_mod_widget_t *widget);





/*
 * Info Pop-up
 */

struct info_popup_t *info_popup_create(char *text);
void info_popup_free(struct info_popup_t *popup);

void info_popup_show(char *text);



/*
 * Visual List
 */


extern char vlist_image_close_path[MAX_PATH_SIZE];
extern char vlist_image_close_sel_path[MAX_PATH_SIZE];

typedef void (*vlist_get_elem_name_func_t)(void *elem, char *buf, int size);
typedef void (*vlist_get_elem_desc_func_t)(void *elem, char *buf, int size);

#define VLIST_FOR_EACH(list, iter) \
	for ((iter) = 0; (iter) < vlist_count((list)); (iter)++)

struct vlist_t *vlist_create(char *title, int width, int height,
	vlist_get_elem_name_func_t get_elem_name,
	vlist_get_elem_name_func_t get_elem_desc);
void vlist_free(struct vlist_t *vlist);

int vlist_count(struct vlist_t *vlist);
void vlist_add(struct vlist_t *vlist, void *elem);
void *vlist_get(struct vlist_t *vlist, int index);
void *vlist_remove_at(struct vlist_t *vlist, int index);

void vlist_refresh(struct vlist_t *vlist);

GtkWidget *vlist_get_widget(struct vlist_t *vlist);




/*
 * Cycle Bar
 */

extern char cycle_bar_back_single_path[MAX_PATH_SIZE];
extern char cycle_bar_back_double_path[MAX_PATH_SIZE];
extern char cycle_bar_back_triple_path[MAX_PATH_SIZE];

extern char cycle_bar_forward_single_path[MAX_PATH_SIZE];
extern char cycle_bar_forward_double_path[MAX_PATH_SIZE];
extern char cycle_bar_forward_triple_path[MAX_PATH_SIZE];

extern char cycle_bar_go_path[MAX_PATH_SIZE];

struct cycle_bar_t;

typedef void (*cycle_bar_refresh_func_t)(void *user_data, long long cycle);

struct cycle_bar_t *cycle_bar_create(void);
void cycle_bar_free(struct cycle_bar_t *cycle_bar);

void cycle_bar_set_refresh_func(struct cycle_bar_t *cycle_bar,
	cycle_bar_refresh_func_t refresh_func, void *user_data);

GtkWidget *cycle_bar_get_widget(struct cycle_bar_t *cycle_bar);
long long cycle_bar_get_cycle(struct cycle_bar_t *cycle_bar);




/*
 * Global Objects
 */

/* State */
extern struct state_file_t *visual_state_file;
extern struct visual_mem_system_t *visual_mem_system;

/* Widgets */
extern struct cycle_bar_t *visual_cycle_bar;
extern struct visual_mem_system_widget_t *visual_mem_system_widget;


#endif
