/*
 * json_utils.h - JSON utilities for DurisMUD WebSocket protocol
 *
 * Provides helper functions for building and parsing JSON messages
 * used in WebSocket communication with web clients.
 *
 * Uses cJSON library (https://github.com/DaveGamble/cJSON)
 *
 * INSTALLATION:
 *   Fedora/RHEL: dnf install cjson-devel
 *   Ubuntu/Debian: apt install libcjson-dev
 *   Or download from: https://github.com/DaveGamble/cJSON
 */

#ifndef JSON_UTILS_H
#define JSON_UTILS_H

#include <cjson/cJSON.h>
#include "structs.h"

/*
 * Message Type Constants
 */
#define MSG_TYPE_CMD        "cmd"
#define MSG_TYPE_AUTH       "auth"
#define MSG_TYPE_CHARGEN    "chargen"
#define MSG_TYPE_GMCP       "gmcp"
#define MSG_TYPE_TEXT       "text"

/*
 * WebSocket Command Constants (prefixed to avoid collision with interp.h CMD_*)
 */
#define WS_CMD_LOGIN           "login"
#define WS_CMD_REGISTER        "register"
#define WS_CMD_CHARGEN_OPTIONS "chargen_options"
#define WS_CMD_ROLL_STATS      "roll_stats"
#define WS_CMD_CREATE_CHAR     "create_character"
#define WS_CMD_ENTER           "enter"
#define WS_CMD_GAME            "game"

/*
 * Auth Status Constants
 */
#define AUTH_SUCCESS        "success"
#define AUTH_REGISTERED     "registered"
#define AUTH_FAILED         "failed"

/*
 * Text Categories
 */
#define TEXT_COMBAT         "combat"
#define TEXT_MOVEMENT       "movement"
#define TEXT_INFO           "info"
#define TEXT_SYSTEM         "system"
#define TEXT_CHANNEL        "channel"

/*
 * GMCP Package Names (Mudlet/IRE compatible)
 */
#define GMCP_ROOM_INFO      "Room.Info"
#define GMCP_CHAR_VITALS    "Char.Vitals"
#define GMCP_CHAR_STATUS    "Char.Status"
#define GMCP_CHAR_AFFECTS   "Char.Affects"
#define GMCP_COMBAT_UPDATE  "Combat.Update"
#define GMCP_COMM_CHANNEL   "Comm.Channel"

/*
 * Parsing Functions
 */

/* Parse incoming JSON command, returns cJSON object or NULL on error */
cJSON *json_parse_command(const char *json_str);

/* Get command type from parsed JSON */
const char *json_get_cmd_type(cJSON *json);

/* Get command name from parsed JSON */
const char *json_get_cmd_name(cJSON *json);

/* Get data object from parsed JSON */
cJSON *json_get_data(cJSON *json);

/* Get string from data object */
const char *json_get_string(cJSON *data, const char *key);

/* Get integer from data object */
int json_get_int(cJSON *data, const char *key, int default_val);

/*
 * Response Building Functions
 */

/* Build auth success response with character list
 * Returns malloc'd string, caller must free */
char *json_build_auth_success(const char *account, struct char_data *char_list);

/* Build auth failure response
 * Returns malloc'd string, caller must free */
char *json_build_auth_failed(const char *error);

/* Build stat roll response
 * Returns malloc'd string, caller must free */
char *json_build_stat_roll(int *rolls, int *racial_mods, int *final, int rerolls);

/* Build chargen complete response
 * Returns malloc'd string, caller must free */
char *json_build_chargen_complete(const char *name, const char *message);

/*
 * GMCP Building Functions
 */

/* Build Room.Info GMCP message
 * ch is the viewing character (for visibility checks), can be NULL
 * Returns malloc'd string, caller must free */
char *json_build_room_info(struct room_data *room, struct char_data *ch);

/* Build Char.Vitals GMCP message
 * Returns malloc'd string, caller must free */
char *json_build_char_vitals(struct char_data *ch);

/* Build Char.Status GMCP message
 * Returns malloc'd string, caller must free */
char *json_build_char_status(struct char_data *ch);

/* Build Char.Affects GMCP message
 * Returns malloc'd string, caller must free */
char *json_build_char_affects(struct char_data *ch);

/* Build Combat.Update GMCP message
 * Returns malloc'd string, caller must free */
char *json_build_combat_update(struct char_data *ch, struct char_data *victim,
                               int damage, const char *damage_type, int critical);

/* Build Comm.Channel GMCP message
 * Returns malloc'd string, caller must free */
char *json_build_comm_channel(const char *channel, const char *sender,
                              const char *text);

/* Build Quest.Status GMCP message
 * Returns malloc'd string, caller must free */
char *json_build_quest_status(struct char_data *ch);

/*
 * Text Message Building
 */

/* Build text message with category
 * Returns malloc'd string, caller must free */
char *json_build_text(const char *category, const char *text);

/*
 * Wrapper for complete GMCP message (with type and package)
 */

/* Build complete GMCP JSON wrapper
 * Returns malloc'd string, caller must free */
char *json_build_gmcp_message(const char *package, const char *data_json);

/*
 * Utility Functions
 */

/* Escape string for JSON (handles quotes, newlines, etc.)
 * Returns malloc'd string, caller must free */
char *json_escape_string(const char *str);

/* Strip ANSI codes and escape for JSON
 * Returns malloc'd string, caller must free */
char *json_escape_ansi_string(const char *str);

/* Free JSON string (convenience wrapper) */
void json_free_string(char *str);

#endif /* JSON_UTILS_H */
