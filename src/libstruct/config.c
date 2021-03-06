/*
 *  Libstruct
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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <mhandle.h>
#include <assert.h>
#include <config.h>
#include <hash-table.h>
#include <linked-list.h>

#define MAX_STRING_SIZE		1000
#define HASH_TABLE_SIZE		100

#define ITEM_ALLOWED  ((void *) 1)
#define ITEM_MANDATORY  ((void *) 2)


/* Structure representing a configuration file.
 * The keys in the hash tables are strings representing section/variable names.
 *    If a key is the name of a section, it is represented as the name of the section.
 *    If a key is the name of a variable, it is represented as "<section>\n<var>"
 *
 */
struct config_t
{
	/* Text file name containing configuration */
	char *file_name;
	
	/* Hash table containing present elements
 	 * The values for sections are (void *) 1, while the values for variables are the
	 * actual value represented for that variable in the config file. */
	struct hash_table_t *items;

	/* Hash table containing allowed items.
	 * The keys are strings "<section>\n<variable>".
	 * The values are SECTION_VARIABLE_ALLOWED/SECTION_VARIABLE_MANDATORY */
	struct hash_table_t *allowed_items;
};




/*
 * Private Functions
 */

static void trim(char *dest, char *str)
{
	/* Copy left-trimmed string to 'dest' */
	while (*str == ' ' || *str == '\n')
		str++;
	snprintf(dest, MAX_STRING_SIZE, "%s", str);
	
	/* Create right-trimmed version of 'dest' */
	str = dest + strlen(dest) - 1;
	while (*dest && (*str == ' ' || *str == '\n')) {
		*str = '\0';
		str--;
	}
}


/* Return a section and variable name from a string "<section>\n<var>" or "<secion>". In the
 * latter case, the variable name is returned as an empty string. */
static void get_section_var_from_item(char *item, char *section, char *var)
{
	char *section_brk;

	snprintf(section, MAX_STRING_SIZE, "%s", item);
	section_brk = index(section, '\n');
	if (section_brk) {
		*section_brk = '\0';
		snprintf(var, MAX_STRING_SIZE, "%s", section_brk + 1);
	} else
		*var = '\0';
}


/* Create string "<section>\n<var>" from separate strings.
 * String "<section>" is created if 'var' is NULL or an empty string.
 * Remove any spaces on the left or right of both the section and variable names. */
static void get_item_from_section_var(char *section, char *var, char *item)
{
	char var_trim[MAX_STRING_SIZE];
	char section_trim[MAX_STRING_SIZE];

	assert(section && *section);
	trim(section_trim, section);
	if (var && *var) {
		trim(var_trim, var);
		snprintf(item, MAX_STRING_SIZE, "%s\n%s", section_trim, var_trim);
	} else
		snprintf(item, MAX_STRING_SIZE, "%s", section_trim);
}


/* Return a variable name and its value from a string "<var>=<value>".
 * Both the returned variable and the value are trimmed.
 * If the input string format is wrong, two empty strings are returned. */
static void get_var_value_from_item(char *item, char *var, char *value)
{
	char var_str[MAX_STRING_SIZE];
	char value_str[MAX_STRING_SIZE];

	char *equal_ptr;
	int equal_pos;

	equal_ptr = index(item, '=');
	if (!equal_ptr) {
		*var = '\0';
		*value = '\0';
		return;
	}

	/* Equal sign found, split string */
	equal_pos = equal_ptr - item;
	strncpy(var_str, item, equal_pos);
	var_str[equal_pos] = '\0';
	strncpy(value_str, equal_ptr + 1, strlen(item) - equal_pos - 1);
	value_str[strlen(item) - equal_pos - 1] = '\0';

	/* Return trimmed strings */
	trim(var, var_str);
	trim(value, value_str);
}


static void config_insert_section(struct config_t *cfg, char *section)
{
	char section_trim[MAX_STRING_SIZE];

	trim(section_trim, section);
	hash_table_insert(cfg->items, section, (void *) 1);
}


static void config_insert_var(struct config_t *cfg, char *section, char *var, char *value)
{
	char item[MAX_STRING_SIZE];
	char value_trim[MAX_STRING_SIZE];
	char *ovalue, *nvalue;

	get_item_from_section_var(section, var, item);

	/* Allocate new value */
	trim(value_trim, value);
	nvalue = strdup(value_trim);

	/* Free previous value if variable existed */
	ovalue = hash_table_get(cfg->items, item);
	if (ovalue) {
		free(ovalue);
		hash_table_set(cfg->items, item, nvalue);
		return;
	}

	/* Insert new value */
	hash_table_insert(cfg->items, item, nvalue);
}




/*
 * Public Functions
 */

/* Creation and destruction */
struct config_t *config_create(char *filename)
{
	struct config_t *cfg;
	
	/* Config object */
	cfg = calloc(1, sizeof(struct config_t));
	if (!cfg) {
		fprintf(stderr, "%s: out of memory\n", __FUNCTION__);
		abort();
	}
	
	/* Main hash table & file name*/
	cfg->file_name = strdup(filename);
	cfg->items = hash_table_create(HASH_TABLE_SIZE, 0);
	cfg->allowed_items = hash_table_create(HASH_TABLE_SIZE, 0);

	/* Return created object */
	if (!cfg->items || !cfg->file_name || !cfg->allowed_items) {
		fprintf(stderr, "%s: out of memory\n", __FUNCTION__);
		abort();
	}
	return cfg;
}


void config_free(struct config_t *cfg)
{
	char *item;
	void *value;
	char section[MAX_STRING_SIZE];
	char var[MAX_STRING_SIZE];

	/* Free variable values */
	for (item = hash_table_find_first(cfg->items, &value);
		item; item = hash_table_find_next(cfg->items, &value))
	{
		get_section_var_from_item(item, section, var);
		if (var[0]) {
			free(value);
			continue;
		}
		assert(value == ITEM_ALLOWED || value == ITEM_MANDATORY);
	}

	/* Free rest */
	free(cfg->file_name);
	hash_table_free(cfg->items);
	hash_table_free(cfg->allowed_items);
	free(cfg);
}


int config_load(struct config_t *cfg)
{
	FILE *f;

	char line[MAX_STRING_SIZE], line_trim[MAX_STRING_SIZE], *line_ptr;

	char section[MAX_STRING_SIZE];
	char var[MAX_STRING_SIZE];
	char value[MAX_STRING_SIZE];
	
	/* Try to open file for reading */
	f = fopen(cfg->file_name, "rt");
	if (!f)
		return 0;
	
	/* Read lines */
	section[0] = '\0';
	while (!feof(f)) {
	
		/* Read a line */
		line_ptr = fgets(line, MAX_STRING_SIZE, f);
		if (!line_ptr)
			break;
		trim(line_trim, line);

		/* Comment */
		if (line_trim[0] == ';')
			continue;
		
		/* New "[ <section> ]" entry */
		if (line_trim[0] == '[' && line_trim[strlen(line_trim) - 1] == ']') {
			
			line_trim[0] = ' ';
			line_trim[strlen(line_trim) - 1] = ' ';
			trim(section, line_trim);
			config_insert_section(cfg, section);
			continue;
		}

		/* If there is no current section, ignore entry */
		if (!section[0])
			continue;
		
		/* New "<var> = <value>" entry. If format is incorrect, ignore line. */
		get_var_value_from_item(line_trim, var, value);
		if (!var[0] || !value[0])
			continue;
		config_insert_var(cfg, section, var, value);
	}
	
	/* close file */
	fclose(f);
	return 1;
}


int config_save(struct config_t *config)
{
	struct linked_list_t *section_list;
	char *section;
	char *item, *value;
	FILE *f;
	
	/* Try to open file for writing */
	f = fopen(config->file_name, "wt");
	if (!f)
		return 0;
	
	/* Create a list with all sections first */
	section_list = linked_list_create();
	for (item = hash_table_find_first(config->items, (void **) &value); item;
		item = hash_table_find_next(config->items, (void **) &value))
	{
		if (value == (char *) 1)
			linked_list_add(section_list, item);
	}

	/* Dump all variables for each section */
	for (linked_list_head(section_list); !linked_list_is_end(section_list); linked_list_next(section_list))
	{
		char section_buf[MAX_STRING_SIZE];
		char var_buf[MAX_STRING_SIZE];

		section = linked_list_get(section_list);
		fprintf(f, "[ %s ]\n", section);

		for (item = hash_table_find_first(config->items, (void **) &value); item;
			item = hash_table_find_next(config->items, (void **) &value))
		{
			get_section_var_from_item(item, section_buf, var_buf);
			if (var_buf[0] && !strcmp(section_buf, section))
				fprintf(f, "%s = %s\n", var_buf, value);
		}

		fprintf(f, "\n");
	}

	/* Free section list */
	linked_list_free(section_list);
	
	/* close file */
	fclose(f);
	return 1;
}


int config_section_exists(struct config_t *cfg, char *section)
{
	char section_trim[MAX_STRING_SIZE];

	trim(section_trim, section);
	return hash_table_get(cfg->items, section_trim) != NULL;
}


int config_var_exists(struct config_t *cfg, char *section, char *var)
{
	char item[MAX_STRING_SIZE];

	get_item_from_section_var(section, var, item);
	return hash_table_get(cfg->items, item) != NULL;
}


int config_section_remove(struct config_t *cfg, char *section)
{
	fprintf(stderr, "%s: not implemented\n", __FUNCTION__);
	return 1;
}


int config_key_remove(struct config_t *cfg, char *section, char *key)
{
	fprintf(stderr, "%s: not implemented\n", __FUNCTION__);
	return 1;
}




/*
 * Enumeration of sections
 */

char *config_section_first(struct config_t *cfg)
{
	char *item;
	char section[MAX_STRING_SIZE];
	char var[MAX_STRING_SIZE];

	item = hash_table_find_first(cfg->items, NULL);
	if (!item)
		return NULL;
	get_section_var_from_item(item, section, var);
	if (!var[0])
		return item;
	return config_section_next(cfg);
}


char *config_section_next(struct config_t *cfg)
{
	char *item;
	char section[MAX_STRING_SIZE];
	char var[MAX_STRING_SIZE];

	do {
		item = hash_table_find_next(cfg->items, NULL);
		if (!item)
			return NULL;
		get_section_var_from_item(item, section, var);
	} while (var[0]);
	return item;
}




/*
 * Writing to configuration file
 */

void config_write_string(struct config_t *cfg, char *section, char *var, char *value)
{
	char item[MAX_STRING_SIZE];

	/* Add section and variable to the set of allowed items, as long as
	 * it is not added already as a mandatory item. */
	get_item_from_section_var(section, var, item);
	if (!hash_table_get(cfg->allowed_items, section))
		hash_table_insert(cfg->allowed_items, section, ITEM_ALLOWED);
	if (!hash_table_get(cfg->allowed_items, item))
		hash_table_insert(cfg->allowed_items, item, ITEM_ALLOWED);
	
	/* Write value */
	config_insert_section(cfg, section);
	config_insert_var(cfg, section, var, value);
}


void config_write_int(struct config_t *cfg, char *section, char *var, int value)
{
	char value_str[MAX_STRING_SIZE];

	sprintf(value_str, "%d", value);
	config_write_string(cfg, section, var, value_str);
}


void config_write_llint(struct config_t *cfg, char *section, char *var, long long value)
{
	char value_str[MAX_STRING_SIZE];

	sprintf(value_str, "%lld", value);
	config_write_string(cfg, section, var, value_str);
}


void config_write_bool(struct config_t *cfg, char *section, char *var, int value)
{
	char value_str[MAX_STRING_SIZE];

	strcpy(value_str, value ? "t" : "f");
	config_write_string(cfg, section, var, value_str);
}


void config_write_double(struct config_t *cfg, char *section, char *var, double value)
{
	char value_str[MAX_STRING_SIZE];

	sprintf(value_str, "%g", value);
	config_write_string(cfg, section, var, value_str);
}


void config_write_ptr(struct config_t *cfg, char *section, char *var, void *value)
{
	char value_str[MAX_STRING_SIZE];

	sprintf(value_str, "%p", value);
	config_write_string(cfg, section, var, value_str);
}




/*
 * Reading from configuration file
 */

char *config_read_string(struct config_t *cfg, char *section, char *var, char *def)
{
	char item[MAX_STRING_SIZE];
	char *value;

	/* Add section and variable to the set of allowed items, as long as
	 * it is not added already as a mandatory item. */
	get_item_from_section_var(section, var, item);
	if (!hash_table_get(cfg->allowed_items, section))
		hash_table_insert(cfg->allowed_items, section, ITEM_ALLOWED);
	if (!hash_table_get(cfg->allowed_items, item))
		hash_table_insert(cfg->allowed_items, item, ITEM_ALLOWED);
	
	/* Read value */
	value = hash_table_get(cfg->items, item);
	return value ? value : def;
}


int config_read_int(struct config_t *cfg, char *section, char *var, int def)
{
	char *result;
	result = config_read_string(cfg, section, var, NULL);
	return result ? atoi(result) : def;
}


long long config_read_llint(struct config_t *cfg, char *section, char *var, long long def)
{
	char *result;
	result = config_read_string(cfg, section, var, NULL);
	return result ? atoll(result) : def;
}


int config_read_bool(struct config_t *cfg, char *section, char *var, int def)
{
	char *result;
	result = config_read_string(cfg, section, var, NULL);
	if (!result)
		return def;
	if (!strcmp(result, "t") || !strcmp(result, "true") ||
		!strcmp(result, "true") || !strcmp(result, "True") ||
		!strcmp(result, "TRUE"))
		return 1;
	return 0;
}


double config_read_double(struct config_t *cfg, char *section, char *var, double def)
{
	char *result;
	double d;
	result = config_read_string(cfg, section, var, NULL);
	if (!result)
		return def;
	sscanf(result, "%lf", &d);
	return d;
}


static void enumerate_map(char **map, int map_count)
{
	int i;
	char *comma = "";

	fprintf(stderr, "Possible allowed values are { ");
	for (i = 0; i < map_count; i++) {
		fprintf(stderr, "%s%s", comma, map[i]);
		comma = ", ";
	}
	fprintf(stderr, " }\n");
}


int config_read_enum(struct config_t *cfg, char *section, char *var, int def, char **map, int map_count)
{
	char *result;
	int i;

	result = config_read_string(cfg, section, var, NULL);
	if (!result)
		return def;
	
	/* Translate */
	for (i = 0; i < map_count; i++)
		if (!strcasecmp(map[i], result))
			return i;
	
	/* No match found with map */
	fprintf(stderr, "%s: section '[ %s ]': variable '%s': invalid value ('%s')\n",
		cfg->file_name, section, var, result);
	enumerate_map(map, map_count);
	exit(1);
}


void *config_read_ptr(struct config_t *cfg, char *section, char *var, void *def)
{
	char *result;
	void *ptr;
	result = config_read_string(cfg, section, var, NULL);
	if (!result)
		return def;
	sscanf(result, "%p", &ptr);
	return ptr;
}




/*
 * Configuration file format
 */

/* Insert a section (and variable) into the hash table of allowed sections (variables).
 * If the item was there, update it with the new allowed/mandatory property.
 * Field 'property' should be ITEM_ALLOWED/ITEM_MANDATORY. */
static void allowed_items_insert(struct config_t *cfg, char *section, char *var, void *property)
{
	char item[MAX_STRING_SIZE];

	get_item_from_section_var(section, var, item);
	if (hash_table_get(cfg->allowed_items, item))
		hash_table_set(cfg->allowed_items, item, property);
	hash_table_insert(cfg->allowed_items, item, property);
}


/* Return true if an item is allowed (or mandatory).
 * Argument 'var' can be NULL or an empty string to refer to a section. */
static int item_is_allowed(struct config_t *cfg, char *section, char *var)
{
	char item[MAX_STRING_SIZE];

	get_item_from_section_var(section, var, item);
	return hash_table_get(cfg->allowed_items, item) != NULL;
}


/* Return true if an item is present in the configuration file.
 * Argument 'var' can be NULL or an empty string to refer to a section. */
static int item_is_present(struct config_t *cfg, char *section, char *var)
{
	char item[MAX_STRING_SIZE];

	get_item_from_section_var(section, var, item);
	return hash_table_get(cfg->items, item) != NULL;
}


void config_section_allow(struct config_t *cfg, char *section)
{
	allowed_items_insert(cfg, section, NULL, ITEM_ALLOWED);
}


void config_section_enforce(struct config_t *cfg, char *section)
{
	allowed_items_insert(cfg, section, NULL, ITEM_MANDATORY);
}


void config_var_allow(struct config_t *cfg, char *section, char *var)
{
	allowed_items_insert(cfg, section, var, ITEM_ALLOWED);
}


void config_var_enforce(struct config_t *cfg, char *section, char *var)
{
	allowed_items_insert(cfg, section, var, ITEM_MANDATORY);
}


void config_check(struct config_t *cfg)
{
	char *item;
	void *property;

	char section[MAX_STRING_SIZE];
	char var[MAX_STRING_SIZE];

	/* Go through mandatory items and check they are present */
	for (item = hash_table_find_first(cfg->allowed_items, &property);
		item; item = hash_table_find_next(cfg->allowed_items, &property))
	{
		
		/* If this is an allowed (not mandatory) item, continue */
		if (property == ITEM_ALLOWED)
			continue;

		/* Item must be in the configuration file */
		get_section_var_from_item(item, section, var);
		if (!item_is_present(cfg, section, NULL)) {
			fprintf(stderr, "%s: section '[ %s ]' not found in configuration file\n",
				cfg->file_name, section);
			exit(1);
		}
		if (!item_is_present(cfg, section, var)) {
			fprintf(stderr, "%s: section '[ %s ]': variable '%s' is missing in the configuration file\n",
				cfg->file_name, section, var);
			exit(1);
		}
	}
	
	/* Go through all present sections/keys and check they are present in the
	 * set of allowed/mandatory items. */
	for (item = hash_table_find_first(cfg->items, NULL);
		item; item = hash_table_find_next(cfg->items, NULL))
	{
		/* Check if it is allowed */
		get_section_var_from_item(item, section, var);
		if (item_is_allowed(cfg, section, var))
			continue;

		/* It is not, error */
		if (!var[0])
			fprintf(stderr, "%s: section '[ %s ]' is not valid in the configuration file\n",
				cfg->file_name, section);
		else
			fprintf(stderr, "%s: section '[ %s ]': variable '%s' is not valid in configuration file\n",
				cfg->file_name, section, var);
		exit(1);
	}
}

