/*
 * json_utils.c - JSON utilities for DurisMUD WebSocket protocol
 *
 * Provides helper functions for building and parsing JSON messages
 * used in WebSocket communication with web clients.
 *
 * Copyright (c) 2025 DurisMUD Development Team
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "json_utils.h"
#include "structs.h"
#include "defines.h"
#include "prototypes.h"
#include "utils.h"
#include "db.h"
#include "handler.h"
#include "comm.h"
#include "spells.h"
#include "specializations.h"
#include "assocs.h"

/* External declarations */
extern const char *class_abbrevs[];
extern const char *race_abbrevs[];
extern const char *specdata[][MAX_SPEC];
extern const char *pc_class_types[];
extern const struct race_names race_names_table[];
extern struct room_data *world;
extern struct zone_data *zone_table;
extern struct index_data *mob_index;
extern struct index_data *obj_index;
extern struct time_info_data time_info;
extern const char *month_name[];

/* SQL function for quest remaining count */
int sql_world_quest_can_do_another(struct char_data *ch);

/*
 * Parse incoming JSON command
 */
cJSON *json_parse_command(const char *json_str) {
    if (!json_str || !*json_str) return NULL;
    return cJSON_Parse(json_str);
}

/*
 * Get command type from parsed JSON
 */
const char *json_get_cmd_type(cJSON *json) {
    cJSON *type = cJSON_GetObjectItem(json, "type");
    if (type && cJSON_IsString(type)) {
        return type->valuestring;
    }
    return NULL;
}

/*
 * Get command name from parsed JSON
 */
const char *json_get_cmd_name(cJSON *json) {
    cJSON *cmd = cJSON_GetObjectItem(json, "cmd");
    if (cmd && cJSON_IsString(cmd)) {
        return cmd->valuestring;
    }
    return NULL;
}

/*
 * Get data object from parsed JSON
 */
cJSON *json_get_data(cJSON *json) {
    return cJSON_GetObjectItem(json, "data");
}

/*
 * Get string from data object
 */
const char *json_get_string(cJSON *data, const char *key) {
    cJSON *item;
    if (!data) return NULL;

    /* If data is a string itself (for simple commands like "game") */
    if (cJSON_IsString(data) && key == NULL) {
        return data->valuestring;
    }

    item = cJSON_GetObjectItem(data, key);
    if (item && cJSON_IsString(item)) {
        return item->valuestring;
    }
    return NULL;
}

/*
 * Get integer from data object
 */
int json_get_int(cJSON *data, const char *key, int default_val) {
    cJSON *item;
    if (!data) return default_val;

    item = cJSON_GetObjectItem(data, key);
    if (item && cJSON_IsNumber(item)) {
        return item->valueint;
    }
    return default_val;
}

/*
 * Escape string for JSON
 */
char *json_escape_string(const char *str) {
    size_t len, i, j;
    char *escaped;
    unsigned char c;

    if (!str) return strdup("");

    /* Calculate required length - skip non-ASCII bytes for UTF-8 safety */
    len = 0;
    for (i = 0; str[i]; i++) {
        c = (unsigned char)str[i];
        if (c >= 128) continue; /* Skip non-ASCII bytes */
        switch (str[i]) {
            case '"':
            case '\\':
            case '\n':
            case '\r':
            case '\t':
                len += 2;
                break;
            default:
                if (c < 32) {
                    len += 6; /* \uXXXX */
                } else {
                    len++;
                }
        }
    }

    escaped = (char *)malloc(len + 1);
    if (!escaped) return NULL;

    j = 0;
    for (i = 0; str[i]; i++) {
        c = (unsigned char)str[i];
        if (c >= 128) continue; /* Skip non-ASCII bytes for UTF-8 safety */
        switch (str[i]) {
            case '"':
                escaped[j++] = '\\';
                escaped[j++] = '"';
                break;
            case '\\':
                escaped[j++] = '\\';
                escaped[j++] = '\\';
                break;
            case '\n':
                escaped[j++] = '\\';
                escaped[j++] = 'n';
                break;
            case '\r':
                escaped[j++] = '\\';
                escaped[j++] = 'r';
                break;
            case '\t':
                escaped[j++] = '\\';
                escaped[j++] = 't';
                break;
            default:
                if (c < 32) {
                    j += sprintf(escaped + j, "\\u%04x", c);
                } else {
                    escaped[j++] = str[i];
                }
        }
    }
    escaped[j] = '\0';

    return escaped;
}

/*
 * Strip ANSI color codes and escape for JSON
 * Handles: &+X, &-X, &=XY, &n, &N (DurisMUD color codes)
 */
char *json_escape_ansi_string(const char *str) {
    char *stripped, *escaped;
    size_t len, i, j;

    if (!str) return strdup("");

    /* First pass: calculate length without color codes */
    len = 0;
    for (i = 0; str[i]; i++) {
        if (str[i] == '&' && str[i+1]) {
            if (str[i+1] == '+' || str[i+1] == '-') {
                /* &+X or &-X - skip 3 chars total */
                i += 2;  /* loop will add 1 more */
            } else if (str[i+1] == '=') {
                /* &=XY - skip 4 chars total */
                i += 3;  /* loop will add 1 more */
            } else if (str[i+1] == 'n' || str[i+1] == 'N') {
                /* &n or &N - skip 2 chars total */
                i += 1;  /* loop will add 1 more */
            } else {
                len++;  /* Not a color code, keep the '&' */
            }
        } else {
            len++;
        }
    }

    stripped = (char *)malloc(len + 1);
    if (!stripped) return NULL;

    /* Second pass: copy without color codes */
    j = 0;
    for (i = 0; str[i]; i++) {
        if (str[i] == '&' && str[i+1]) {
            if (str[i+1] == '+' || str[i+1] == '-') {
                i += 2;
            } else if (str[i+1] == '=') {
                i += 3;
            } else if (str[i+1] == 'n' || str[i+1] == 'N') {
                i += 1;
            } else {
                stripped[j++] = str[i];
            }
        } else {
            stripped[j++] = str[i];
        }
    }
    stripped[j] = '\0';

    /* Now escape for JSON */
    escaped = json_escape_string(stripped);
    free(stripped);

    return escaped;
}

/*
 * Free JSON string
 */
void json_free_string(char *str) {
    if (str) free(str);
}

/*
 * Build auth success response
 */
char *json_build_auth_success(const char *account, struct char_data *char_list) {
    cJSON *root, *data, *characters, *char_obj;
    struct char_data *ch;
    char *result;

    root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "type", MSG_TYPE_AUTH);
    cJSON_AddStringToObject(root, "status", AUTH_SUCCESS);

    data = cJSON_CreateObject();
    cJSON_AddStringToObject(data, "account", account);

    characters = cJSON_CreateArray();

    /* Add each character */
    for (ch = char_list; ch; ch = ch->next) {
        char_obj = cJSON_CreateObject();
        cJSON_AddStringToObject(char_obj, "name", GET_NAME(ch));
        cJSON_AddStringToObject(char_obj, "class", get_class_name(ch, NULL));
        cJSON_AddNumberToObject(char_obj, "level", GET_LEVEL(ch));
        cJSON_AddStringToObject(char_obj, "race", race_names_table[(int)GET_RACE(ch)].normal);
        cJSON_AddItemToArray(characters, char_obj);
    }

    cJSON_AddItemToObject(data, "characters", characters);
    cJSON_AddItemToObject(root, "data", data);

    result = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);

    return result;
}

/*
 * Build auth failure response
 */
char *json_build_auth_failed(const char *error) {
    cJSON *root;
    char *result;

    root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "type", MSG_TYPE_AUTH);
    cJSON_AddStringToObject(root, "status", AUTH_FAILED);
    cJSON_AddStringToObject(root, "error", error ? error : "Unknown error");

    result = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);

    return result;
}

/*
 * Build text message with category
 */
char *json_build_text(const char *category, const char *text) {
    cJSON *root;
    char *result;

    root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "type", MSG_TYPE_TEXT);
    cJSON_AddStringToObject(root, "category", category ? category : TEXT_INFO);
    cJSON_AddStringToObject(root, "data", text ? text : "");

    result = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);

    return result;
}

/*
 * Build complete GMCP JSON wrapper
 */
char *json_build_gmcp_message(const char *package, const char *data_json) {
    cJSON *root, *data;
    char *result;

    root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "type", MSG_TYPE_GMCP);
    cJSON_AddStringToObject(root, "package", package);

    /* Parse the data JSON and add it */
    data = cJSON_Parse(data_json);
    if (data) {
        cJSON_AddItemToObject(root, "data", data);
    } else {
        cJSON_AddNullToObject(root, "data");
    }

    result = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);

    return result;
}

/*
 * Build Room.Info GMCP message data
 * Follows standard GMCP Room.Info format for compatibility with MUD clients
 * ch is the viewing character (for visibility checks), can be NULL
 */
char *json_build_room_info(struct room_data *room, struct char_data *ch) {
    cJSON *root, *exits, *coords, *doors, *players, *npcs, *items;
    char *result;
    char *clean_name, *clean_area;
    int dir;
    struct char_data *tch;
    struct obj_data *obj;
    /* Standard GMCP uses short exit names */
    const char *dir_abbrevs[] = {
        "n", "e", "s", "w", "u", "d",
        "ne", "nw", "se", "sw"
    };
    /* Terrain type to environment string mapping */
    const char *env_names[] = {
        "inside", "city", "field", "forest", "hills", "mountains",
        "water", "water", "air", "underwater", "desert", "arctic",
        "swamp", "ocean", "path", "road", "jungle", "cavern"
    };

    if (!room) return strdup("{}");

    root = cJSON_CreateObject();

    /* Standard field: num (not vnum) - hide for wilderness zones */
    int zone_num = (room->zone >= 0) ? zone_table[room->zone].number : 0;
    bool is_wilderness = (zone_num == 600 ||                          // The Adventurers Shipyards
                          (zone_num >= 1200 && zone_num <= 1238) ||   // Alatorin
                          (zone_num >= 5000 && zone_num <= 6599) ||   // Surface
                          (zone_num >= 6600 && zone_num <= 6999) ||   // Newbie Maps
                          (zone_num >= 7000 && zone_num <= 8599));    // Underdark
    if (is_wilderness) {
        cJSON_AddNumberToObject(root, "num", 0);
    } else {
        cJSON_AddNumberToObject(root, "num", room->number);
    }

    /* Room name - clean for Mudlet, colored for web */
    clean_name = json_escape_ansi_string(room->name ? room->name : "Unknown");
    cJSON_AddStringToObject(root, "name", clean_name);
    free(clean_name);
    clean_name = json_escape_string(room->name ? room->name : "Unknown");
    cJSON_AddStringToObject(root, "colored_name", clean_name);
    free(clean_name);

    /* Area name - clean for Mudlet, colored for web */
    if (room->zone >= 0 && zone_table[room->zone].name) {
        clean_area = json_escape_ansi_string(zone_table[room->zone].name);
        cJSON_AddStringToObject(root, "area", clean_area);
        free(clean_area);
        clean_area = json_escape_string(zone_table[room->zone].name);
        cJSON_AddStringToObject(root, "colored_area", clean_area);
        free(clean_area);
    } else {
        cJSON_AddStringToObject(root, "area", "Unknown");
        cJSON_AddStringToObject(root, "colored_area", "Unknown");
    }

    /* Standard field: environment (string, not terrain number) */
    if (room->sector_type >= 0 && room->sector_type < 18) {
        cJSON_AddStringToObject(root, "environment", env_names[room->sector_type]);
    } else {
        cJSON_AddStringToObject(root, "environment", "unknown");
    }

    /* Standard field: coords object - hide for wilderness zones */
    coords = cJSON_CreateObject();
    if (!is_wilderness) {
        cJSON_AddNumberToObject(coords, "x", room->x_coord);
        cJSON_AddNumberToObject(coords, "y", room->y_coord);
        cJSON_AddNumberToObject(coords, "z", room->z_coord);
    }
    cJSON_AddItemToObject(root, "coords", coords);

    /* Build exits object - standard format: "n": 12345 (just the room number) */
    /* Skip secret exits unless the door is open (discovered) */
    /* Hide vnums for wilderness zones (show true instead) */
    exits = cJSON_CreateObject();
    for (dir = 0; dir < 10; dir++) {
        if (room->dir_option[dir] && room->dir_option[dir]->to_room != NOWHERE) {
            /* Hide secret exits unless door is open */
            if (IS_SET(room->dir_option[dir]->exit_info, EX_SECRET) &&
                IS_SET(room->dir_option[dir]->exit_info, EX_CLOSED)) {
                continue;
            }
            if (is_wilderness) {
                cJSON_AddBoolToObject(exits, dir_abbrevs[dir], 1);
            } else {
                cJSON_AddNumberToObject(exits, dir_abbrevs[dir],
                    world[room->dir_option[dir]->to_room].number);
            }
        }
    }
    cJSON_AddItemToObject(root, "exits", exits);

    /* Build doors object - only for exits that have doors */
    /* Skip secret doors that are still closed (undiscovered) */
    doors = cJSON_CreateObject();
    for (dir = 0; dir < 10; dir++) {
        if (room->dir_option[dir] &&
            IS_SET(room->dir_option[dir]->exit_info, EX_ISDOOR)) {
            /* Hide secret doors unless they are open (discovered) */
            if (IS_SET(room->dir_option[dir]->exit_info, EX_SECRET) &&
                IS_SET(room->dir_option[dir]->exit_info, EX_CLOSED)) {
                continue;
            }
            cJSON *door = cJSON_CreateObject();
            char *clean_keyword = json_escape_ansi_string(
                room->dir_option[dir]->keyword ? room->dir_option[dir]->keyword : "door");
            cJSON_AddStringToObject(door, "name", clean_keyword);
            free(clean_keyword);
            cJSON_AddBoolToObject(door, "closed",
                IS_SET(room->dir_option[dir]->exit_info, EX_CLOSED));
            cJSON_AddBoolToObject(door, "locked",
                IS_SET(room->dir_option[dir]->exit_info, EX_LOCKED));
            cJSON_AddItemToObject(doors, dir_abbrevs[dir], door);
        }
    }
    cJSON_AddItemToObject(root, "doors", doors);

    /* Build players array - visible PCs in room */
    players = cJSON_CreateArray();
    if (ch) {
        for (tch = room->people; tch; tch = tch->next_in_room) {
            if (tch == ch) continue;  /* Skip self */
            if (IS_NPC(tch)) continue;  /* Skip NPCs */
            if (!CAN_SEE(ch, tch)) continue;  /* Visibility check */

            cJSON *player = cJSON_CreateObject();
            clean_name = json_escape_ansi_string(GET_NAME(tch));
            cJSON_AddStringToObject(player, "name", clean_name);
            free(clean_name);
            cJSON_AddItemToArray(players, player);
        }
    }
    cJSON_AddItemToObject(root, "players", players);

    /* Build npcs array - visible NPCs in room */
    npcs = cJSON_CreateArray();
    if (ch) {
        for (tch = room->people; tch; tch = tch->next_in_room) {
            if (!IS_NPC(tch)) continue;  /* Skip PCs */
            if (!CAN_SEE(ch, tch)) continue;  /* Visibility check */

            cJSON *npc = cJSON_CreateObject();
            /* Clean name for Mudlet compatibility - use short_descr for NPCs */
            clean_name = json_escape_ansi_string(tch->player.short_descr ? tch->player.short_descr : GET_NAME(tch));
            cJSON_AddStringToObject(npc, "name", clean_name);
            free(clean_name);
            /* Colored name for web client - use short_descr which has ANSI codes */
            clean_name = json_escape_string(tch->player.short_descr ? tch->player.short_descr : GET_NAME(tch));
            cJSON_AddStringToObject(npc, "colored_name", clean_name);
            free(clean_name);
            cJSON_AddNumberToObject(npc, "vnum", GET_VNUM(tch));
            /* Extract first keyword from NPC's name list for targeting */
            if (tch->player.name) {
                char keyword_buf[64];
                strncpy(keyword_buf, tch->player.name, sizeof(keyword_buf) - 1);
                keyword_buf[sizeof(keyword_buf) - 1] = '\0';
                char *space = strchr(keyword_buf, ' ');
                if (space) *space = '\0';  /* Get first word only */
                cJSON_AddStringToObject(npc, "keyword", keyword_buf);
            }
            cJSON_AddItemToArray(npcs, npc);
        }
    }
    cJSON_AddItemToObject(root, "npcs", npcs);

    /* Build items array - visible objects in room */
    items = cJSON_CreateArray();
    if (ch) {
        for (obj = room->contents; obj; obj = obj->next_content) {
            if (!CAN_SEE_OBJ(ch, obj)) continue;  /* Visibility check */

            cJSON *item = cJSON_CreateObject();
            /* Clean name for Mudlet compatibility */
            clean_name = json_escape_ansi_string(obj->short_description);
            cJSON_AddStringToObject(item, "name", clean_name);
            free(clean_name);
            /* Colored name for web client */
            clean_name = json_escape_string(obj->short_description);
            cJSON_AddStringToObject(item, "colored_name", clean_name);
            free(clean_name);
            cJSON_AddNumberToObject(item, "vnum", OBJ_VNUM(obj));
            cJSON_AddItemToArray(items, item);
        }
    }
    cJSON_AddItemToObject(root, "items", items);

    /* DurisMUD extensions (beyond standard GMCP) */
    if (room->zone >= 0) {
        cJSON_AddNumberToObject(root, "zone", zone_table[room->zone].number);
    } else {
        cJSON_AddNumberToObject(root, "zone", 0);
    }
    cJSON_AddNumberToObject(root, "terrain", room->sector_type);

    result = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);

    return result;
}

/*
 * Build Char.Vitals GMCP message data
 */
char *json_build_char_vitals(struct char_data *ch) {
    cJSON *root;
    char *result;

    if (!ch) return strdup("{}");

    root = cJSON_CreateObject();
    cJSON_AddNumberToObject(root, "hp", GET_HIT(ch));
    cJSON_AddNumberToObject(root, "maxHp", GET_MAX_HIT(ch));
    cJSON_AddNumberToObject(root, "mana", GET_MANA(ch));
    cJSON_AddNumberToObject(root, "maxMana", GET_MAX_MANA(ch));
    cJSON_AddNumberToObject(root, "move", GET_VITALITY(ch));
    cJSON_AddNumberToObject(root, "maxMove", GET_MAX_VITALITY(ch));
    cJSON_AddNumberToObject(root, "exp", GET_EXP(ch));

    /* Experience to next level */
    extern long new_exp_table[];
    long tnl = 0;
    if (GET_LEVEL(ch) < MAXLVLMORTAL) {
        tnl = new_exp_table[GET_LEVEL(ch) + 1] - GET_EXP(ch);
        if (tnl < 0) tnl = 0;
    }
    cJSON_AddNumberToObject(root, "tnl", tnl);

    /* All coin types */
    cJSON_AddNumberToObject(root, "platinum", GET_PLATINUM(ch));
    cJSON_AddNumberToObject(root, "gold", GET_GOLD(ch));
    cJSON_AddNumberToObject(root, "silver", GET_SILVER(ch));
    cJSON_AddNumberToObject(root, "copper", GET_COPPER(ch));

    /* Position - matches POS_ constants in defines.h */
    const char *positions[] = {
        "on your ass", "sitting", "kneeling", "standing"
    };
    int pos = GET_POS(ch);
    if (pos >= 0 && pos <= 3) {
        cJSON_AddStringToObject(root, "position", positions[pos]);
    } else {
        cJSON_AddStringToObject(root, "position", "standing");
    }

    /* Fighting target */
    if (IS_FIGHTING(ch) && GET_OPPONENT(ch) && GET_NAME(GET_OPPONENT(ch))) {
        cJSON_AddStringToObject(root, "fighting", GET_NAME(GET_OPPONENT(ch)));
    } else {
        cJSON_AddNullToObject(root, "fighting");
    }

    /* Whether character uses mana (Psionicist, Mindflayer, Illithid/Pillithid) */
    cJSON_AddBoolToObject(root, "usesMana", USES_MANA(ch) ? 1 : 0);

    result = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);

    return result;
}

/*
 * Build Char.Status GMCP message data
 */
char *json_build_char_status(struct char_data *ch) {
    cJSON *root;
    char *result;
    int race;
    const char *class_name;

    if (!ch) return strdup("{}");

    root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "name", GET_NAME(ch) ? GET_NAME(ch) : "Unknown");
    cJSON_AddNumberToObject(root, "level", GET_LEVEL(ch));

    /* Use get_class_string which handles specialization and multiclass */
    char class_buffer[MAX_STRING_LENGTH];
    get_class_string(ch, class_buffer);
    cJSON_AddStringToObject(root, "class", class_buffer[0] ? class_buffer : "Unknown");

    race = (int)GET_RACE(ch);
    if (race >= 0 && race <= LAST_RACE) {
        cJSON_AddStringToObject(root, "race", race_names_table[race].normal);
    } else {
        cJSON_AddStringToObject(root, "race", "Unknown");
    }

    /* Alignment */
    if (IS_GOOD(ch)) {
        cJSON_AddStringToObject(root, "alignment", "good");
    } else if (IS_EVIL(ch)) {
        cJSON_AddStringToObject(root, "alignment", "evil");
    } else {
        cJSON_AddStringToObject(root, "alignment", "neutral");
    }

    /* Guild */
    if (GET_ASSOC(ch) && IS_MEMBER(GET_A_BITS(ch))) {
        cJSON_AddStringToObject(root, "guild", GET_ASSOC(ch)->get_name().c_str());
    } else {
        cJSON_AddStringToObject(root, "guild", "");
    }

    /* Title */
    if (GET_TITLE(ch)) {
        cJSON_AddStringToObject(root, "title", GET_TITLE(ch));
    } else {
        cJSON_AddStringToObject(root, "title", "");
    }

    result = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);

    return result;
}

/*
 * Build Char.Affects GMCP message data
 */
char *json_build_char_affects(struct char_data *ch) {
    extern Skill skills[];
    cJSON *root, *affect_obj;
    struct affected_type *aff;
    char *result;
    int last = -1;

    root = cJSON_CreateArray();

    if (ch) {
        for (aff = ch->affected; aff; aff = aff->next) {
            /* Match score command logic for showing active spells */
            if (aff->type <= 0 || !skills[aff->type].name || aff->type > LAST_SKILL) {
                continue;
            }
            /* Skip affects marked as hidden */
            if (IS_SET(aff->flags, AFFTYPE_NOSHOW)) {
                continue;
            }
            /* Skip duplicate affect types (spells can have multiple affect structs) */
            if (aff->type == last) {
                continue;
            }
            last = aff->type;

            affect_obj = cJSON_CreateObject();
            cJSON_AddStringToObject(affect_obj, "name", skills[aff->type].name);
            cJSON_AddNumberToObject(affect_obj, "id", aff->type);
            /* Send duration in seconds for frontend countdown timer */
            cJSON_AddNumberToObject(affect_obj, "duration", aff->duration * 60);
            cJSON_AddStringToObject(affect_obj, "icon", "shield"); /* Default icon */
            cJSON_AddItemToArray(root, affect_obj);
        }
    }

    result = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);

    return result;
}

/*
 * Build Comm.Channel GMCP message data
 */
char *json_build_comm_channel(const char *channel, const char *sender,
                              const char *text) {
    cJSON *root;
    char *result;
    time_t now;

    root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "channel", channel ? channel : "info");
    cJSON_AddStringToObject(root, "sender", sender ? sender : "System");
    cJSON_AddStringToObject(root, "text", text ? text : "");

    time(&now);
    cJSON_AddNumberToObject(root, "timestamp", (double)now);

    result = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);

    return result;
}

/*
 * Build Comm.Channel GMCP message data with alignment (for nchat)
 */
char *json_build_comm_channel_ex(const char *channel, const char *sender,
                                  const char *text, const char *alignment) {
    cJSON *root;
    char *result;
    time_t now;

    root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "channel", channel ? channel : "info");
    cJSON_AddStringToObject(root, "sender", sender ? sender : "System");
    cJSON_AddStringToObject(root, "text", text ? text : "");

    if (alignment && *alignment) {
        cJSON_AddStringToObject(root, "alignment", alignment);
    }

    time(&now);
    cJSON_AddNumberToObject(root, "timestamp", (double)now);

    result = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);

    return result;
}

/*
 * Build stat roll response
 */
char *json_build_stat_roll(int *rolls, int *racial_mods, int *final, int rerolls) {
    cJSON *root, *data, *rolls_obj, *mods_obj, *final_obj;
    char *result;
    const char *stats[] = {"str", "int", "wis", "dex", "con", "cha"};
    int i;

    root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "type", MSG_TYPE_CHARGEN);
    cJSON_AddStringToObject(root, "step", "stats");

    data = cJSON_CreateObject();

    rolls_obj = cJSON_CreateObject();
    mods_obj = cJSON_CreateObject();
    final_obj = cJSON_CreateObject();

    for (i = 0; i < 6; i++) {
        cJSON_AddNumberToObject(rolls_obj, stats[i], rolls[i]);
        cJSON_AddNumberToObject(mods_obj, stats[i], racial_mods[i]);
        cJSON_AddNumberToObject(final_obj, stats[i], final[i]);
    }

    cJSON_AddItemToObject(data, "rolls", rolls_obj);
    cJSON_AddItemToObject(data, "racial_mods", mods_obj);
    cJSON_AddItemToObject(data, "final", final_obj);
    cJSON_AddNumberToObject(data, "rerolls_remaining", rerolls);

    cJSON_AddItemToObject(root, "data", data);

    result = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);

    return result;
}

/*
 * Build chargen complete response
 */
char *json_build_chargen_complete(const char *name, const char *message) {
    cJSON *root, *data;
    char *result;

    root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "type", MSG_TYPE_CHARGEN);
    cJSON_AddStringToObject(root, "step", "complete");

    data = cJSON_CreateObject();
    cJSON_AddStringToObject(data, "name", name ? name : "");
    cJSON_AddStringToObject(data, "message", message ? message : "Character created!");

    cJSON_AddItemToObject(root, "data", data);

    result = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);

    return result;
}

/*
 * Build Quest.Status GMCP message
 * Returns quest data for bartender quests
 */
char *json_build_quest_status(struct char_data *ch) {
    cJSON *root;
    char *result;
    char target_buf[MAX_STRING_LENGTH];
    struct char_data *q_mob = NULL;
    int remaining;

    if (!ch || IS_NPC(ch)) return NULL;

    root = cJSON_CreateObject();

    /* Get remaining quests for today */
    remaining = sql_world_quest_can_do_another(ch);
    cJSON_AddNumberToObject(root, "remaining", remaining);

    /* Check if quest is active */
    if (ch->only.pc->quest_active == 1) {
        cJSON_AddBoolToObject(root, "active", 1);

        /* Quest type */
        if (ch->only.pc->quest_type == FIND_AND_ASK) {
            cJSON_AddStringToObject(root, "type", "ask");
        } else if (ch->only.pc->quest_type == FIND_AND_KILL) {
            cJSON_AddStringToObject(root, "type", "kill");
            cJSON_AddNumberToObject(root, "killCount", ch->only.pc->quest_kill_how_many);
            cJSON_AddNumberToObject(root, "killRequired", ch->only.pc->quest_kill_original);
        } else {
            cJSON_AddNullToObject(root, "type");
        }

        /* Build target string - need to load mob to get name */
        q_mob = read_mobile(real_mobile(ch->only.pc->quest_mob_vnum), REAL);
        if (q_mob) {
            int zone_idx = real_zone0(ch->only.pc->quest_zone_number);

            if (ch->only.pc->quest_type == FIND_AND_ASK) {
                snprintf(target_buf, MAX_STRING_LENGTH, "Go ask %s in %s about the %s.",
                    q_mob->player.short_descr,
                    zone_table[zone_idx].name,
                    month_name[time_info.month]);
            } else if (ch->only.pc->quest_type == FIND_AND_KILL) {
                int kills_left = ch->only.pc->quest_kill_original - ch->only.pc->quest_kill_how_many;
                snprintf(target_buf, MAX_STRING_LENGTH, "Go kill %d %s (%d left) in %s!",
                    ch->only.pc->quest_kill_original,
                    q_mob->player.short_descr,
                    kills_left,
                    zone_table[zone_idx].name);
            } else {
                snprintf(target_buf, MAX_STRING_LENGTH, "Unknown quest type");
            }

            extract_char(q_mob);
            q_mob = NULL;
        } else {
            snprintf(target_buf, MAX_STRING_LENGTH, "Quest target unavailable");
        }

        cJSON_AddStringToObject(root, "target", target_buf);
        cJSON_AddNumberToObject(root, "zoneNumber", ch->only.pc->quest_zone_number);
    } else {
        cJSON_AddBoolToObject(root, "active", 0);
        cJSON_AddNullToObject(root, "type");
        cJSON_AddStringToObject(root, "target", "");
    }

    /* Always include mapBought status */
    cJSON_AddBoolToObject(root, "mapBought", ch->only.pc->quest_map_bought == 1);

    result = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);

    return result;
}
