/* provides a simple client program that reads ACPI status from the /proc 
 * filesystem
 *
 * Copyright (C) 2001  Grahame Bowland <grahame@angrygoats.net>
 *                     Michael Meskes <meskes@debian.org>
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

#ifndef _ACPI_H
#define _APCI_H

#include "config.h"

/* remember to update this when making new releases */
#define ACPI_VERSION_STRING "acpi " VERSION

#define ACPI_PATH_PROC   "/proc/acpi"
#define ACPI_PATH_SYS   "/sys/class"
#define BUF_SIZE    1024

#define TEMP_KELVIN     0
#define TEMP_CELSIUS    1
#define TEMP_FAHRENHEIT 2
#define ABSOLUTE_ZERO   273.1

#ifndef FALSE
#define FALSE           0
#endif
#ifndef TRUE
#define TRUE            !(FALSE)
#endif

#define BATTERY 0
#define AC_ADAPTER 1
#define THERMAL_ZONE 2
#define COOLING_DEV 3

extern struct device
{
	int type;
	char *proc;
	char *sys;
	char *sys_dev;
} device[4];

struct list *find_devices(char *acpi_path, int device_nr, int proc_interface);

void free_devices(struct list *devices);

void print_battery_information(struct list *batteries, int show_empty_slots, int show_capacity);

void print_ac_adapter_information(struct list *batteries, int show_empty_slots);

void print_thermal_information(struct list *batteries, int show_empty_slots, int temp_units, int show_trip_points);

void print_cooling_information(struct list *batteries, int show_empty_slots);

#endif

