/*
 * uoproxy
 *
 * (c) 2005-2007 Max Kellermann <max@duempel.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; version 2 of the License.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#include "connection.h"
#include "server.h"

#include <assert.h>
#include <stdlib.h>
#include <errno.h>

void
connection_server_add(struct connection *c, struct linked_server *ls)
{
    assert(ls->connection == NULL);

    list_add(&ls->siblings, &c->servers);
    ls->connection = c;
}

void
connection_server_remove(struct connection *c, struct linked_server *ls)
{
    connection_check(c);
    assert(ls != NULL);
    assert(c == ls->connection);

    if (c->current_server == ls)
        c->current_server = NULL;

    ls->connection = NULL;
    list_del(&ls->siblings);
}

struct linked_server *
connection_server_new(struct connection *c, int fd)
{
    struct linked_server *ls;
    int ret;

    ls = calloc(1, sizeof(*ls));
    if (ls == NULL)
        return NULL;

    ret = uo_server_create(fd, &ls->server);
    if (ret != 0) {
        free(ls);
        errno = ret;
        return NULL;
    }

    connection_server_add(c, ls);
    return ls;
}