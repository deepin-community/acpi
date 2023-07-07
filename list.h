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

#ifndef _LIST_H
#define _LIST_H

struct list {
    void *data;
    struct list *next;
};

struct field {
    char *attr;
    char *value;
};

/* create a new list
 * 
 * Pre: 
 * Post: returns a pointer to a valid linked-list
 */
struct list *list_new(void *data);

/* append to a list
 * 
 * Pre: none. if node == NULL this is equivalent to list_new
 * Post: returns a pointer to a valid linked-list
 */
struct list *list_append(struct list *node, void *data);

/* go to the next node in a list
 * 
 * Pre: node != NULL
 * Post: returns a pointer to a valid linked-list
 */
struct list *list_next(struct list *node);

/* get the length of a list
 * 
 * Pre: node != NULL
 * Post: returns a pointer to a valid linked-list
 */
unsigned int list_length(struct list *node);

/* free the memory used by a list
 * 
 * Pre: node != NULL
 * Post: returns a pointer to a valid linked-list
 */
void list_free(struct list *node);

#endif

