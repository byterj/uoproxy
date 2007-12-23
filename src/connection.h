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

#ifndef __CONNECTION_H
#define __CONNECTION_H

#include "packets.h"
#include "world.h"
#include "cversion.h"

#include <event.h>

#define MAX_CHARACTERS 16
#define MAX_WALK_QUEUE 4

struct instance;
struct addrinfo;

struct stateful_client {
    int reconnecting;
    struct event reconnect_event;

    struct uo_client *client;
    struct event ping_event;

    struct uo_fragment_character_info characters[MAX_CHARACTERS];
    unsigned num_characters;

    uint16_t supported_features_flags;

    unsigned char ping_request, ping_ack;

    struct world world;
};

struct linked_server {
    struct list_head siblings;

    struct connection *connection;

    struct uo_server *server;
    int welcome, attaching;

    struct client_version client_version;
};

struct connection_walk_item {
    struct uo_packet_walk packet;
    uint8_t seq;
};

struct connection_walk_state {
    struct linked_server *server;
    struct connection_walk_item queue[MAX_WALK_QUEUE];
    unsigned queue_size;
    uint8_t seq_next;
};

struct connection {
    /* linked list and parent */
    struct list_head siblings;

    struct instance *instance;

    /* flags */
    int background;

    int in_game;

    /* reconnect */
    int autoreconnect;

    /* client stuff (= connection to server) */

    struct stateful_client client;

    /* state */
    char username[30], password[30];

    unsigned server_index;

    unsigned character_index;

    struct connection_walk_state walk;

    /* client version */

    struct client_version client_version;

    /* sub-objects */

    struct list_head servers;
};

int connection_new(struct instance *instance,
                   int server_socket,
                   struct connection **connectionp);

#ifdef NDEBUG
static inline void
connection_check(const struct connection *c)
{
    (void)c;
}
#else
void connection_check(const struct connection *c);
#endif

void connection_delete(struct connection *c);

void connection_speak_console(struct connection *c, const char *msg);

void connection_broadcast_servers_except(struct connection *c,
                                         const void *data, size_t length,
                                         struct uo_server *except);


/* server list */

void
connection_server_add(struct connection *c, struct linked_server *ls);

void
connection_server_remove(struct connection *c, struct linked_server *ls);

struct linked_server *
connection_server_new(struct connection *c, int fd);

void
connection_server_dispose(struct connection *c, struct linked_server *ls);


/* world */

void
connection_world_clear(struct connection *c);


/* walk */

void connection_walk_server_removed(struct connection_walk_state *state,
                                    struct linked_server *server);

void
connection_walk_request(struct linked_server *server,
                        const struct uo_packet_walk *p);

void connection_walk_cancel(struct connection *c,
                            const struct uo_packet_walk_cancel *p);

void connection_walk_ack(struct connection *c,
                         const struct uo_packet_walk_ack *p);

/* reconnect */

int
connection_client_connect(struct connection *c,
                          const struct addrinfo *server_address,
                          uint32_t seed);

void connection_disconnect(struct connection *c);

void connection_reconnect(struct connection *c);

void
connection_reconnect_delayed(struct connection *c);

/* attach */

struct connection *find_attach_connection(struct connection *c);

void attach_after_play_server(struct connection *c,
                              struct linked_server *ls);

void
attach_after_play_character(struct linked_server *ls);

/* command */

void
connection_handle_command(struct linked_server *server, const char *command);

#endif
