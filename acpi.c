/* provides a simple client program that reads ACPI status from the /proc filesystem
 *
 * Copyright (C) 2001  Grahame Bowland <grahame@angrygoats.net>
 *	    (C) 2008-2012  Michael Meskes  <meskes@debian.org>
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

#include <unistd.h>
#include <sys/types.h>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>
#include <errno.h>

#include "list.h"
#include "acpi.h"

#define DEVICE_LEN	20
#define TRIP_POINTS	5
#define BATTERY_DESC	"Battery"
#define AC_ADAPTER_DESC "Adapter"
#define THERMAL_DESC	"Thermal"
#define COOLING_DESC	"Cooling"

#define MIN_PRESENT_RATE 0.01
#define MIN_CAPACITY	 0.01
#define MIN_TEMP	 0.01

static int ignore_directory_entry(struct dirent *de)
{
    return !strcmp(de->d_name, ".") || !strcmp(de->d_name, "..");
}

static struct field *parse_field(char *buf, char *given_attr)
{
    struct field *rval;
    char *p;
    char *attr;
    char *value;
    int has_attr = 0;

    attr = calloc(BUF_SIZE, sizeof(char));
    value = calloc(BUF_SIZE, sizeof(char));
    rval = malloc(sizeof(struct field));
    if (!rval || !attr || !value) {
	fprintf(stderr, "Out of memory. Could not allocate memory in parse_field.\n");
	exit(1);
    }

    p = buf;
    if (!given_attr) {
	while (*(p++)) {
	    if (*p == ':') {
		strncpy(attr, buf, p - buf);
		has_attr = 1;
		break;
	    }
	}
	if (!has_attr) {
	    free(attr);
	    free(value);
	    free(rval);
	    return NULL;
	}
	if (*p == ' ')
	    p++;
	while (*(p++)) {
	    if (*p != ' ')
		break;
	}
    } else {
	strncpy(attr, given_attr, BUF_SIZE);
    }
    strncpy(value, p, BUF_SIZE);
    if (attr[strlen(attr) - 1] == '\n')
	attr[strlen(attr) - 1] = '\0';
    if (value[strlen(value) - 1] == '\n')
	value[strlen(value) - 1] = '\0';
    rval->attr = attr;
    rval->value = value;
    return rval;
}

static struct list *parse_info_file(struct list *l, char *filename, char *given_attr)
{
    FILE *fd;
    char buf[BUF_SIZE];

    fd = fopen(filename, "r");
    if (!fd)
	return l;

    while (fgets(buf, BUF_SIZE, fd) != NULL) {
	struct field *f;
	f = parse_field(buf, given_attr);
	if (!f) 
	    continue;
	l = list_append(l, f);
    }
    fclose(fd);
    return l;
}

struct file_list {
    char *file;
    char *attr;
};

static struct file_list sys_list[] = {
    {"current_now", "current_now"},
    {"power_now", "power_now"},
    {"charge_now", "charge_now"},
    {"energy_now", "energy_now"},
    {"voltage_now", "voltage_now"},
    {"voltage_min_design", "voltage_min_design"},
    {"charge_full", "charge_full"},
    {"energy_full", "energy_full"},
    {"charge_full_design", "charge_full_design"},
    {"energy_full_design", "energy_full_design"},
    {"online", "online"},
    {"status", "charging state"},
    {"type", "type"},
    {"temp", "sys_temp"},
    {"trip_point_0_type", "trip_point_0_type"},
    {"trip_point_0_temp", "trip_point_0_temp"},
    {"trip_point_1_type", "trip_point_1_type"},
    {"trip_point_1_temp", "trip_point_1_temp"},
    {"trip_point_2_type", "trip_point_2_type"},
    {"trip_point_2_temp", "trip_point_2_temp"},
    {"trip_point_3_type", "trip_point_3_type"},
    {"trip_point_3_temp", "trip_point_3_temp"},
    {"trip_point_4_type", "trip_point_4_type"},
    {"trip_point_4_temp", "trip_point_4_temp"},
    {"cur_state", "cur_state"},
    {"max_state", "max_state"}
};

static struct file_list proc_list[] = {
    {"state", NULL},
    {"status", NULL},
    {"info", NULL},
    {"temperature", NULL},
    {"cooling_mode", NULL},
};

static struct list *get_info(char *device_name, int proc_interface)
{
    struct list *rval = NULL;
    struct file_list *list = proc_interface ? proc_list : sys_list;
    int i, n = (proc_interface ? sizeof(proc_list) : sizeof(sys_list)) / sizeof(struct file_list);
    char *filename = malloc(strlen(device_name) + strlen("/energy_full_design "));

    if (filename == NULL) {
	fprintf(stderr, "Out of memory. Could not allocate memory in get_info.\n");
	return NULL;
    }

    for (i = 0; i < n; i++) {
	sprintf(filename, "%s/%s", device_name, list[i].file);
	rval = parse_info_file(rval, filename, list[i].attr);
    }

    return rval;
}

void free_devices(struct list *devices)
{
    struct list *p, *r, *s;
    struct field *f;

    p = devices;
    while (p) {
	r = s = p->data;
	while (r) {
	    f = r->data;
	    free(f->attr);
	    free(f->value);
	    free(f);
	    r = r->next;
	}
	list_free(s);
	p = p->next;
    }
    list_free(devices);
}

struct list *find_devices(char *acpi_path, int device_nr,
			  int proc_interface)
{
    DIR *d;
    struct dirent *de;
    struct list *device_info;
    struct list *rval = NULL;
    char *device_type = proc_interface ? device[device_nr].proc : device[device_nr].sys;
    int found_data = FALSE;

    if (chdir(acpi_path) < 0) {
	fprintf(stderr, "No ACPI support in kernel, or incorrect acpi_path (\"%s\").\n", acpi_path);
	exit(1);
    }

    if (chdir(device_type) == 0) {
	d = opendir(".");
	if (!d) 
	return NULL;

	while ((de = readdir(d))) {
	    if (ignore_directory_entry(de))
		continue;

	    found_data = TRUE;
	    device_info = get_info(de->d_name, proc_interface);

	    if (device_info)
		rval = list_append(rval, device_info);
	}
	closedir(d);
    }

    if (!found_data) {
	fprintf(stderr, "No support for device type: %s\n", device_type);
	return NULL;
    }

    return rval;
}

static int get_unit_value(char *value)
{
    int n = -1;
    sscanf(value, "%d", &n);
    return n;
}

void print_battery_information(struct list *batteries, int show_empty_slots, int show_capacity)
{
    struct list *battery = batteries;
    struct list *fields;
    struct field *value;
    int battery_num = 1;

    while (battery) {
	int remaining_capacity = -1;
	int remaining_energy = -1;
	int present_rate = -1;
	int voltage = -1;
	int design_capacity = -1;
	int design_capacity_unit = -1;
	int last_capacity = -1;
	int last_capacity_unit = -1;
	int hours, minutes, seconds;
	int percentage;
	char *state = NULL, *poststr;
	int type_battery = TRUE;
	char capacity_unit[4] = "mAh";

	fields = battery->data;
	while (fields) {
	    value = fields->data;
	    if (!strcasecmp(value->attr, "remaining capacity")) {
		remaining_capacity = get_unit_value(value->value);
		if (!state)
		    state = strdup("available");
	    } else if (!strcmp(value->attr, "charge_now")) {
		remaining_capacity = get_unit_value(value->value) / 1000;
		if (!state)
		    state = strdup("available");
	    } else if (!strcmp(value->attr, "energy_now")) {
		remaining_energy = get_unit_value(value->value) / 1000;
		if (!state)
		    state = strdup("available");
	    } else if (!strcasecmp(value->attr, "present rate")) {
		present_rate = get_unit_value(value->value);
	    } else if (!strcmp(value->attr, "current_now")) {
		present_rate = get_unit_value(value->value) / 1000;
	    } else if (!strcmp(value->attr, "power_now")) {
		present_rate = get_unit_value(value->value) / 1000;
	    } else if (!strcasecmp(value->attr, "last full capacity")) {
		last_capacity = get_unit_value(value->value);
		if (!state)
		    state = strdup("available");
	    } else if (!strcmp(value->attr, "charge_full")) {
		last_capacity = get_unit_value(value->value) / 1000;
		if (!state)
		    state = strdup("available");
	    } else if (!strcmp(value->attr, "energy_full")) {
		last_capacity_unit = get_unit_value(value->value) / 1000;
		if (!state)
		    state = strdup("available");
	    } else if (!strcmp(value->attr, "charge_full_design")) {
		design_capacity = get_unit_value(value->value) / 1000;
	    } else if (!strcmp(value->attr, "energy_full_design")) {
		design_capacity_unit =
		    get_unit_value(value->value) / 1000;
	    } else if (!strcmp(value->attr, "type")) {
		type_battery = (strcasecmp(value->value, "battery") == 0);
	    } else if (!strcmp(value->attr, "charging state") || !strcmp(value->attr, "State")) {
		state = value->value;
	    } else if (!strcmp(value->attr, "voltage_now")) {
		voltage = get_unit_value(value->value) / 1000;
		if (!voltage) /* zero voltage makes all calculations mood */
			voltage = -1;
	    }
	    fields = list_next(fields);
	}
	if (type_battery) {	/* or else this is the ac_adapter */
	    if (!state) {
		if (show_empty_slots) 
		    printf("%s %d: slot empty\n", BATTERY_DESC, battery_num - 1);
	    } else {
		/* convert energy values (in mWh) to charge values (in mAh) if needed and possible */
		if (last_capacity_unit != -1 && last_capacity == -1) {
		    if (voltage != -1) {
			last_capacity = last_capacity_unit * 1000 / voltage;
		    } else {
			last_capacity = last_capacity_unit;
			strcpy(capacity_unit, "mWh");
		    }
		}
		if (design_capacity_unit != -1 && design_capacity == -1) {
		    if (voltage != -1) {
			design_capacity = design_capacity_unit * 1000 / voltage;
		    } else {
			design_capacity = design_capacity_unit;
			strcpy(capacity_unit, "mWh");
		    }
		}
		if (remaining_energy != -1 && remaining_capacity == -1) {
		    if (voltage != -1) {
			remaining_capacity = remaining_energy * 1000 / voltage;
			present_rate = present_rate * 1000 / voltage;
		    } else {
			remaining_capacity = remaining_energy;
		    }
		}
		if (last_capacity < MIN_CAPACITY)
		    percentage = 0;
		else
		    percentage = remaining_capacity * 100 / last_capacity;
		
		if (percentage > 100)
		    percentage = 100;

		printf("%s %d: %s, %d%%", BATTERY_DESC, battery_num - 1, state, percentage);

		if (present_rate == -1) {
		    poststr = "rate information unavailable";
		    seconds = -1;
		} else if (!strcasecmp(state, "charging")) {
		    if (present_rate > MIN_PRESENT_RATE) {
			seconds = 3600 * (last_capacity - remaining_capacity) / present_rate;
			poststr = " until charged";
		    } else {
			poststr = "charging at zero rate - will never fully charge.";
			seconds = -1;
		    }
		} else if (!strcasecmp(state, "discharging")) {
		    if (present_rate > MIN_PRESENT_RATE) {
			seconds = 3600 * remaining_capacity / present_rate;
			poststr = " remaining";
		    } else {
			poststr = "discharging at zero rate - will never fully discharge.";
			seconds = -1;
		    }
		} else {
		    poststr = NULL;
		    seconds = -1;
		}

		if (seconds > 0) {
		    hours = seconds / 3600;
		    seconds -= 3600 * hours;
		    minutes = seconds / 60;
		    seconds -= 60 * minutes;
		    printf(", %02d:%02d:%02d%s", hours, minutes, seconds, poststr);
		} else if (poststr != NULL) {
		    printf(", %s", poststr);
		}

		printf("\n");

		if (show_capacity && design_capacity > 0) {
		    if (last_capacity <= 100) {
			/* some broken systems just give a percentage here */
			percentage = last_capacity;
			last_capacity = percentage * design_capacity / 100;
		    } else {
			percentage = last_capacity * 100 / design_capacity;
		    }
		    if (percentage > 100)
			percentage = 100;

		    printf ("%s %d: design capacity %d %s, last full capacity %d %s = %d%%\n",
			 BATTERY_DESC, battery_num - 1, design_capacity, capacity_unit, last_capacity, capacity_unit, percentage);
		}
	    }
	    battery_num++;
	}
	battery = list_next(battery);
    }
}

void print_ac_adapter_information(struct list *ac_adapters, int show_empty_slots)
{
    struct list *adapter = ac_adapters;
    struct list *fields;
    struct field *value;
    int adapter_num = 1;

    while (adapter) {
	char *state = NULL;
	int type_ac = TRUE;

	 fields = adapter->data;
	while (fields) {
	    value = fields->data;
	    if (!strcmp(value->attr, "state") || !strcmp(value->attr, "status"))
		state = value->value;
	    else if (!strcmp(value->attr, "online"))
		state = get_unit_value(value->value) ? "on-line" : "off-line";
	    else if (!strcmp(value->attr, "type"))
		type_ac = (strcasecmp(value->value, "mains") == 0);

	    fields = list_next(fields);
	}
	if (type_ac) {		/* or else this is a battery */
	    if (!state) {
		if (show_empty_slots) 
		    printf("%s %d: slot empty\n", AC_ADAPTER_DESC, adapter_num - 1);
	    } else  {
		printf("%s %d: %s\n", AC_ADAPTER_DESC, adapter_num - 1, state);
	    }

	    adapter_num++;
	}
	adapter = list_next(adapter);
    }
}

static double get_real_temp(float temperature, char **scale, int temp_units)
{
	double real_temp = (double) temperature;

	switch (temp_units) {
	case TEMP_CELSIUS:
	    *scale = "degrees C";
	    break;
	case TEMP_FAHRENHEIT:
	    real_temp = (real_temp * 1.8) + 32;
	    *scale = "degrees F";
	    break;
	case TEMP_KELVIN:
	default:
	    real_temp += ABSOLUTE_ZERO;
	    *scale = "kelvin";
	    break;
	}
	return (real_temp);
}

void print_thermal_information(struct list *thermal, int show_empty_slots, int temp_units, int show_trip_points)
{
    struct list *sensor = thermal;
    struct list *fields;
    struct field *value;
    int sensor_num = 1;
    int type_zone = TRUE;

    while (sensor) {
	float temperature = -1;
	struct {
			float trip_temp;
			char *trip_type;
	} trip[TRIP_POINTS];
	char str[]="trip_point_123_type";
	char *state = NULL, *scale;
	double real_temp;
	int i, trip_points = -1;

	memset(trip, 0, sizeof trip);
	fields = sensor->data;
	while (fields) {
	    value = fields->data;
	    if (!strcmp(value->attr, "state")) {
		state = value->value;
	    } else if (!strcmp(value->attr, "type")) {
		type_zone = (strstr(value->value, "thermal zone") != NULL || strstr(value->value, "acpitz") != NULL);
		if (!state)
		    state = strdup("ok");
	    } else if (!strcmp(value->attr, "temperature")) {
		temperature = get_unit_value(value->value);
		if (strstr(value->value, "dK"))
		    temperature = (temperature / 10) - ABSOLUTE_ZERO;
		if (!state)
		    state = strdup("ok");
	    } else if (!strcmp(value->attr, "sys_temp")) {
		temperature = get_unit_value(value->value) / 1000.0;
		if (!state)
		    state = strdup("ok");
	    } else {
		for (i = 0; i < TRIP_POINTS; i++)
		{
			sprintf(str, "trip_point_%d_temp", i);
			if (!strcmp(value->attr, str)) {
				trip[i].trip_temp = get_unit_value(value->value) / 1000.0;
			}
			sprintf(str, "trip_point_%d_type", i);
			if (!strcmp(value->attr, str)) {
				trip[i].trip_type = value->value;
				if (i > trip_points)
					trip_points = i;
			}
		}
	    }
	    fields = list_next(fields);
	}
	if (type_zone) {	/* or else this is a cooling device */
	    for (i = 0; i <= trip_points; i++)
	    {
		if (temperature >= trip[i].trip_temp && trip[i].trip_temp >= MIN_TEMP) {
			state = trip[i].trip_type;
			break;
		}
	    }
	    if (!state) {
		if (show_empty_slots) 
		    printf("%s %d: slot empty\n", THERMAL_DESC, sensor_num - 1);
	    } else {
		real_temp = get_real_temp(temperature, &scale, temp_units);
		printf("%s %d: %s, %.1f %s\n", THERMAL_DESC, sensor_num - 1, state, real_temp, scale);
		if (show_trip_points) {
		    for (i = 0; i <= trip_points; i++)
		    {
			if (trip[i].trip_temp >= MIN_TEMP) {
				real_temp = get_real_temp(trip[i].trip_temp, &scale, temp_units);
				printf("%s %d: trip point %d switches to mode %s at temperature %.1f %s\n",
				THERMAL_DESC, sensor_num - 1, i, trip[i].trip_type, real_temp, scale);
			}
		    }
		}
	    }
	    sensor_num++;
	}
	sensor = list_next(sensor);
    }
}

void print_cooling_information(struct list *cooling, int show_empty_slots)
{
    struct list *sensor = cooling;
    struct list *fields;
    struct field *value;
    int sensor_num = 1;

    while (sensor) {
	char *state = NULL, *type = NULL;
	int cur_state = -1, max_state = -1;
	int type_cooling = TRUE;

	fields = sensor->data;
	while (fields) {
	    value = fields->data;
	    if (!strcmp(value->attr, "status")) {
		state = value->value;
	    } else if (!strcmp(value->attr, "type")) {
		type = value->value;
		type_cooling = (strstr(type, "thermal zone") == NULL && strstr(type, "acpitz") == NULL);
	    } else if (!strcmp(value->attr, "cur_state")) {
		cur_state = get_unit_value(value->value);
	    } else if (!strcmp(value->attr, "max_state")) {
		max_state = get_unit_value(value->value);
	    }
	    fields = list_next(fields);
	}
	if (type_cooling) {	/* or else this is a thermal zone */
	    if (!state && !type) {
		if (show_empty_slots)
		    printf("%s %d: slot empty\n", COOLING_DESC, sensor_num - 1);
	    } else if (state) {
		printf("%s %d: %s\n", COOLING_DESC, sensor_num - 1, state);
	    } else if (cur_state < 0 || max_state < 0) {
		printf("%s %d: %s no state information available\n", COOLING_DESC, sensor_num - 1, type);
	    } else {
		printf("%s %d: %s %d of %d\n", COOLING_DESC, sensor_num - 1, type, cur_state, max_state);
	    }

	    sensor_num++;
	}

	sensor = list_next(sensor);
    }
}
