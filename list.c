/* a simple implementation of linked lists
 *
 * Copyright (C) 2001  Grahame Bowland <grahame@angrygoats.net>
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
#include "list.h"

struct list *list_new(void *data)/*{{{*/
{
	struct list *r;

	r = malloc(sizeof(struct list));
	if (!r) {
		fprintf(stderr, "Out of memory. Could not allocate memory in list_new_node.\n");
		exit(1);
	}
	r->data = data;
	r->next = NULL;
	return r;
}

struct list *list_append(struct list *node, void *data)/*{{{*/
{
	struct list *r;
	/* we actually do appends as prepends for efficiency */
	r = list_new(data);
	r->next = node;
	return r;
}

struct list *list_next(struct list *node)/*{{{*/
{
	return node->next;
}

unsigned int list_length(struct list *node)/*{{{*/
{
	unsigned int l = 0;
	while (node) {
		l++;
		node = node->next;
	}
	return l;
}

void list_free(struct list *node)/*{{{*/
{
	struct list *p, *next;
	p = node;
	while (p) {
		next = p->next;
		free(p);
		p = next;
	}
}

