/*
 * gmcp.c - GMCP (Generic MUD Communication Protocol) implementation
 *
 * Sends structured game data to clients via:
 *   - Telnet: IAC SB GMCP <package> <json> IAC SE
 *   - WebSocket: JSON message with type "gmcp"
 *
 * Copyright (c) 2025 DurisMUD Development Team
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "gmcp.h"
#include "websocket.h"
#include "json_utils.h"
#include "structs.h"
#include "prototypes.h"
#include "utils.h"
#include "comm.h"
#include "db.h"
#include "handler.h"
#include "spells.h"

/* External declarations */
extern struct room_data *world;
extern struct zone_data *zone_table;
extern const char *pc_class_types[];
extern struct descriptor_data *descriptor_list;

/*
 * Send GMCP negotiation request to client
 */
void gmcp_negotiate(struct descriptor_data *d) {
    unsigned char negotiate[3];

    if (!d || d->websocket) return;  /* WebSocket clients don't need telnet negotiation */

    negotiate[0] = TELNET_IAC;
    negotiate[1] = TELNET_WILL;
    negotiate[2] = TELOPT_GMCP;

    write(d->descriptor, negotiate, 3);
}

/*
 * Handle GMCP negotiation response
 */
void gmcp_handle_negotiation(struct descriptor_data *d, int cmd) {
    if (!d) return;

    if (cmd == TELNET_DO) {
        d->gmcp_enabled = 1;
        statuslog(56, "GMCP enabled for %s", d->host);
    } else if (cmd == TELNET_DONT) {
        d->gmcp_enabled = 0;
    }
}

/*
 * Handle incoming GMCP data from telnet client
 * (Client preferences, module subscriptions, etc.)
 */
void gmcp_handle_input(struct descriptor_data *d, const char *data, size_t len) {
    /* Currently just log - future: parse client requests */
    if (d && data && len > 0) {
        /* statuslog(56, "GMCP input from %s: %.*s", d->host, (int)len, data); */
    }
}

/*
 * Core GMCP send function
 * Handles both telnet and WebSocket transport
 */
void gmcp_send(struct descriptor_data *d, const char *package, const char *json) {
    if (!d || !package || !json) return;
    if (!d->gmcp_enabled && !d->websocket) return;

    if (d->websocket) {
        /* WebSocket: Send JSON message */
        websocket_send_json(d, "gmcp", package, json);
    } else {
        /* Telnet: Send IAC SB GMCP <package> <json> IAC SE */
        size_t pkg_len = strlen(package);
        size_t json_len = strlen(json);
        size_t total_len = 3 + pkg_len + 1 + json_len + 2;  /* IAC SB GMCP + package + space + json + IAC SE */
        unsigned char *buf = (unsigned char *)malloc(total_len);

        if (!buf) return;

        buf[0] = TELNET_IAC;
        buf[1] = TELNET_SB;
        buf[2] = TELOPT_GMCP;
        memcpy(buf + 3, package, pkg_len);
        buf[3 + pkg_len] = ' ';
        memcpy(buf + 4 + pkg_len, json, json_len);
        buf[total_len - 2] = TELNET_IAC;
        buf[total_len - 1] = TELNET_SE;

        /* Use binary write to handle MCCP compression properly */
        write_to_descriptor_binary(d, buf, total_len);
        free(buf);
    }
}

/*
 * Send Room.Info when character enters a room
 */
void gmcp_room_info(struct char_data *ch) {
    char *json;
    struct room_data *room;

    if (!ch || !ch->desc) return;
    if (!GMCP_ENABLED(ch)) return;

    room = &world[ch->in_room];
    json = json_build_room_info(room, ch);

    if (json) {
        gmcp_send(ch->desc, GMCP_PKG_ROOM_INFO, json);
        free(json);
    }
}

/*
 * Send Char.Vitals when HP/Mana/Move changes
 */
void gmcp_char_vitals(struct char_data *ch) {
    char *json;

    if (!ch || !ch->desc) return;
    if (!GMCP_ENABLED(ch)) return;

    json = json_build_char_vitals(ch);

    if (json) {
        gmcp_send(ch->desc, GMCP_PKG_CHAR_VITALS, json);
        free(json);
    }
}

/*
 * Send vitals update to all group members
 */
void gmcp_group_vitals(struct char_data *ch) {
    struct group_list *gl;

    if (!ch || !ch->group) return;

    /* Iterate through all group members */
    for (gl = ch->group; gl; gl = gl->next) {
        if (gl->ch && GMCP_ENABLED(gl->ch)) {
            gmcp_char_vitals(gl->ch);
        }
    }
}

/*
 * Send Char.Status on login, level up, etc.
 */
void gmcp_char_status(struct char_data *ch) {
    char *json;

    if (!ch || !ch->desc) return;
    if (!GMCP_ENABLED(ch)) return;

    json = json_build_char_status(ch);

    if (json) {
        gmcp_send(ch->desc, GMCP_PKG_CHAR_STATUS, json);
        free(json);
    }
}

/*
 * Send Char.Affects when buffs/debuffs change
 */
void gmcp_char_affects(struct char_data *ch) {
    char *json;

    if (!ch || !ch->desc) return;
    if (!GMCP_ENABLED(ch)) return;

    json = json_build_char_affects(ch);

    if (json) {
        gmcp_send(ch->desc, GMCP_PKG_CHAR_AFFECTS, json);
        free(json);
    }
}

/*
 * Send Combat.Update for a combat round
 */
void gmcp_combat_update(struct char_data *ch, struct char_data *victim,
                        int damage, const char *damage_type, int critical) {
    cJSON *root, *target, *round;
    char *json;
    int health_pct;

    if (!ch || !ch->desc || !victim) return;
    if (!GMCP_ENABLED(ch)) return;

    root = cJSON_CreateObject();

    /* Target info */
    target = cJSON_CreateObject();
    cJSON_AddStringToObject(target, "name", GET_NAME(victim));

    /* Calculate health percentage */
    if (GET_MAX_HIT(victim) > 0) {
        health_pct = (GET_HIT(victim) * 100) / GET_MAX_HIT(victim);
        if (health_pct < 0) health_pct = 0;
    } else {
        health_pct = 0;
    }

    /* Health description */
    const char *health_desc;
    if (health_pct >= 100) health_desc = "excellent";
    else if (health_pct >= 90) health_desc = "few scratches";
    else if (health_pct >= 75) health_desc = "small wounds";
    else if (health_pct >= 50) health_desc = "quite a few wounds";
    else if (health_pct >= 30) health_desc = "big nasty wounds";
    else if (health_pct >= 15) health_desc = "pretty hurt";
    else if (health_pct >= 1) health_desc = "awful";
    else health_desc = "bleeding to death";

    cJSON_AddStringToObject(target, "health", health_desc);
    cJSON_AddNumberToObject(target, "healthPercent", health_pct);

    /* Target position - matches POS_ constants in defines.h */
    const char *positions[] = {"on their ass", "sitting", "kneeling", "standing"};
    int pos = GET_POS(victim);
    const char *pos_desc = (pos >= 0 && pos <= 3) ? positions[pos] : "standing";
    cJSON_AddStringToObject(target, "position", pos_desc);
    cJSON_AddItemToObject(root, "target", target);

    /* Round info */
    if (damage > 0 || damage_type) {
        round = cJSON_CreateObject();
        cJSON_AddStringToObject(round, "attacker", GET_NAME(ch));
        cJSON_AddNumberToObject(round, "damage", damage);
        cJSON_AddStringToObject(round, "damageType", damage_type ? damage_type : "physical");
        cJSON_AddBoolToObject(round, "critical", critical);
        cJSON_AddItemToObject(root, "round", round);
    }

    json = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);

    if (json) {
        gmcp_send(ch->desc, GMCP_PKG_COMBAT_UPDATE, json);
        free(json);
    }
}

/*
 * Send target health update (for health bar)
 */
void gmcp_combat_target(struct char_data *ch, struct char_data *victim) {
    gmcp_combat_update(ch, victim, 0, NULL, 0);
}

/*
 * Send combat ended notification
 */
void gmcp_combat_end(struct char_data *ch) {
    if (!ch || !ch->desc) return;
    if (!GMCP_ENABLED(ch)) return;

    gmcp_send(ch->desc, GMCP_PKG_COMBAT_UPDATE, "{\"target\":null}");
}

/*
 * Send channel message via GMCP
 */
void gmcp_comm_channel(struct char_data *ch, const char *channel,
                       const char *sender, const char *text) {
    char *json;

    if (!ch || !ch->desc) return;
    if (!GMCP_ENABLED(ch)) return;

    json = json_build_comm_channel(channel, sender, text);

    if (json) {
        gmcp_send(ch->desc, GMCP_PKG_COMM_CHANNEL, json);
        free(json);
    }
}

/*
 * Broadcast channel message to all GMCP-enabled players
 */
void gmcp_broadcast_channel(const char *channel, const char *sender,
                            const char *text, struct char_data *exclude) {
    struct descriptor_data *d;
    char *json;

    json = json_build_comm_channel(channel, sender, text);
    if (!json) return;

    for (d = descriptor_list; d; d = d->next) {
        if (d->character && d->character != exclude &&
            STATE(d) == CON_PLAYING && d->gmcp_enabled) {
            gmcp_send(d, GMCP_PKG_COMM_CHANNEL, json);
        }
    }

    free(json);
}

/*
 * Send Quest.Status for bartender quests
 */
void gmcp_quest_status(struct char_data *ch) {
    char *json;

    if (!ch || !ch->desc) return;
    if (!GMCP_ENABLED(ch)) return;

    json = json_build_quest_status(ch);

    if (json) {
        gmcp_send(ch->desc, GMCP_PKG_QUEST_STATUS, json);
        free(json);
    }
}
