/* provides a simple client program that reads ACPI status from the /proc 
 * filesystem
 *
 * Copyright (C) 2001  Grahame Bowland <grahame@angrygoats.net>
 * Copyright (C) 2008-2012  Michael Meskes <meskes@debian.org>
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include "acpi.h"

struct device device[4] = {
			{ BATTERY, "battery", "power_supply", "BAT" },
			{ AC_ADAPTER, "ac_adapter", "power_supply", "AC" },
			{ THERMAL_ZONE, "thermal_zone", "thermal", "thermal_zone" },
			{ COOLING_DEV, "fan", "thermal", "cooling_device" }
			  };

static void do_show_batteries(char *acpi_path, int show_empty_slots, int show_details, int proc_interface)
{
	struct list *batteries;

	batteries = find_devices(acpi_path, BATTERY, proc_interface);
	print_battery_information(batteries, show_empty_slots, show_details);
	free_devices(batteries);
}

static void do_show_ac_adapter(char *acpi_path, int show_empty_slots, int proc_interface)
{
	struct list *ac_adapter;

	ac_adapter = find_devices(acpi_path, AC_ADAPTER, proc_interface);
	print_ac_adapter_information(ac_adapter, show_empty_slots);
	free_devices(ac_adapter);
}

static void do_show_thermal(char *acpi_path, int show_empty_slots, int temperature_units, int show_details, int proc_interface) {
	struct list *thermal;

	thermal = find_devices(acpi_path, THERMAL_ZONE, proc_interface);
	print_thermal_information(thermal, show_empty_slots, temperature_units, show_details);
	free_devices(thermal);
}

static void do_show_cooling(char *acpi_path, int show_empty_slots, int proc_interface) {
	struct list *cooling;

	cooling = find_devices(acpi_path, COOLING_DEV, proc_interface);
	print_cooling_information(cooling, show_empty_slots);
	free_devices(cooling);
}

static int version(void)
{
	printf(ACPI_VERSION_STRING "\n"
"\n"
"Copyright (C) 2001 Grahame Bowland.\n"
"              2008-2012 Michael Meskes.\n"
"This is free software; see the source for copying conditions.  There is NO\n"
"warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.\n"
);
	return 1;
}

static int usage(char *argv[])
{
	printf(
"Usage: acpi [OPTION]...\n"
"Shows information from the /proc filesystem, such as battery status or\n"
"thermal information.\n"
"\n"
"  -b, --battery            battery information\n"
"  -i, --details            show additional details if available:\n"
"                             - battery capacity information\n"
"                             - temperature trip points\n"
"  -a, --ac-adapter         ac adapter information\n"
"  -t, --thermal            thermal information\n"
"  -c, --cooling            cooling information\n"
"  -V, --everything         show every device, overrides above options\n"
"  -s, --show-empty         show non-operational devices\n"
"  -f, --fahrenheit         use fahrenheit as the temperature unit\n"
"  -k, --kelvin             use kelvin as the temperature unit\n"
"  -d, --directory <dir>    path to ACPI info (/sys/class resp. /proc/acpi)\n"
"  -p, --proc               use old proc interface instead of new sys interface\n"
"  -h, --help               display this help and exit\n"
"  -v, --version            output version information and exit\n"
"\n"
"By default, acpi displays information on installed system batteries.\n"
"Non-operational devices, for example empty battery slots are hidden.\n"
"The default unit of temperature is degrees celsius.\n"
"\n"
"Report bugs to Michael Meskes <meskes@debian.org>.\n"
);
	return 1;
}

static struct option long_options[] = {
	{ "help", 0, 0, 'h' }, 
	{ "version", 0, 0, 'v' }, 
	{ "verbose", 0, 0, 'V' }, 
	{ "battery", 0, 0, 'b' }, 
	{ "ac-adapter", 0, 0, 'a' }, 
	{ "thermal", 0, 0, 't' }, 
	{ "cooling", 0, 0, 'c' }, 
	{ "show-empty", 0, 0, 's' }, 
	{ "fahrenheit", 0, 0, 'f' }, 
	{ "kelvin", 0, 0, 'k' }, 
	{ "directory", 1, 0, 'd' },
	{ "everything", 0, 0, 'V' }, 
	{ "proc", 0, 0, 'p' }, 
	{ "details", 0, 0, 'i' }, 
	{ 0, 0, 0, 0 }, 
};

int main(int argc, char *argv[])
{
	int show_batteries = FALSE;
	int show_ac_adapter = FALSE;
	int show_thermal = FALSE;
	int show_cooling = FALSE;
	int show_empty_slots = FALSE;
	int show_details = FALSE;
	int proc_interface = FALSE;
	int temperature_units = TEMP_CELSIUS;
	int ch, option_index;
	char *acpi_path = strdup(ACPI_PATH_SYS);

	if (!acpi_path) {
		fprintf(stderr, "Out of memory in main()\n");
		return -1;
	}

	while ((ch = getopt_long(argc, argv, "ipVbtashvfkcd:", long_options, &option_index)) != -1) {
		switch (ch) {
			case 'V':
				show_batteries = show_ac_adapter = show_thermal = show_cooling = show_details = TRUE;
				break;
			case 'b':
				show_batteries = TRUE;
				break;
			case 'a':
				show_ac_adapter = TRUE;
				break;
			case 't':
				show_thermal = TRUE;
				break;
			case 'c':
				show_cooling = TRUE;
				break;
			case 's':
				show_empty_slots = TRUE;
				break;
			case 'i':
				show_details = TRUE;
				break;
			case 'v':
				return version();
				break;
			case 'f':
				temperature_units = TEMP_FAHRENHEIT;
				break;
			case 'k':
				temperature_units = TEMP_KELVIN;
				break;
			case 'p':
				proc_interface = TRUE;
				free(acpi_path);
				acpi_path = strdup(ACPI_PATH_PROC);
				if (!acpi_path) {
					fprintf(stderr, "Out of memory in main()\n");
					return -1;
				}
				break;
			case 'd':
				free(acpi_path);
				acpi_path = strdup(optarg);
				if (!acpi_path) {
					fprintf(stderr, "Out of memory in main()\n");
					return -1;
				}
				break;
			case 'h':
			default:
				return usage(argv);
		}
	}

	/* if nothing was chosen, we show the battery information */
	if (!show_batteries && !show_ac_adapter && !show_thermal && !show_cooling)
		show_batteries = TRUE;

	if (show_batteries) {
		do_show_batteries(acpi_path, show_empty_slots, show_details, proc_interface);
	}
	if (show_ac_adapter) {
		do_show_ac_adapter(acpi_path, show_empty_slots, proc_interface);
	}
	if (show_thermal) {
		do_show_thermal(acpi_path, show_empty_slots, temperature_units, show_details, proc_interface);
	}
	if (show_cooling) {
		do_show_cooling(acpi_path, show_empty_slots, proc_interface);
	}
	return 0;
}

