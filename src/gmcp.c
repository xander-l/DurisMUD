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
#include "map.h"
#include <cjson/cJSON.h>
#include <math.h>

/* External declarations */
extern struct room_data *world;
extern struct zone_data *zone_table;
extern const char *pc_class_types[];
extern struct descriptor_data *descriptor_list;
extern const struct class_names class_names_table[];
extern const struct race_names race_names_table[];

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

    /* Check descriptor is still valid (not a dangling pointer to closed socket) */
    if (!is_desc_valid(d)) return;

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
 * Send Room.Map for wilderness zones (surface, underdark, alatorin, newbie maps)
 * Sends ASCII art map with ANSI colors
 *
 * This function is called from map.c's display_map_room() which passes the
 * already-generated map buffer to ensure consistency with terminal output.
 */

void gmcp_room_map(struct char_data *ch) {
    /* This stub is kept for backwards compatibility.
     * The actual GMCP Room.Map is now sent from display_map_room() in map.c
     * using gmcp_send_room_map() to ensure the map matches the terminal output.
     */
    (void)ch;  /* Suppress unused parameter warning */
}

/*
 * Send pre-generated map buffer via GMCP
 * Called from display_map_room() in map.c
 */
void gmcp_send_room_map(struct char_data *ch, const char *map_buf) {
    char *json_str;
    cJSON *json;

    if (!ch || !ch->desc) return;
    if (!GMCP_ENABLED(ch)) return;
    if (!map_buf || !*map_buf) return;

    /* Build JSON */
    json = cJSON_CreateObject();
    if (!json) return;

    cJSON_AddStringToObject(json, "map", map_buf);

    json_str = cJSON_PrintUnformatted(json);
    cJSON_Delete(json);

    if (json_str) {
        gmcp_send(ch->desc, GMCP_PKG_ROOM_MAP, json_str);
        free(json_str);
    }
}

/*
 * Send pre-generated quest map buffer via GMCP
 * Called from gmcp_quest_map() after generating map at quest location
 */
void gmcp_send_quest_map(struct char_data *ch, const char *map_buf) {
    char *json_str;
    cJSON *json;

    if (!ch || !ch->desc) return;
    if (!GMCP_ENABLED(ch)) return;
    if (!map_buf || !*map_buf) return;
    if (!ch->only.pc) return;

    /* Build JSON */
    json = cJSON_CreateObject();
    if (!json) return;

    cJSON_AddStringToObject(json, "map", map_buf);
    cJSON_AddNumberToObject(json, "zoneNumber", ch->only.pc->quest_zone_number);

    json_str = cJSON_PrintUnformatted(json);
    cJSON_Delete(json);

    if (json_str) {
        gmcp_send(ch->desc, GMCP_PKG_QUEST_MAP, json_str);
        free(json_str);
    }
}

/*
 * Room Update Throttling
 * Track "dirty" rooms and flush updates periodically to avoid flooding
 * during high-activity scenarios (e.g., 30-player battles)
 */
#define MAX_DIRTY_ROOMS 100
static int dirty_rooms[MAX_DIRTY_ROOMS];
static int dirty_room_count = 0;

void gmcp_mark_room_dirty(int room_number) {
    int i;

    /* Validate room number */
    if (room_number < 0) return;

    /* Check if already marked */
    for (i = 0; i < dirty_room_count; i++) {
        if (dirty_rooms[i] == room_number) return;
    }

    /* Add to dirty list if space available */
    if (dirty_room_count < MAX_DIRTY_ROOMS) {
        dirty_rooms[dirty_room_count++] = room_number;
    }
}

void gmcp_flush_dirty_rooms(void) {
    int i;
    struct char_data *tch;
    struct room_data *room;

    for (i = 0; i < dirty_room_count; i++) {
        room = &world[dirty_rooms[i]];
        if (!room) continue;

        /* Send Room.Info to all players in this room */
        for (tch = room->people; tch; tch = tch->next_in_room) {
            if (!IS_NPC(tch) && tch->desc && GMCP_ENABLED(tch)) {
                gmcp_room_info(tch);
            }
        }
    }

    dirty_room_count = 0;
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
 * Send Group.Status for group panel
 * Sends all group members with HP/Move/position for the web client's group tab
 */
void gmcp_send_group_status(struct char_data *ch) {
    cJSON *root, *members_arr, *member_obj;
    struct group_list *gl;
    char *json_str;
    int count = 0;
    int max_size = 20;  /* Default max group size */
    int is_first = 1;

    if (!ch || !ch->desc) return;
    if (!GMCP_ENABLED(ch)) return;

    /* If not in a group, send null/empty */
    if (!ch->group) {
        gmcp_send(ch->desc, GMCP_PKG_GROUP_STATUS, "{\"members\":[],\"size\":0,\"maxSize\":20}");
        return;
    }

    root = cJSON_CreateObject();
    members_arr = cJSON_CreateArray();

    /* Iterate through group members */
    for (gl = ch->group; gl; gl = gl->next) {
        struct char_data *member = gl->ch;
        if (!member) continue;

        member_obj = cJSON_CreateObject();

        /* Name */
        cJSON_AddStringToObject(member_obj, "name", GET_NAME(member));

        /* Level - for NPCs use mob level, for players use GET_LEVEL */
        cJSON_AddNumberToObject(member_obj, "level", GET_LEVEL(member));

        /* Class - NULL for NPCs */
        if (IS_NPC(member)) {
            cJSON_AddNullToObject(member_obj, "class");
        } else {
            cJSON_AddStringToObject(member_obj, "class",
                class_names_table[flag2idx(member->player.m_class)].normal);
        }

        /* Race - NULL for NPCs */
        if (IS_NPC(member)) {
            cJSON_AddNullToObject(member_obj, "race");
        } else {
            cJSON_AddStringToObject(member_obj, "race",
                race_names_table[(int)GET_RACE(member)].normal);
        }

        /* HP and Move */
        cJSON_AddNumberToObject(member_obj, "hp", GET_HIT(member));
        cJSON_AddNumberToObject(member_obj, "maxHp", GET_MAX_HIT(member));
        cJSON_AddNumberToObject(member_obj, "move", GET_VITALITY(member));
        cJSON_AddNumberToObject(member_obj, "maxMove", GET_MAX_VITALITY(member));

        /* Position - POS_PRONE=0, POS_SITTING=1, POS_KNEELING=2, POS_STANDING=3 */
        const char *pos_str;
        switch (GET_POS(member)) {
            case POS_PRONE:     pos_str = "prone"; break;
            case POS_SITTING:   pos_str = "sitting"; break;
            case POS_KNEELING:  pos_str = "kneeling"; break;
            case POS_STANDING:  pos_str = "standing"; break;
            default:            pos_str = "standing"; break;
        }
        cJSON_AddStringToObject(member_obj, "position", pos_str);

        /* Rank - head for leader, front/back based on PLR2_BACK_RANK flag */
        const char *rank_str;
        if (is_first) {
            rank_str = "head";
            is_first = 0;
        } else if (!IS_NPC(member) && PLR2_FLAGGED(member, PLR2_BACK_RANK)) {
            rank_str = "back";
        } else {
            rank_str = "front";
        }
        cJSON_AddStringToObject(member_obj, "rank", rank_str);

        /* isNpc */
        cJSON_AddBoolToObject(member_obj, "isNpc", IS_NPC(member) ? 1 : 0);

        /* inRoom - is this member in the same room as the viewer? */
        cJSON_AddBoolToObject(member_obj, "inRoom", (member->in_room == ch->in_room) ? 1 : 0);

        /* For NPCs, calculate target number and keyword */
        if (IS_NPC(member) && member->in_room == ch->in_room) {
            /* Count matching mobs from end of room list (LIFO) to get target number */
            struct char_data *tch;
            int target_num = 0;
            const char *first_keyword = NULL;

            /* Get first keyword from NPC's name list */
            if (member->player.name) {
                static char keyword_buf[64];
                strncpy(keyword_buf, member->player.name, sizeof(keyword_buf) - 1);
                keyword_buf[sizeof(keyword_buf) - 1] = '\0';
                /* Get first word (keyword) */
                char *space = strchr(keyword_buf, ' ');
                if (space) *space = '\0';
                first_keyword = keyword_buf;
            }

            /* Count matching mobs in room - LIFO order means last in = #1 */
            if (first_keyword) {
                for (tch = world[ch->in_room].people; tch; tch = tch->next_in_room) {
                    if (IS_NPC(tch) && tch->player.name) {
                        /* Check if this mob matches the keyword */
                        if (isname(first_keyword, tch->player.name)) {
                            target_num++;
                            if (tch == member) {
                                /* Found our member - target_num is now correct (LIFO) */
                                break;
                            }
                        }
                    }
                }
            }

            if (target_num > 0 && first_keyword) {
                cJSON_AddNumberToObject(member_obj, "targetNum", target_num);
                cJSON_AddStringToObject(member_obj, "targetKeyword", first_keyword);
            } else {
                cJSON_AddNullToObject(member_obj, "targetNum");
                cJSON_AddNullToObject(member_obj, "targetKeyword");
            }
        } else {
            cJSON_AddNullToObject(member_obj, "targetNum");
            cJSON_AddNullToObject(member_obj, "targetKeyword");
        }

        cJSON_AddItemToArray(members_arr, member_obj);
        count++;
    }

    cJSON_AddItemToObject(root, "members", members_arr);
    cJSON_AddNumberToObject(root, "size", count);
    cJSON_AddNumberToObject(root, "maxSize", max_size);

    json_str = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);

    if (json_str) {
        gmcp_send(ch->desc, GMCP_PKG_GROUP_STATUS, json_str);
        free(json_str);
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
 * Send channel message via GMCP with alignment (for nchat)
 */
void gmcp_comm_channel_ex(struct char_data *ch, const char *channel,
                          const char *sender, const char *text,
                          const char *alignment) {
    char *json;

    if (!ch || !ch->desc) return;
    if (!GMCP_ENABLED(ch)) return;

    json = json_build_comm_channel_ex(channel, sender, text, alignment);

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

    /* Send quest map once per session if player has one purchased */
    if (ch->only.pc && ch->only.pc->quest_map_bought == 1 &&
        !ch->desc->gmcp_quest_map_sent) {
        gmcp_quest_map(ch);
        ch->desc->gmcp_quest_map_sent = 1;
    }
}

/*
 * Generate and send Quest.Map for bartender quests
 * Temporarily teleports to quest map room to generate the map
 */
void gmcp_quest_map(struct char_data *ch) {
    int old_room, map_room;

    if (!ch || !ch->desc) return;
    if (!GMCP_ENABLED(ch)) return;
    if (!ch->only.pc) return;
    if (ch->only.pc->quest_map_bought != 1) return;

    map_room = real_room(ch->only.pc->quest_map_room);
    if (map_room < 0) return;

    /* Temporarily move to map room */
    old_room = ch->in_room;
    char_from_room(ch);
    char_to_room(ch, map_room, -2);

    /* Generate and send quest map (20 tile radius, always show) */
    display_map_room(ch, map_room, 20, 999, 1);  /* 1 = Quest.Map */

    /* Return to original room */
    char_from_room(ch);
    char_to_room(ch, old_room, -2);
}
