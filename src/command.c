/*
 * uoproxy
 *
 * (c) 2005 Max Kellermann <max@duempel.org>
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

#include <string.h>
#include <stdio.h>

#include "connection.h"
#include "server.h"
#include "client.h"

static void change_character(struct connection *c,
                             struct linked_server *server,
                             unsigned idx) {
    if (idx >= MAX_CHARACTERS || c->characters[idx].name[0] == 0) {
        uo_server_speak_console(server->server,
                                "uoproxy: no character in slot");
        return;
    }

    c->character_index = idx;
    uo_server_speak_console(server->server,
                            "uoproxy: changing character");

    connection_reconnect(c);
}

void connection_handle_command(struct connection *c,
                               struct linked_server *server,
                               const char *command) {
    if (!c->in_game || server == NULL || server->server == NULL)
        return;

    if (*command == 0) {
        uo_server_speak_console(server->server,
                                "uoproxy commands: % %reconnect %char %drop");
    } else if (strcmp(command, "reconnect") == 0) {
        if (c->client == NULL) {
            uo_server_speak_console(server->server,
                                    "uoproxy: not connected");
        } else {
            uo_server_speak_console(server->server,
                                    "uoproxy: reconnecting");
            connection_reconnect(c);
        }
    } else if (strcmp(command, "char") == 0) {
        char msg[1024] = "uoproxy:";
        unsigned i;

        if (c->num_characters == 0) {
            uo_server_speak_console(server->server,
                                    "uoproxy: no characters in list");
            return;
        }

        for (i = 0; i < MAX_CHARACTERS; i++) {
            if (c->characters[i].name[0]) {
                sprintf(msg + strlen(msg), " %u=", i);
                strncat(msg, c->characters[i].name,
                        sizeof(c->characters[i].name));
            }
        }

        uo_server_speak_console(server->server, msg);
    } else if (strncmp(command, "char ", 5) == 0) {
        if (command[5] >= '0' && command[5] <= '9' && command[6] == 0) {
            change_character(c, server, command[5] - '0');
        } else {
            uo_server_speak_console(server->server,
                                    "uoproxy: invalid %char syntax");
        }
    } else if (strcmp(command, "drop") == 0) {
        if (c->client == NULL) {
            uo_server_speak_console(server->server,
                                    "uoproxy: not connected");
        } else {
            struct uo_packet_drop p = {
                .cmd = PCK_Drop,
                .serial = 0,
                .x = c->packet_start.x,
                .y = c->packet_start.y,
                .z = ntohs(c->packet_start.x),
                .dest_serial = 0,
            };

            uo_client_send(c->client, &p, sizeof(p));
        }
    } else {
        uo_server_speak_console(server->server,
                                "unknown uoproxy command, type '%' for help");
    }
}
