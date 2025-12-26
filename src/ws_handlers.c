/*
 * ws_handlers.c - WebSocket command handlers for DurisMUD
 *
 * Handles JSON commands from web clients:
 * - login: Account authentication
 * - register: Account creation
 * - enter: Enter game with character
 * - game: In-game commands
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <sys/stat.h>

#include "ws_handlers.h"
#include "websocket.h"
#include "json_utils.h"
#include "structs.h"
#include "defines.h"
#include "prototypes.h"
#include "utils.h"
#include "comm.h"
#include "db.h"
#include "account.h"
#include "mm.h"
#include "files.h"

/* External declarations */
extern struct descriptor_data *descriptor_list;
extern struct mm_ds *dead_mob_pool;
extern struct mm_ds *dead_pconly_pool;
extern struct room_data *world;
extern long top_of_world;
extern int is_bcrypt_hash(const char *hash);
extern int bcrypt_verify_password(const char *password, const char *hash);
extern char* bcrypt_hash_password(const char *password);
extern bool account_exists(const char *dir, char *name);
extern int read_account(P_acct acct);
extern int write_account(P_acct acct);
extern int is_valid_email(const char *email);
extern bool is_email_taken(const char *email);
extern int restoreCharOnly(P_char ch, char *name);
extern const struct class_names class_names_table[];
extern const struct race_names race_names_table[];
extern int class_table[LAST_RACE + 1][CLASS_COUNT + 1];
extern void roll_basic_attributes(P_char ch, int type);
extern const struct stat_data stat_factor[];
extern const char *stat_to_string2(int val);
extern const char *town_name_list[];
extern const int avail_hometowns[][LAST_RACE + 1];

/* Helper structure for character display */
struct ws_char_info {
    char name[32];
    int level;
    int race;
    int hometown;  /* Room index for last room name */
    char class_str[256];  /* Full class string from get_class_string (e.g., "Cleric / Zealot") */
};

/* Forward declaration for character list builder */
static cJSON *ws_build_character_list(struct descriptor_data *d);

/*
 * Helper function to get race faction from shared playable_races[] array
 * Returns: "good", "evil", "neutral", or "unknown" if not playable
 */
static const char *ws_get_race_faction(int race)
{
    int i;
    for (i = 0; playable_races[i].race_id != -1; i++) {
        if (playable_races[i].race_id == race) {
            return playable_races[i].faction;
        }
    }
    return "unknown";
}

/*
 * Helper function to check if race is playable
 * Uses shared playable_races[] array from constant.c
 */
static int ws_is_playable_race(int race)
{
    int i;
    for (i = 0; playable_races[i].race_id != -1; i++) {
        if (playable_races[i].race_id == race) return 1;
    }
    return 0;
}

/*
 * Helper function to get alignment string from class_table value
 * -1 = evil, 0 = neutral, 1 = good, 2 = any, 3 = good/neutral, 4 = neutral/evil, 5 = forbidden
 */
static const char *ws_get_class_alignment(int value)
{
    switch (value) {
        case -1: return "evil";
        case 0:  return "neutral";
        case 1:  return "good";
        case 2:  return "any";
        case 3:  return "good_neutral";
        case 4:  return "neutral_evil";
        default: return NULL;  /* 5 = forbidden */
    }
}

/*
 * Load basic character info for JSON response
 */
static int ws_load_char_info(const char *charname, struct ws_char_info *info)
{
    extern char *get_class_string(P_char ch, char *strn);
    P_char temp_ch;
    int result;

    temp_ch = (struct char_data *)malloc(sizeof(struct char_data));
    if (!temp_ch) return 0;

    memset(temp_ch, 0, sizeof(struct char_data));

    temp_ch->only.pc = (struct pc_only_data *)malloc(sizeof(struct pc_only_data));
    if (!temp_ch->only.pc) {
        free(temp_ch);
        return 0;
    }

    memset(temp_ch->only.pc, 0, sizeof(struct pc_only_data));

    result = restoreCharOnly(temp_ch, (char *)charname);
    if (result < 0) {
        if (temp_ch->only.pc) free(temp_ch->only.pc);
        free(temp_ch);
        return 0;
    }

    strncpy(info->name, GET_NAME(temp_ch), 31);
    info->name[31] = '\0';
    /* Capitalize first letter */
    if (info->name[0]) info->name[0] = toupper(info->name[0]);

    info->level = GET_LEVEL(temp_ch);
    info->race = GET_RACE(temp_ch);
    info->hometown = GET_HOME(temp_ch);

    /* Use get_class_string like GMCP does - handles spec and multiclass */
    get_class_string(temp_ch, info->class_str);

    if (temp_ch->only.pc) free(temp_ch->only.pc);
    free(temp_ch);

    return 1;
}

/*
 * Get race name string
 */
static const char *ws_get_race_name(int race)
{
    extern const struct race_names race_names_table[];
    if (race >= 0) {
        return race_names_table[race].normal;
    }
    return "Unknown";
}

/*
 * Get class name string
 */
static const char *ws_get_class_name(unsigned int m_class)
{
    int idx = flag2idx(m_class);
    if (idx >= 0) {
        return class_names_table[idx].normal;
    }
    return "Unknown";
}

/*
 * Send auth success message with character list
 */
void ws_send_auth_success(struct descriptor_data *d, const char *account_name)
{
    cJSON *root = cJSON_CreateObject();
    cJSON *data = cJSON_CreateObject();

    cJSON_AddStringToObject(root, "type", "auth");
    cJSON_AddStringToObject(root, "status", "success");

    cJSON_AddStringToObject(data, "account", account_name);

    /* Build character list using shared helper */
    cJSON_AddItemToObject(data, "characters", ws_build_character_list(d));
    cJSON_AddItemToObject(root, "data", data);

    char *json_str = cJSON_PrintUnformatted(root);
    if (json_str) {
        websocket_send_text(d, json_str);
        free(json_str);
    }

    cJSON_Delete(root);
}

/*
 * Send reconnect success message - used when player reconnects to linkdead character
 */
void ws_send_reconnect_success(struct descriptor_data *d, const char *account_name, const char *char_name)
{
    cJSON *root = cJSON_CreateObject();
    cJSON *data = cJSON_CreateObject();
    cJSON *character = cJSON_CreateObject();

    cJSON_AddStringToObject(root, "type", "auth");
    cJSON_AddStringToObject(root, "status", "reconnected");

    cJSON_AddStringToObject(data, "account", account_name);

    /* Add reconnected character info */
    if (d->character) {
        cJSON_AddStringToObject(character, "name", GET_NAME(d->character));
        cJSON_AddNumberToObject(character, "level", GET_LEVEL(d->character));
        cJSON_AddStringToObject(character, "race", ws_get_race_name(GET_RACE(d->character)));
        cJSON_AddStringToObject(character, "class", ws_get_class_name(d->character->player.m_class));
    } else {
        cJSON_AddStringToObject(character, "name", char_name);
    }

    cJSON_AddItemToObject(data, "character", character);
    cJSON_AddItemToObject(root, "data", data);

    char *json_str = cJSON_PrintUnformatted(root);
    if (json_str) {
        websocket_send_text(d, json_str);
        free(json_str);
    }

    cJSON_Delete(root);
}

/*
 * Send full game state after reconnection
 * This resynchronizes the client with room, vitals, affects, etc.
 */
void ws_send_full_game_state(struct descriptor_data *d)
{
    if (!d || !d->character) return;

    /* Trigger GMCP updates to send current state */
    /* Room info */
    if (d->character->in_room >= 0) {
        extern void gmcp_room_info(struct char_data *ch);
        extern void gmcp_room_map(struct char_data *ch);
        gmcp_room_info(d->character);
        gmcp_room_map(d->character);
    }

    /* Character vitals */
    extern void gmcp_char_vitals(struct char_data *ch);
    gmcp_char_vitals(d->character);

    /* Character status */
    extern void gmcp_char_status(struct char_data *ch);
    gmcp_char_status(d->character);

    /* Character affects */
    extern void gmcp_char_affects(struct char_data *ch);
    gmcp_char_affects(d->character);

    /* Quest status */
    extern void gmcp_quest_status(struct char_data *ch);
    gmcp_quest_status(d->character);

    /* Send a "look" to show the room */
    write_to_q("look", &d->input, 0);
}

/*
 * Send auth failed message
 */
void ws_send_auth_failed(struct descriptor_data *d, const char *error)
{
    cJSON *root = cJSON_CreateObject();

    cJSON_AddStringToObject(root, "type", "auth");
    cJSON_AddStringToObject(root, "status", "failed");
    cJSON_AddStringToObject(root, "error", error);

    char *json_str = cJSON_PrintUnformatted(root);
    if (json_str) {
        websocket_send_text(d, json_str);
        free(json_str);
    }

    cJSON_Delete(root);
}

/*
 * Send text message
 */
void ws_send_text(struct descriptor_data *d, const char *category, const char *text)
{
    cJSON *root = cJSON_CreateObject();

    cJSON_AddStringToObject(root, "type", "text");
    cJSON_AddStringToObject(root, "category", category);
    cJSON_AddStringToObject(root, "data", text);

    char *json_str = cJSON_PrintUnformatted(root);
    if (json_str) {
        websocket_send_text(d, json_str);
        free(json_str);
    }

    cJSON_Delete(root);
}

/*
 * Send system message
 */
void ws_send_system(struct descriptor_data *d, const char *status, const char *message)
{
    cJSON *root = cJSON_CreateObject();
    cJSON *data = cJSON_CreateObject();

    cJSON_AddStringToObject(root, "type", "system");
    cJSON_AddStringToObject(data, "status", status);
    cJSON_AddStringToObject(data, "message", message);
    cJSON_AddItemToObject(root, "data", data);

    char *json_str = cJSON_PrintUnformatted(root);
    if (json_str) {
        websocket_send_text(d, json_str);
        free(json_str);
    }

    cJSON_Delete(root);
}

/*
 * Handle login command
 */
void ws_cmd_login(struct descriptor_data *d, cJSON *data)
{
    cJSON *account_item, *password_item;
    const char *account_name, *password;
    char tmp_name[256];
    int password_valid = 0;

    if (!data) {
        ws_send_auth_failed(d, "Missing login data");
        return;
    }

    account_item = cJSON_GetObjectItem(data, "account");
    password_item = cJSON_GetObjectItem(data, "password");

    if (!account_item || !cJSON_IsString(account_item)) {
        ws_send_auth_failed(d, "Missing account name");
        return;
    }

    if (!password_item || !cJSON_IsString(password_item)) {
        ws_send_auth_failed(d, "Missing password");
        return;
    }

    account_name = account_item->valuestring;
    password = password_item->valuestring;

    /* Validate account name */
    if (strlen(account_name) < 3 || strlen(account_name) > 20) {
        ws_send_auth_failed(d, "Invalid account name");
        return;
    }

    /* Lowercase for filesystem lookup */
    strncpy(tmp_name, account_name, sizeof(tmp_name) - 1);
    tmp_name[sizeof(tmp_name) - 1] = '\0';
    for (int i = 0; tmp_name[i]; i++) {
        tmp_name[i] = tolower(tmp_name[i]);
    }

    /* Check if account exists */
    if (!account_exists("Accounts", tmp_name)) {
        ws_send_auth_failed(d, "Account not found");
        return;
    }

    /* Allocate and load account using proper allocation function */
    if (!d->account) {
        d->account = allocate_account();
        if (!d->account) {
            ws_send_auth_failed(d, "Failed to allocate account");
            return;
        }
    }

    d->account->acct_name = str_dup(tmp_name);

    if (read_account(d->account) == -1) {
        ws_send_auth_failed(d, "Error loading account");
        d->account = free_account(d->account);
        return;
    }

    /* Verify password */
    if (is_bcrypt_hash(d->account->acct_password)) {
        password_valid = bcrypt_verify_password(password, d->account->acct_password);
    } else {
        /* Legacy MD5 hash */
        password_valid = (strcmp(CRYPT2((char *)password, d->account->acct_password),
                                 d->account->acct_password) == 0);
    }

    if (!password_valid) {
        ws_send_auth_failed(d, "Invalid password");
        /* Free the entire account on auth failure */
        d->account = free_account(d->account);
        return;
    }

    /*
     * RECONNECT CHECK:
     * Check if any character from this account is currently in-game (including linkdead).
     * If so, reconnect to that character automatically.
     */
    {
        extern struct char_data *get_char_online(char *name, bool include_linkdead);
        struct descriptor_data *k, *next_k;
        struct acct_chars *c;
        struct char_data *online_char = NULL;

        /* Search through account's characters to find one that's in-game */
        if (d->account && d->account->acct_character_list) {
            c = d->account->acct_character_list;
            while (c) {
                online_char = get_char_online(c->charname, 1); /* include linkdead */
                if (online_char) {
                    statuslog(56, "WebSocket: Found in-game character %s for account %s (linkdead=%s)",
                              GET_NAME(online_char), tmp_name,
                              online_char->desc ? "no" : "yes");
                    break;
                }
                c = c->next;
            }
        }

        /* Kick any duplicate sessions in character selection (not playing) */
        for (k = descriptor_list; k; k = next_k) {
            next_k = k->next;
            if (k == d) continue;

            if (k->websocket && k->account && k->account->acct_name &&
                strcasecmp(k->account->acct_name, tmp_name) == 0 &&
                k->connected != CON_PLAYING) {

                statuslog(56, "WebSocket: Kicking duplicate account session for %s from %s (new login from %s)",
                          tmp_name, k->host, d->host);

                ws_send_system(k, "kicked", "Another session has logged in with this account.");
                websocket_close(k, WS_CLOSE_NORMAL, "Duplicate session");
                close_socket(k);
            }
        }

        /* If we found an in-game character, reconnect to it */
        if (online_char) {
            struct descriptor_data *old_desc = online_char->desc;

            /* If there's an old descriptor, close it */
            if (old_desc && old_desc != d) {
                old_desc->character = NULL;

                if (old_desc->websocket) {
                    ws_send_system(old_desc, "kicked", "Reconnected from another session.");
                    websocket_close(old_desc, WS_CLOSE_NORMAL, "Reconnected");
                }
                close_socket(old_desc);
            }

            /* Attach character to new descriptor */
            d->character = online_char;
            online_char->desc = d;
            d->connected = CON_PLAYING;

            statuslog(56, "WebSocket: Reconnected %s to character %s from %s",
                      tmp_name, GET_NAME(d->character), d->host);

            /* Send reconnect success message */
            ws_send_reconnect_success(d, tmp_name, GET_NAME(d->character));

            /* Send current game state (room info, vitals, etc.) */
            ws_send_full_game_state(d);

            return;
        }
    }

    /* Success - show character selection */
    d->connected = CON_ACCT_SELECT_CHAR;
    statuslog(56, "WebSocket login success for account: %s from %s", tmp_name, d->host);

    ws_send_auth_success(d, tmp_name);
}

/*
 * Handle enter game command
 */
void ws_cmd_enter(struct descriptor_data *d, cJSON *data)
{
    cJSON *char_item;
    const char *char_name;
    struct acct_chars *c;
    struct descriptor_data *k, *next_k;

    if (!data) {
        ws_send_text(d, "system", "Missing character data");
        return;
    }

    char_item = cJSON_GetObjectItem(data, "character");
    if (!char_item || !cJSON_IsString(char_item)) {
        ws_send_text(d, "system", "Missing character name");
        return;
    }

    char_name = char_item->valuestring;

    if (!d->account || !d->account->acct_character_list) {
        ws_send_text(d, "system", "No characters available");
        return;
    }

    /* Find character in account list */
    c = d->account->acct_character_list;
    while (c) {
        if (strcasecmp(c->charname, char_name) == 0) {
            break;
        }
        c = c->next;
    }

    if (!c) {
        ws_send_text(d, "system", "Character not found");
        return;
    }

    /*
     * DUPLICATE SESSION CHECK:
     * If this character is already logged in, kick the old session.
     * This prevents duplicate characters from HMR, page refreshes, etc.
     */
    for (k = descriptor_list; k; k = next_k) {
        next_k = k->next;

        /* Skip self */
        if (k == d) continue;

        /* Check if this descriptor has the same character selected or playing */
        if (k->character && GET_NAME(k->character) &&
            strcasecmp(GET_NAME(k->character), char_name) == 0) {

            statuslog(56, "WebSocket: Kicking duplicate session for %s from %s (new connection from %s)",
                      char_name, k->host, d->host);

            /* Notify old client they're being disconnected */
            if (k->websocket) {
                ws_send_system(k, "kicked", "Another session has connected with this character.");
                websocket_close(k, WS_CLOSE_NORMAL, "Duplicate session");
            }

            /* Close the old descriptor */
            close_socket(k);
        }
        /* Also check if descriptor is in character selection with same char selected */
        else if (k->selected_char_name &&
                 strcasecmp(k->selected_char_name, char_name) == 0) {

            statuslog(56, "WebSocket: Kicking pending session for %s from %s (new connection from %s)",
                      char_name, k->host, d->host);

            if (k->websocket) {
                ws_send_system(k, "kicked", "Another session has connected with this character.");
                websocket_close(k, WS_CLOSE_NORMAL, "Duplicate session");
            }

            close_socket(k);
        }
    }

    /* Store selection and use nanny flow to enter game */
    if (d->selected_char_name) {
        str_free(d->selected_char_name);
    }
    d->selected_char_name = str_dup(c->charname);

    /* Queue 'y' to confirm character selection */
    write_to_q("y", &d->input, 0);
    d->connected = CON_ACCT_CONFIRM_CHAR;
}

/*
 * Handle game command
 */
void ws_cmd_game(struct descriptor_data *d, cJSON *data)
{
    const char *cmd;

    if (!data) return;

    if (cJSON_IsString(data)) {
        cmd = data->valuestring;
    } else {
        cJSON *cmd_item = cJSON_GetObjectItem(data, "command");
        if (cmd_item && cJSON_IsString(cmd_item)) {
            cmd = cmd_item->valuestring;
        } else {
            return;
        }
    }

    if (cmd && *cmd) {
        write_to_q(cmd, &d->input, 0);
    }
}

/*
 * Handle register command - create a new account
 */
void ws_cmd_register(struct descriptor_data *d, cJSON *data)
{
    cJSON *account_json, *password_json, *email_json;
    char tmp_name[MAX_INPUT_LENGTH];
    char *hash;
    int i;

    /* Get required fields from JSON */
    account_json = cJSON_GetObjectItemCaseSensitive(data, "account");
    password_json = cJSON_GetObjectItemCaseSensitive(data, "password");
    email_json = cJSON_GetObjectItemCaseSensitive(data, "email");

    if (!cJSON_IsString(account_json) || !account_json->valuestring ||
        !cJSON_IsString(password_json) || !password_json->valuestring ||
        !cJSON_IsString(email_json) || !email_json->valuestring) {
        ws_send_auth_failed(d, "Missing required fields: account, password, email");
        return;
    }

    /* Validate account name length */
    if (strlen(account_json->valuestring) < 3) {
        ws_send_auth_failed(d, "Account name must be at least 3 characters");
        return;
    }

    if (strlen(account_json->valuestring) > 14) {
        ws_send_auth_failed(d, "Account name must be 14 characters or less");
        return;
    }

    /* Copy and normalize account name */
    strncpy(tmp_name, account_json->valuestring, MAX_INPUT_LENGTH - 1);
    tmp_name[MAX_INPUT_LENGTH - 1] = '\0';

    /* Convert to lowercase except first char */
    tmp_name[0] = toupper(tmp_name[0]);
    for (i = 1; tmp_name[i]; i++) {
        tmp_name[i] = tolower(tmp_name[i]);
    }

    /* Validate account name characters */
    for (i = 0; tmp_name[i]; i++) {
        if (!isalpha(tmp_name[i]) && tmp_name[i] != '_') {
            ws_send_auth_failed(d, "Account name can only contain letters and underscores");
            return;
        }
    }

    /* Check if account already exists */
    if (account_exists("Accounts", tmp_name)) {
        ws_send_auth_failed(d, "An account with that name already exists");
        return;
    }

    /* Validate email format */
    if (!is_valid_email(email_json->valuestring)) {
        ws_send_auth_failed(d, "Invalid email address format");
        return;
    }

    /* Check if email is already in use */
    if (is_email_taken(email_json->valuestring)) {
        ws_send_auth_failed(d, "Email address is already in use");
        return;
    }

    /* Validate password length */
    if (strlen(password_json->valuestring) < 6) {
        ws_send_auth_failed(d, "Password must be at least 6 characters");
        return;
    }

    /* Allocate new account */
    if (!d->account) {
        d->account = allocate_account();
        if (!d->account) {
            ws_send_auth_failed(d, "Failed to create account - server error");
            statuslog(56, "&+RALERT&n: WebSocket could not allocate account for %s", tmp_name);
            return;
        }
    }

    /* Set account name */
    d->account->acct_name = str_dup(tmp_name);

    /* Set email */
    d->account->acct_email = str_dup(email_json->valuestring);

    /* Hash password with bcrypt */
    hash = bcrypt_hash_password(password_json->valuestring);
    if (!hash) {
        ws_send_auth_failed(d, "Failed to hash password - server error");
        d->account = free_account(d->account);
        return;
    }
    d->account->acct_password = str_dup(hash);
    free(hash);

    /* Mark account as confirmed (skip email verification for web clients) */
    d->account->acct_confirmed = 1;

    /* Save account to disk */
    if (write_account(d->account) == -1) {
        ws_send_auth_failed(d, "Failed to save account - server error");
        statuslog(56, "&+RALERT&n: WebSocket failed to write account for %s", tmp_name);
        d->account = free_account(d->account);
        return;
    }

    statuslog(56, "WebSocket: New account created: %s", tmp_name);

    /* Send success response (account with no characters) */
    ws_send_auth_success(d, "registered");
}

/*
 * Handle chargen options request (placeholder)
 */
void ws_cmd_chargen_options(struct descriptor_data *d, cJSON *data)
{
    cJSON *response, *races_array, *race_obj, *classes_array, *class_obj;
    int i, j, race_id, align_val;
    const char *align_str;
    char *json_str;

    response = cJSON_CreateObject();
    if (!response) {
        ws_send_system(d, "error", "Failed to create chargen options");
        return;
    }

    cJSON_AddStringToObject(response, "type", "chargen_options");
    races_array = cJSON_AddArrayToObject(response, "races");

    /* Loop over playable_races[] array */
    for (i = 0; playable_races[i].race_id != -1; i++)
    {
        race_id = playable_races[i].race_id;

        race_obj = cJSON_CreateObject();
        cJSON_AddNumberToObject(race_obj, "id", race_id);
        cJSON_AddStringToObject(race_obj, "name", race_names_table[race_id].normal);
        cJSON_AddStringToObject(race_obj, "ansi", race_names_table[race_id].ansi);
        cJSON_AddStringToObject(race_obj, "faction", playable_races[i].faction);

        /* Build array of available classes for this race */
        classes_array = cJSON_AddArrayToObject(race_obj, "classes");

        for (j = 1; j <= CLASS_COUNT; j++)
        {
            align_val = class_table[race_id][j];

            /* Skip if class is forbidden for this race (value 5) */
            if (align_val == 5) continue;

            align_str = ws_get_class_alignment(align_val);
            if (!align_str) continue;  /* Extra safety */

            class_obj = cJSON_CreateObject();
            cJSON_AddNumberToObject(class_obj, "id", j);
            cJSON_AddStringToObject(class_obj, "name", class_names_table[j].normal);
            cJSON_AddStringToObject(class_obj, "ansi", class_names_table[j].ansi);
            cJSON_AddStringToObject(class_obj, "alignment", align_str);

            cJSON_AddItemToArray(classes_array, class_obj);
        }

        cJSON_AddItemToArray(races_array, race_obj);
    }

    /* Send the response */
    json_str = cJSON_PrintUnformatted(response);
    if (json_str) {
        websocket_send_text(d, json_str);
        free(json_str);
    }

    cJSON_Delete(response);
}

/*
 * Handle roll stats command
 * Expects: { race: <race_id> }
 * Returns: { type: "roll_stats", base: {...}, racial: {...}, final: {...} }
 */
void ws_cmd_roll_stats(struct descriptor_data *d, cJSON *data)
{
    cJSON *response, *stats_obj;
    cJSON *race_item;
    int race_id;
    char *json_str;
    P_char temp_ch;

    /* Get race from request */
    race_item = cJSON_GetObjectItem(data, "race");
    if (!race_item || !cJSON_IsNumber(race_item)) {
        ws_send_system(d, "error", "Missing or invalid race");
        return;
    }
    race_id = race_item->valueint;

    /* Validate race is playable */
    if (!ws_is_playable_race(race_id)) {
        ws_send_system(d, "error", "Invalid race selection");
        return;
    }

    /* Create temporary character for stat rolling */
    temp_ch = (struct char_data *)malloc(sizeof(struct char_data));
    if (!temp_ch) {
        ws_send_system(d, "error", "Server error: memory allocation failed");
        return;
    }
    memset(temp_ch, 0, sizeof(struct char_data));

    /* Set race and roll stats */
    GET_RACE(temp_ch) = race_id;
    roll_basic_attributes(temp_ch, 0);  /* ROLL_NORMAL = 0 */

    /* Store rolled stats in descriptor for later use */
    d->chargen_stats = temp_ch->base_stats;
    d->chargen_race = race_id;
    d->chargen_bonus_remaining = 5;  /* 5 bonus points to allocate */

    /* Build response - send only quality labels, not numbers! */
    response = cJSON_CreateObject();
    cJSON_AddStringToObject(response, "type", "roll_stats");

    /* Stats as quality labels only */
    stats_obj = cJSON_CreateObject();
    cJSON_AddStringToObject(stats_obj, "str", stat_to_string2(temp_ch->base_stats.Str));
    cJSON_AddStringToObject(stats_obj, "dex", stat_to_string2(temp_ch->base_stats.Dex));
    cJSON_AddStringToObject(stats_obj, "agi", stat_to_string2(temp_ch->base_stats.Agi));
    cJSON_AddStringToObject(stats_obj, "con", stat_to_string2(temp_ch->base_stats.Con));
    cJSON_AddStringToObject(stats_obj, "pow", stat_to_string2(temp_ch->base_stats.Pow));
    cJSON_AddStringToObject(stats_obj, "int", stat_to_string2(temp_ch->base_stats.Int));
    cJSON_AddStringToObject(stats_obj, "wis", stat_to_string2(temp_ch->base_stats.Wis));
    cJSON_AddStringToObject(stats_obj, "cha", stat_to_string2(temp_ch->base_stats.Cha));
    cJSON_AddStringToObject(stats_obj, "luk", stat_to_string2(temp_ch->base_stats.Luk));
    cJSON_AddStringToObject(stats_obj, "kar", stat_to_string2(temp_ch->base_stats.Kar));
    cJSON_AddItemToObject(response, "stats", stats_obj);

    /* Bonus points remaining */
    cJSON_AddNumberToObject(response, "bonusRemaining", d->chargen_bonus_remaining);

    /* Cleanup temp character */
    free(temp_ch);

    /* Send response */
    json_str = cJSON_PrintUnformatted(response);
    if (json_str) {
        websocket_send_text(d, json_str);
        free(json_str);
    }

    cJSON_Delete(response);
}

/*
 * Handle add bonus command
 * Expects: { stat: "str"|"dex"|"agi"|"con"|"pow"|"int"|"wis"|"cha" }
 * Adds +5 to the specified stat (max 100), decrements bonus remaining
 * Returns updated stats with quality labels
 */
void ws_cmd_add_bonus(struct descriptor_data *d, cJSON *data)
{
    cJSON *response, *stats_obj, *stat_item;
    char *json_str;
    const char *stat_name;
    sh_int *stat_ptr = NULL;

    /* Check if we have bonus points remaining */
    if (d->chargen_bonus_remaining <= 0) {
        ws_send_system(d, "error", "No bonus points remaining");
        return;
    }

    /* Get stat to boost */
    stat_item = cJSON_GetObjectItem(data, "stat");
    if (!stat_item || !cJSON_IsString(stat_item)) {
        ws_send_system(d, "error", "Missing or invalid stat");
        return;
    }
    stat_name = stat_item->valuestring;

    /* Map stat name to pointer (9 stats can receive bonus - not kar) */
    if (strcmp(stat_name, "str") == 0) stat_ptr = &d->chargen_stats.Str;
    else if (strcmp(stat_name, "dex") == 0) stat_ptr = &d->chargen_stats.Dex;
    else if (strcmp(stat_name, "agi") == 0) stat_ptr = &d->chargen_stats.Agi;
    else if (strcmp(stat_name, "con") == 0) stat_ptr = &d->chargen_stats.Con;
    else if (strcmp(stat_name, "pow") == 0) stat_ptr = &d->chargen_stats.Pow;
    else if (strcmp(stat_name, "int") == 0) stat_ptr = &d->chargen_stats.Int;
    else if (strcmp(stat_name, "wis") == 0) stat_ptr = &d->chargen_stats.Wis;
    else if (strcmp(stat_name, "cha") == 0) stat_ptr = &d->chargen_stats.Cha;
    else if (strcmp(stat_name, "luk") == 0) stat_ptr = &d->chargen_stats.Luk;
    else {
        ws_send_system(d, "error", "Invalid stat name");
        return;
    }

    /* Check if stat is already at max */
    if (*stat_ptr >= 100) {
        ws_send_system(d, "error", "Stat is already at maximum");
        return;
    }

    /* Add bonus (+5, capped at 100) */
    *stat_ptr = BOUNDED(1, *stat_ptr + 5, 100);
    d->chargen_bonus_remaining--;

    /* Build response with updated stats */
    response = cJSON_CreateObject();
    cJSON_AddStringToObject(response, "type", "bonus_added");

    stats_obj = cJSON_CreateObject();
    cJSON_AddStringToObject(stats_obj, "str", stat_to_string2(d->chargen_stats.Str));
    cJSON_AddStringToObject(stats_obj, "dex", stat_to_string2(d->chargen_stats.Dex));
    cJSON_AddStringToObject(stats_obj, "agi", stat_to_string2(d->chargen_stats.Agi));
    cJSON_AddStringToObject(stats_obj, "con", stat_to_string2(d->chargen_stats.Con));
    cJSON_AddStringToObject(stats_obj, "pow", stat_to_string2(d->chargen_stats.Pow));
    cJSON_AddStringToObject(stats_obj, "int", stat_to_string2(d->chargen_stats.Int));
    cJSON_AddStringToObject(stats_obj, "wis", stat_to_string2(d->chargen_stats.Wis));
    cJSON_AddStringToObject(stats_obj, "cha", stat_to_string2(d->chargen_stats.Cha));
    cJSON_AddStringToObject(stats_obj, "luk", stat_to_string2(d->chargen_stats.Luk));
    cJSON_AddStringToObject(stats_obj, "kar", stat_to_string2(d->chargen_stats.Kar));
    cJSON_AddItemToObject(response, "stats", stats_obj);

    cJSON_AddNumberToObject(response, "bonusRemaining", d->chargen_bonus_remaining);
    cJSON_AddStringToObject(response, "boostedStat", stat_name);

    json_str = cJSON_PrintUnformatted(response);
    if (json_str) {
        websocket_send_text(d, json_str);
        free(json_str);
    }

    cJSON_Delete(response);
}

/*
 * Handle swap stats command
 * Expects: { stat1: "str", stat2: "int" }
 * Swaps the values of two stats
 */
void ws_cmd_swap_stats(struct descriptor_data *d, cJSON *data)
{
    cJSON *response, *stats_obj, *stat1_item, *stat2_item;
    char *json_str;
    const char *stat1_name, *stat2_name;
    sh_int *stat1_ptr = NULL, *stat2_ptr = NULL;
    sh_int temp;

    /* Get stat names */
    stat1_item = cJSON_GetObjectItem(data, "stat1");
    stat2_item = cJSON_GetObjectItem(data, "stat2");
    if (!stat1_item || !cJSON_IsString(stat1_item) ||
        !stat2_item || !cJSON_IsString(stat2_item)) {
        ws_send_system(d, "error", "Missing stat names for swap");
        return;
    }
    stat1_name = stat1_item->valuestring;
    stat2_name = stat2_item->valuestring;

    /* Can't swap same stat */
    if (strcmp(stat1_name, stat2_name) == 0) {
        ws_send_system(d, "error", "Cannot swap a stat with itself");
        return;
    }

    /* Map stat1 name to pointer (9 stats swappable - not kar) */
    if (strcmp(stat1_name, "str") == 0) stat1_ptr = &d->chargen_stats.Str;
    else if (strcmp(stat1_name, "dex") == 0) stat1_ptr = &d->chargen_stats.Dex;
    else if (strcmp(stat1_name, "agi") == 0) stat1_ptr = &d->chargen_stats.Agi;
    else if (strcmp(stat1_name, "con") == 0) stat1_ptr = &d->chargen_stats.Con;
    else if (strcmp(stat1_name, "pow") == 0) stat1_ptr = &d->chargen_stats.Pow;
    else if (strcmp(stat1_name, "int") == 0) stat1_ptr = &d->chargen_stats.Int;
    else if (strcmp(stat1_name, "wis") == 0) stat1_ptr = &d->chargen_stats.Wis;
    else if (strcmp(stat1_name, "cha") == 0) stat1_ptr = &d->chargen_stats.Cha;
    else if (strcmp(stat1_name, "luk") == 0) stat1_ptr = &d->chargen_stats.Luk;
    else {
        ws_send_system(d, "error", "Invalid first stat name");
        return;
    }

    /* Map stat2 name to pointer (9 stats swappable - not kar) */
    if (strcmp(stat2_name, "str") == 0) stat2_ptr = &d->chargen_stats.Str;
    else if (strcmp(stat2_name, "dex") == 0) stat2_ptr = &d->chargen_stats.Dex;
    else if (strcmp(stat2_name, "agi") == 0) stat2_ptr = &d->chargen_stats.Agi;
    else if (strcmp(stat2_name, "con") == 0) stat2_ptr = &d->chargen_stats.Con;
    else if (strcmp(stat2_name, "pow") == 0) stat2_ptr = &d->chargen_stats.Pow;
    else if (strcmp(stat2_name, "int") == 0) stat2_ptr = &d->chargen_stats.Int;
    else if (strcmp(stat2_name, "wis") == 0) stat2_ptr = &d->chargen_stats.Wis;
    else if (strcmp(stat2_name, "cha") == 0) stat2_ptr = &d->chargen_stats.Cha;
    else if (strcmp(stat2_name, "luk") == 0) stat2_ptr = &d->chargen_stats.Luk;
    else {
        ws_send_system(d, "error", "Invalid second stat name");
        return;
    }

    /* Perform the swap */
    temp = *stat1_ptr;
    *stat1_ptr = *stat2_ptr;
    *stat2_ptr = temp;

    /* Build response with updated stats */
    response = cJSON_CreateObject();
    cJSON_AddStringToObject(response, "type", "stats_swapped");

    stats_obj = cJSON_CreateObject();
    cJSON_AddStringToObject(stats_obj, "str", stat_to_string2(d->chargen_stats.Str));
    cJSON_AddStringToObject(stats_obj, "dex", stat_to_string2(d->chargen_stats.Dex));
    cJSON_AddStringToObject(stats_obj, "agi", stat_to_string2(d->chargen_stats.Agi));
    cJSON_AddStringToObject(stats_obj, "con", stat_to_string2(d->chargen_stats.Con));
    cJSON_AddStringToObject(stats_obj, "pow", stat_to_string2(d->chargen_stats.Pow));
    cJSON_AddStringToObject(stats_obj, "int", stat_to_string2(d->chargen_stats.Int));
    cJSON_AddStringToObject(stats_obj, "wis", stat_to_string2(d->chargen_stats.Wis));
    cJSON_AddStringToObject(stats_obj, "cha", stat_to_string2(d->chargen_stats.Cha));
    cJSON_AddStringToObject(stats_obj, "luk", stat_to_string2(d->chargen_stats.Luk));
    cJSON_AddStringToObject(stats_obj, "kar", stat_to_string2(d->chargen_stats.Kar));
    cJSON_AddItemToObject(response, "stats", stats_obj);

    cJSON_AddStringToObject(response, "swapped1", stat1_name);
    cJSON_AddStringToObject(response, "swapped2", stat2_name);

    json_str = cJSON_PrintUnformatted(response);
    if (json_str) {
        websocket_send_text(d, json_str);
        free(json_str);
    }

    cJSON_Delete(response);
}

/*
 * Handle create character command
 * Expects: { name, race, class, sex, alignment (for neutral races), stats }
 * This validates input and prepares character creation
 * Full character creation requires proper account integration
 */
void ws_cmd_create_character(struct descriptor_data *d, cJSON *data)
{
    cJSON *name_item, *race_item, *class_item, *sex_item, *align_item;
    cJSON *hometown_item, *hardcore_item, *newbie_item;
    cJSON *response;
    char *json_str;
    const char *name;
    int race_id, class_id, sex, alignment;
    int hometown_id, is_hardcore, is_newbie;
    int class_align_req;
    const char *faction;

    /* Validate required fields */
    name_item = cJSON_GetObjectItem(data, "name");
    race_item = cJSON_GetObjectItem(data, "race");
    class_item = cJSON_GetObjectItem(data, "class");
    sex_item = cJSON_GetObjectItem(data, "sex");
    hometown_item = cJSON_GetObjectItem(data, "hometown");
    hardcore_item = cJSON_GetObjectItem(data, "hardcore");
    newbie_item = cJSON_GetObjectItem(data, "newbie");

    if (!name_item || !cJSON_IsString(name_item)) {
        ws_send_system(d, "error", "Missing or invalid character name");
        return;
    }
    if (!race_item || !cJSON_IsNumber(race_item)) {
        ws_send_system(d, "error", "Missing or invalid race");
        return;
    }
    if (!class_item || !cJSON_IsNumber(class_item)) {
        ws_send_system(d, "error", "Missing or invalid class");
        return;
    }
    if (!sex_item || !cJSON_IsNumber(sex_item)) {
        ws_send_system(d, "error", "Missing or invalid sex");
        return;
    }

    name = name_item->valuestring;
    race_id = race_item->valueint;
    class_id = class_item->valueint;
    sex = sex_item->valueint;

    /* Parse optional new fields */
    hometown_id = hometown_item && cJSON_IsNumber(hometown_item) ? hometown_item->valueint : -1;
    is_hardcore = hardcore_item && cJSON_IsBool(hardcore_item) ? cJSON_IsTrue(hardcore_item) : 0;
    is_newbie = newbie_item && cJSON_IsBool(newbie_item) ? cJSON_IsTrue(newbie_item) : 1;

    /* Veterans only can be hardcore */
    if (is_newbie && is_hardcore) {
        is_hardcore = 0;  /* Newbies cannot be hardcore */
    }

    /* Validate name length */
    if (strlen(name) < 2 || strlen(name) > 12) {
        ws_send_system(d, "error", "Name must be 2-12 characters");
        return;
    }

    /* Validate race is playable */
    if (!ws_is_playable_race(race_id)) {
        ws_send_system(d, "error", "Invalid race selection");
        return;
    }

    /* Validate class is valid for race */
    if (class_id < 1 || class_id > CLASS_COUNT) {
        ws_send_system(d, "error", "Invalid class selection");
        return;
    }

    class_align_req = class_table[race_id][class_id];
    if (class_align_req == 5) {
        ws_send_system(d, "error", "That class is not available for your race");
        return;
    }

    /* Validate sex */
    if (sex < 1 || sex > 2) {  /* 1 = male, 2 = female */
        ws_send_system(d, "error", "Invalid sex selection");
        return;
    }

    /* Check alignment for neutral races */
    faction = ws_get_race_faction(race_id);
    if (strcmp(faction, "neutral") == 0) {
        align_item = cJSON_GetObjectItem(data, "alignment");
        if (!align_item || !cJSON_IsString(align_item)) {
            ws_send_system(d, "error", "Neutral races must choose an alignment");
            return;
        }
        if (strcmp(align_item->valuestring, "good") == 0) {
            alignment = 1;  /* good */
        } else if (strcmp(align_item->valuestring, "evil") == 0) {
            alignment = -1;  /* evil */
        } else {
            ws_send_system(d, "error", "Invalid alignment selection");
            return;
        }
    } else if (strcmp(faction, "good") == 0) {
        alignment = 1;
    } else {
        alignment = -1;
    }

    /* Validate class alignment requirement */
    if (class_align_req == 1 && alignment != 1) {  /* good only */
        ws_send_system(d, "error", "That class requires good alignment");
        return;
    }
    if (class_align_req == -1 && alignment != -1) {  /* evil only */
        ws_send_system(d, "error", "That class requires evil alignment");
        return;
    }

    /* Store chargen options in descriptor for later use */
    d->chargen_hometown = hometown_id;
    d->chargen_hardcore = is_hardcore;
    d->chargen_newbie = is_newbie;

    /*
     * ACTUAL CHARACTER CREATION
     */

    /* External function declarations */
    extern bool pfile_exists(const char *dir, char *name);
    extern void clear_char(P_char ch);
    extern void setCharPhysTypeInfo(P_char ch);
    extern void init_char(P_char ch);
    extern void add_char_to_account(P_desc d);
    extern int writeCharacter(P_char ch, int type, int room);
    extern void enter_game(P_desc d);
    extern int find_hometown(int race, bool force);

    char capitalized_name[MAX_NAME_LENGTH + 1];
    int i, actual_hometown;
    P_char ch;

    /* Capitalize name */
    strncpy(capitalized_name, name, MAX_NAME_LENGTH);
    capitalized_name[MAX_NAME_LENGTH] = '\0';
    capitalized_name[0] = toupper(capitalized_name[0]);
    for (i = 1; capitalized_name[i]; i++) {
        capitalized_name[i] = tolower(capitalized_name[i]);
    }

    /* Check if name already exists */
    if (pfile_exists(SAVE_DIR, capitalized_name)) {
        cJSON *err = cJSON_CreateObject();
        cJSON_AddStringToObject(err, "type", "create_character");
        cJSON_AddStringToObject(err, "status", "error");
        cJSON_AddStringToObject(err, "message", "That name is already in use");
        char *err_str = cJSON_PrintUnformatted(err);
        if (err_str) {
            websocket_send_text(d, err_str);
            free(err_str);
        }
        cJSON_Delete(err);
        return;
    }
    if (pfile_exists(BADNAME_DIR, capitalized_name)) {
        cJSON *err = cJSON_CreateObject();
        cJSON_AddStringToObject(err, "type", "create_character");
        cJSON_AddStringToObject(err, "status", "error");
        cJSON_AddStringToObject(err, "message", "That name has been declined");
        char *err_str = cJSON_PrintUnformatted(err);
        if (err_str) {
            websocket_send_text(d, err_str);
            free(err_str);
        }
        cJSON_Delete(err);
        return;
    }

    /* Allocate character structure if not exists */
    if (!d->character) {
        d->character = (struct char_data *) mm_get(dead_mob_pool);
        clear_char(d->character);
        ensure_pconly_pool();
        d->character->only.pc = (struct pc_only_data *) mm_get(dead_pconly_pool);
        d->character->only.pc->aggressive = -1;
        d->character->desc = d;
        setCharPhysTypeInfo(d->character);
    }

    ch = d->character;

    /* Set character name */
    if (ch->player.name) {
        str_free(ch->player.name);
    }
    ch->player.name = str_dup(capitalized_name);

    /* Set race */
    GET_RACE(ch) = race_id;

    /* Set sex */
    ch->player.sex = sex;  /* 1 = male, 2 = female */

    /* Set class - convert class_id to bitmask */
    ch->player.m_class = 1 << (class_id - 1);

    /* Set alignment (1000 for good, -1000 for evil) */
    GET_ALIGNMENT(ch) = (alignment == 1) ? 1000 : -1000;

    /* Set racewar based on race and alignment */
    if (OLD_RACE_GOOD(race_id, GET_ALIGNMENT(ch))) {
        GET_RACEWAR(ch) = RACEWAR_GOOD;
    } else if (OLD_RACE_EVIL(race_id, GET_ALIGNMENT(ch))) {
        GET_RACEWAR(ch) = RACEWAR_EVIL;
    } else if (OLD_RACE_PUNDEAD(race_id)) {
        GET_RACEWAR(ch) = RACEWAR_UNDEAD;
    } else if (IS_HARPY(ch)) {
        GET_RACEWAR(ch) = RACEWAR_NEUTRAL;
    }

    /* Set hometown */
    if (hometown_id < 0 || hometown_id > LAST_HOME) {
        actual_hometown = find_hometown(race_id, false);
        if (actual_hometown == HOME_CHOICE) {
            /* Race has multiple choices - pick first available */
            for (i = 0; i <= LAST_HOME; i++) {
                if (avail_hometowns[i][race_id] == 1) {
                    actual_hometown = i;
                    break;
                }
            }
        }
    } else {
        actual_hometown = hometown_id;
    }
    GET_HOME(ch) = actual_hometown;
    GET_BIRTHPLACE(ch) = actual_hometown;
    GET_ORIG_BIRTHPLACE(ch) = actual_hometown;

    /* Copy stats from descriptor (already rolled with bonuses/swaps applied) */
    ch->base_stats = d->chargen_stats;
    ch->curr_stats = d->chargen_stats;

    /* Level stays at 0 (from clear_char's bzero) - matching telnet behavior.
       Level 0 causes enter_game() to skip item restoration block. */

    /* Set hardcore/newbie flags */
    if (is_newbie) {
        SET_BIT(ch->specials.act2, PLR2_NEWBIE);
    }
    if (is_hardcore) {
        SET_BIT(ch->specials.act2, PLR2_HARDCORE_CHAR);
    }

    /* Initialize character (sets PID, skills, HP/mana/vitality, etc.) */
    init_char(ch);

    /* Copy account password to character */
    strncpy(ch->only.pc->pwd, d->account->acct_password, sizeof(ch->only.pc->pwd) - 1);
    ch->only.pc->pwd[sizeof(ch->only.pc->pwd) - 1] = '\0';

    /* Add character to account */
#ifdef USE_ACCOUNT
    add_char_to_account(d);
#endif

    /* d->rtype stays unset (0) - matching telnet behavior for new characters.
       rtype is only set by restoreCharOnly() when loading an existing character. */

    /* Note: EQ_WIPE check is inside if(GET_LEVEL(ch)) block in enter_game(),
       so with level 0 it's never reached - no hack needed. */

    /* Save character to disk */
    writeCharacter(ch, RENT_QUIT, NOWHERE);

    /* Log new player creation */
    logit(LOG_NEW, "%s [%s] new WebSocket player.", GET_NAME(ch), d->host);
    statuslog(ch->player.level, "%s [%s] new WebSocket player.", GET_NAME(ch), d->host);

    /* Set connection state BEFORE entering game (required for prompts) */
    STATE(d) = CON_PLAYING;

    /* Enter the game */
    enter_game(d);

    /* Enable prompt display */
    d->prompt_mode = TRUE;

    /* Send full game state via GMCP */
    ws_send_full_game_state(d);

    /* Send success response */
    response = cJSON_CreateObject();
    cJSON_AddStringToObject(response, "type", "create_character");
    cJSON_AddStringToObject(response, "status", "created");
    cJSON_AddStringToObject(response, "message", "Character created successfully!");
    cJSON_AddStringToObject(response, "name", capitalized_name);
    cJSON_AddStringToObject(response, "race", race_names_table[race_id].normal);
    cJSON_AddStringToObject(response, "class", class_names_table[class_id].normal);
    cJSON_AddStringToObject(response, "faction", (alignment == 1) ? "good" : "evil");
    cJSON_AddBoolToObject(response, "hardcore", is_hardcore ? cJSON_True : cJSON_False);
    cJSON_AddBoolToObject(response, "newbie", is_newbie ? cJSON_True : cJSON_False);
    if (actual_hometown >= 0 && actual_hometown <= LAST_HOME) {
        cJSON_AddStringToObject(response, "hometown", town_name_list[actual_hometown]);
    }

    json_str = cJSON_PrintUnformatted(response);
    if (json_str) {
        websocket_send_text(d, json_str);
        free(json_str);
    }

    cJSON_Delete(response);
}

/*
 * Validate character name - check if already taken
 */
void ws_cmd_validate_name(struct descriptor_data *d, cJSON *data)
{
    extern bool pfile_exists(const char *dir, char *name);
    cJSON *name_item, *response;
    char *json_str;
    const char *name;
    char capitalized_name[MAX_NAME_LENGTH + 1];
    int i;

    name_item = cJSON_GetObjectItem(data, "name");
    if (!name_item || !cJSON_IsString(name_item)) {
        ws_send_system(d, "error", "Missing character name");
        return;
    }

    name = name_item->valuestring;

    /* Validate name length */
    if (strlen(name) < 2) {
        response = cJSON_CreateObject();
        cJSON_AddStringToObject(response, "type", "validate_name");
        cJSON_AddBoolToObject(response, "valid", 0);
        cJSON_AddStringToObject(response, "message", "Name must be at least 2 characters");
        goto send_response;
    }
    if (strlen(name) > 12) {
        response = cJSON_CreateObject();
        cJSON_AddStringToObject(response, "type", "validate_name");
        cJSON_AddBoolToObject(response, "valid", 0);
        cJSON_AddStringToObject(response, "message", "Name must be at most 12 characters");
        goto send_response;
    }

    /* Validate name contains only letters */
    for (i = 0; name[i]; i++) {
        if (!isalpha(name[i])) {
            response = cJSON_CreateObject();
            cJSON_AddStringToObject(response, "type", "validate_name");
            cJSON_AddBoolToObject(response, "valid", 0);
            cJSON_AddStringToObject(response, "message", "Name can only contain letters");
            goto send_response;
        }
    }

    /* Capitalize name for pfile_exists (it lowercases internally) */
    strncpy(capitalized_name, name, MAX_NAME_LENGTH);
    capitalized_name[MAX_NAME_LENGTH] = '\0';
    capitalized_name[0] = toupper(capitalized_name[0]);
    for (i = 1; capitalized_name[i]; i++) {
        capitalized_name[i] = tolower(capitalized_name[i]);
    }

    statuslog(56, "WS validate_name: checking '%s' in SAVE_DIR='%s'", capitalized_name, SAVE_DIR);

    /* Check if player file exists - use same function as create_character */
    if (pfile_exists(SAVE_DIR, capitalized_name)) {
        statuslog(56, "WS validate_name: '%s' EXISTS - returning invalid", capitalized_name);
        response = cJSON_CreateObject();
        cJSON_AddStringToObject(response, "type", "validate_name");
        cJSON_AddBoolToObject(response, "valid", 0);
        cJSON_AddStringToObject(response, "message", "Name is already in use");
        goto send_response;
    }

    /* Check badname directory - use correct BADNAME_DIR constant */
    if (pfile_exists(BADNAME_DIR, capitalized_name)) {
        response = cJSON_CreateObject();
        cJSON_AddStringToObject(response, "type", "validate_name");
        cJSON_AddBoolToObject(response, "valid", 0);
        cJSON_AddStringToObject(response, "message", "That name is not allowed");
        goto send_response;
    }

    /* Name is valid and available */
    statuslog(56, "WS validate_name: '%s' is available", capitalized_name);
    response = cJSON_CreateObject();
    cJSON_AddStringToObject(response, "type", "validate_name");
    cJSON_AddBoolToObject(response, "valid", 1);
    cJSON_AddStringToObject(response, "message", "Name is available");

send_response:
    json_str = cJSON_PrintUnformatted(response);
    if (json_str) {
        websocket_send_text(d, json_str);
        free(json_str);
    }
    cJSON_Delete(response);
}

/*
 * Get available hometowns for a race
 */
void ws_cmd_get_hometowns(struct descriptor_data *d, cJSON *data)
{
    cJSON *race_item, *response, *options, *option;
    char *json_str;
    int race_id, i, count = 0;

    race_item = cJSON_GetObjectItem(data, "race");
    if (!race_item || !cJSON_IsNumber(race_item)) {
        ws_send_system(d, "error", "Missing race ID");
        return;
    }

    race_id = race_item->valueint;

    if (race_id < 1 || race_id > LAST_RACE) {
        ws_send_system(d, "error", "Invalid race ID");
        return;
    }

    response = cJSON_CreateObject();
    cJSON_AddStringToObject(response, "type", "hometowns");
    cJSON_AddNumberToObject(response, "race", race_id);

    options = cJSON_CreateArray();

    /* Find available hometowns for this race */
    for (i = 0; i <= LAST_HOME; i++) {
        if (avail_hometowns[i][race_id] == 1) {
            option = cJSON_CreateObject();
            cJSON_AddNumberToObject(option, "id", i);
            cJSON_AddStringToObject(option, "name", town_name_list[i]);
            cJSON_AddItemToArray(options, option);
            count++;
        }
    }

    cJSON_AddItemToObject(response, "options", options);
    cJSON_AddNumberToObject(response, "count", count);
    /* If count == 1, frontend can skip the selection step */
    cJSON_AddBoolToObject(response, "hasChoice", count > 1 ? cJSON_True : cJSON_False);

    json_str = cJSON_PrintUnformatted(response);
    if (json_str) {
        websocket_send_text(d, json_str);
        free(json_str);
    }
    cJSON_Delete(response);
}

/*
 * ============================================================================
 * Account Menu WebSocket Handlers
 * ============================================================================
 */

/*
 * Send account message (generic helper)
 */
static void ws_send_account_message(struct descriptor_data *d, const char *action,
                                     cJSON *data_obj, const char *error)
{
    cJSON *root = cJSON_CreateObject();

    cJSON_AddStringToObject(root, "type", "account");
    cJSON_AddStringToObject(root, "action", action);

    if (data_obj) {
        cJSON_AddItemToObject(root, "data", data_obj);
    }
    if (error) {
        cJSON_AddStringToObject(root, "error", error);
    }

    char *json_str = cJSON_PrintUnformatted(root);
    if (json_str) {
        websocket_send_text(d, json_str);
        free(json_str);
    }

    cJSON_Delete(root);
}

/*
 * Build character list JSON array (reusable helper)
 */
static cJSON *ws_build_character_list(struct descriptor_data *d)
{
    cJSON *characters = cJSON_CreateArray();
    struct acct_chars *c;

    if (d->account && d->account->acct_character_list) {
        c = d->account->acct_character_list;
        while (c) {
            struct ws_char_info info;
            if (ws_load_char_info(c->charname, &info)) {
                cJSON *char_obj = cJSON_CreateObject();
                cJSON_AddStringToObject(char_obj, "name", info.name);
                cJSON_AddNumberToObject(char_obj, "level", info.level);
                cJSON_AddStringToObject(char_obj, "race", ws_get_race_name(info.race));
                cJSON_AddStringToObject(char_obj, "class", info.class_str);

                /* Last room name */
                if (info.hometown >= 0 && info.hometown < top_of_world && world[info.hometown].name) {
                    cJSON_AddStringToObject(char_obj, "lastRoom", world[info.hometown].name);
                } else {
                    cJSON_AddStringToObject(char_obj, "lastRoom", "Unknown");
                }

                cJSON_AddItemToArray(characters, char_obj);
            }
            c = c->next;
        }
    }

    return characters;
}

/*
 * Get extended account information
 * Client: {"type":"cmd","cmd":"account_info"}
 * Server: {"type":"account","action":"info","data":{...}}
 *
 * Optimized: Single pass through characters to collect playtime, immortal level,
 * and character list data (previously loaded each character 2-3 times)
 */
void ws_cmd_account_info(struct descriptor_data *d, cJSON *data)
{
    extern char *get_class_string(P_char ch, char *strn);
    cJSON *info_data, *characters, *char_obj;
    char time_buf[64];
    char class_str[256];
    char name_cap[32];
    struct acct_chars *c;
    P_char temp_ch;
    long total_playtime = 0;
    int immortal_level = 0;

    if (!d->account) {
        ws_send_account_message(d, "error", NULL, "Not logged in");
        return;
    }

    info_data = cJSON_CreateObject();

    /* Account name */
    cJSON_AddStringToObject(info_data, "name", d->account->acct_name);

    /* Email */
    cJSON_AddStringToObject(info_data, "email",
        d->account->acct_email ? d->account->acct_email : "");

    /* Created date - we don't store this, use first character's creation or "unknown" */
    cJSON_AddStringToObject(info_data, "created", "unknown");

    /* Last login */
    if (d->account->acct_last > 0) {
        strftime(time_buf, sizeof(time_buf), "%Y-%m-%d %H:%M:%S",
                 localtime(&d->account->acct_last));
        cJSON_AddStringToObject(info_data, "lastLogin", time_buf);
    } else {
        cJSON_AddStringToObject(info_data, "lastLogin", "never");
    }

    /* Single pass: collect playtime, immortal level, and character list */
    characters = cJSON_CreateArray();
    c = d->account->acct_character_list;
    while (c) {
        temp_ch = (struct char_data *)malloc(sizeof(struct char_data));
        if (temp_ch) {
            memset(temp_ch, 0, sizeof(struct char_data));
            temp_ch->only.pc = (struct pc_only_data *)malloc(sizeof(struct pc_only_data));
            if (temp_ch->only.pc) {
                memset(temp_ch->only.pc, 0, sizeof(struct pc_only_data));
                if (restoreCharOnly(temp_ch, c->charname) >= 0) {
                    int level = GET_LEVEL(temp_ch);

                    /* Accumulate playtime */
                    total_playtime += temp_ch->player.time.played;

                    /* Track highest immortal level */
                    if (level >= 57 && level > immortal_level) {
                        immortal_level = level;
                    }

                    /* Build character JSON object */
                    strncpy(name_cap, GET_NAME(temp_ch), 31);
                    name_cap[31] = '\0';
                    if (name_cap[0]) name_cap[0] = toupper(name_cap[0]);

                    get_class_string(temp_ch, class_str);

                    char_obj = cJSON_CreateObject();
                    cJSON_AddStringToObject(char_obj, "name", name_cap);
                    cJSON_AddNumberToObject(char_obj, "level", level);
                    cJSON_AddStringToObject(char_obj, "race", ws_get_race_name(GET_RACE(temp_ch)));
                    cJSON_AddStringToObject(char_obj, "class", class_str);

                    /* Last room name */
                    int hometown = GET_HOME(temp_ch);
                    if (hometown >= 0 && hometown < top_of_world && world[hometown].name) {
                        cJSON_AddStringToObject(char_obj, "lastRoom", world[hometown].name);
                    } else {
                        cJSON_AddStringToObject(char_obj, "lastRoom", "Unknown");
                    }

                    cJSON_AddItemToArray(characters, char_obj);
                }
                free(temp_ch->only.pc);
            }
            free(temp_ch);
        }
        c = c->next;
    }

    cJSON_AddNumberToObject(info_data, "totalPlaytime", total_playtime);
    cJSON_AddNumberToObject(info_data, "immortalLevel", immortal_level);
    cJSON_AddItemToObject(info_data, "characters", characters);

    ws_send_account_message(d, "info", info_data, NULL);
}

/*
 * Change account email
 * Client: {"type":"cmd","cmd":"change_email","data":{"newEmail":"..."}}
 * Server: {"type":"account","action":"email_changed","data":{"email":"..."}}
 */
void ws_cmd_change_email(struct descriptor_data *d, cJSON *data)
{
    cJSON *new_email_json;
    const char *new_email;
    cJSON *result_data;

    if (!d->account) {
        ws_send_account_message(d, "error", NULL, "Not logged in");
        return;
    }

    if (!data) {
        ws_send_account_message(d, "error", NULL, "Missing data");
        return;
    }

    new_email_json = cJSON_GetObjectItem(data, "newEmail");
    if (!new_email_json || !cJSON_IsString(new_email_json)) {
        ws_send_account_message(d, "error", NULL, "Missing newEmail field");
        return;
    }

    new_email = new_email_json->valuestring;

    /* Validate email format */
    if (!is_valid_email(new_email)) {
        ws_send_account_message(d, "error", NULL, "Invalid email format");
        return;
    }

    /* Check if email is already taken (by another account) */
    if (is_email_taken(new_email)) {
        /* But allow keeping the same email */
        if (!d->account->acct_email || strcasecmp(new_email, d->account->acct_email) != 0) {
            ws_send_account_message(d, "error", NULL, "Email already in use");
            return;
        }
    }

    /* Update email */
    if (d->account->acct_email) {
        FREE(d->account->acct_email);
    }
    d->account->acct_email = str_dup(new_email);
    write_account(d->account);

    statuslog(56, "Account %s changed email to %s", d->account->acct_name, new_email);

    result_data = cJSON_CreateObject();
    cJSON_AddStringToObject(result_data, "email", new_email);
    ws_send_account_message(d, "email_changed", result_data, NULL);
}

/*
 * Change account password
 * Client: {"type":"cmd","cmd":"change_password","data":{"currentPassword":"...","newPassword":"..."}}
 * Server: {"type":"account","action":"password_changed"}
 */
void ws_cmd_change_password(struct descriptor_data *d, cJSON *data)
{
    cJSON *current_json, *new_json;
    const char *current_password, *new_password;
    char *hash;

    if (!d->account) {
        ws_send_account_message(d, "error", NULL, "Not logged in");
        return;
    }

    if (!data) {
        ws_send_account_message(d, "error", NULL, "Missing data");
        return;
    }

    current_json = cJSON_GetObjectItem(data, "currentPassword");
    new_json = cJSON_GetObjectItem(data, "newPassword");

    if (!current_json || !cJSON_IsString(current_json) ||
        !new_json || !cJSON_IsString(new_json)) {
        ws_send_account_message(d, "error", NULL, "Missing password fields");
        return;
    }

    current_password = current_json->valuestring;
    new_password = new_json->valuestring;

    /* Verify current password */
    if (!bcrypt_verify_password(current_password, d->account->acct_password)) {
        ws_send_account_message(d, "error", NULL, "Current password incorrect");
        return;
    }

    /* Validate new password (at least 6 characters) */
    if (strlen(new_password) < 6) {
        ws_send_account_message(d, "error", NULL, "Password must be at least 6 characters");
        return;
    }

    /* Hash new password */
    hash = bcrypt_hash_password(new_password);
    if (!hash) {
        ws_send_account_message(d, "error", NULL, "Failed to hash password");
        return;
    }

    /* Update password */
    if (d->account->acct_password) {
        FREE(d->account->acct_password);
    }
    d->account->acct_password = str_dup(hash);
    free(hash);
    write_account(d->account);

    statuslog(56, "Account %s changed password", d->account->acct_name);

    ws_send_account_message(d, "password_changed", NULL, NULL);
}

/*
 * Delete a character
 * Client: {"type":"cmd","cmd":"delete_character","data":{"name":"...","confirm":true}}
 * Server: {"type":"account","action":"character_deleted","data":{"name":"...","characters":[...]}}
 */
void ws_cmd_delete_character(struct descriptor_data *d, cJSON *data)
{
    cJSON *name_json, *confirm_json;
    const char *char_name;
    struct acct_chars *c, *prev;
    P_char ch;
    cJSON *result_data;

    if (!d->account) {
        ws_send_account_message(d, "error", NULL, "Not logged in");
        return;
    }

    if (!data) {
        ws_send_account_message(d, "error", NULL, "Missing data");
        return;
    }

    name_json = cJSON_GetObjectItem(data, "name");
    confirm_json = cJSON_GetObjectItem(data, "confirm");

    if (!name_json || !cJSON_IsString(name_json)) {
        ws_send_account_message(d, "error", NULL, "Missing character name");
        return;
    }

    if (!confirm_json || !cJSON_IsTrue(confirm_json)) {
        ws_send_account_message(d, "error", NULL, "Deletion not confirmed");
        return;
    }

    char_name = name_json->valuestring;

    /* Find character in account list */
    c = d->account->acct_character_list;
    prev = NULL;
    while (c) {
        if (strcasecmp(c->charname, char_name) == 0) {
            break;
        }
        prev = c;
        c = c->next;
    }

    if (!c) {
        ws_send_account_message(d, "error", NULL, "Character not found");
        return;
    }

    /* Load character for deletion */
    ch = (struct char_data *)malloc(sizeof(struct char_data));
    if (!ch) {
        ws_send_account_message(d, "error", NULL, "Failed to load character");
        return;
    }

    memset(ch, 0, sizeof(struct char_data));
    ch->only.pc = (struct pc_only_data *)malloc(sizeof(struct pc_only_data));
    if (!ch->only.pc) {
        free(ch);
        ws_send_account_message(d, "error", NULL, "Failed to load character");
        return;
    }

    memset(ch->only.pc, 0, sizeof(struct pc_only_data));

    if (restoreCharOnly(ch, (char *)char_name) < 0) {
        free(ch->only.pc);
        free(ch);
        ws_send_account_message(d, "error", NULL, "Failed to load character file");
        return;
    }

    /* Log the deletion */
    statuslog(ch->player.level, "%s deleted %s via web client (%s@%s).",
              d->account->acct_name, char_name, d->login, d->host);
    logit(LOG_PLAYER, "%s deleted %s via web client (%s@%s).",
          d->account->acct_name, char_name, d->login, d->host);

    /* Delete character file */
    deleteCharacter(ch);

    /* Free temp character */
    free(ch->only.pc);
    free(ch);

    /* Remove from account character list */
    if (prev) {
        prev->next = c->next;
    } else {
        d->account->acct_character_list = c->next;
    }
    FREE(c->charname);
    FREE(c);

    /* Save account */
    write_account(d->account);

    /* Send success with updated character list */
    result_data = cJSON_CreateObject();
    cJSON_AddStringToObject(result_data, "name", char_name);
    cJSON_AddItemToObject(result_data, "characters", ws_build_character_list(d));
    ws_send_account_message(d, "character_deleted", result_data, NULL);
}

/*
 * Get rested bonus status for all characters
 * Client: {"type":"cmd","cmd":"rested_bonus"}
 * Server: {"type":"account","action":"rested_bonus","data":{"characters":[...]}}
 */
void ws_cmd_rested_bonus(struct descriptor_data *d, cJSON *data)
{
    cJSON *result_data, *characters, *char_obj;
    struct acct_chars *c;
    P_char temp_ch;
    time_t current_time;

    if (!d->account) {
        ws_send_account_message(d, "error", NULL, "Not logged in");
        return;
    }

    result_data = cJSON_CreateObject();
    characters = cJSON_CreateArray();
    current_time = time(0);

    c = d->account->acct_character_list;
    while (c) {
        temp_ch = (struct char_data *)malloc(sizeof(struct char_data));
        if (temp_ch) {
            memset(temp_ch, 0, sizeof(struct char_data));
            temp_ch->only.pc = (struct pc_only_data *)malloc(sizeof(struct pc_only_data));
            if (temp_ch->only.pc) {
                memset(temp_ch->only.pc, 0, sizeof(struct pc_only_data));
                if (restoreCharOnly(temp_ch, c->charname) >= 0) {
                    time_t offline_seconds = current_time - temp_ch->player.time.saved;
                    int offline_hours = offline_seconds / 3600;
                    int max_hours = 20;  /* Well-rested threshold */
                    int percent = (offline_hours * 100) / max_hours;
                    if (percent > 100) percent = 100;

                    /* Capitalize name */
                    char name_cap[32];
                    strncpy(name_cap, GET_NAME(temp_ch), 31);
                    name_cap[31] = '\0';
                    if (name_cap[0]) name_cap[0] = toupper(name_cap[0]);

                    char_obj = cJSON_CreateObject();
                    cJSON_AddStringToObject(char_obj, "name", name_cap);
                    cJSON_AddNumberToObject(char_obj, "restedPercent", percent);
                    cJSON_AddNumberToObject(char_obj, "restedHours", offline_hours > max_hours ? max_hours : offline_hours);
                    cJSON_AddNumberToObject(char_obj, "maxHours", max_hours);
                    cJSON_AddItemToArray(characters, char_obj);
                }
                free(temp_ch->only.pc);
            }
            free(temp_ch);
        }
        c = c->next;
    }

    cJSON_AddItemToObject(result_data, "characters", characters);
    ws_send_account_message(d, "rested_bonus", result_data, NULL);
}

/*
 * Logout from account (disconnect)
 * Client: {"type":"cmd","cmd":"logout"}
 * Server: {"type":"account","action":"logged_out"} then close connection
 */
void ws_cmd_logout(struct descriptor_data *d, cJSON *data)
{
    if (!d->account) {
        ws_send_account_message(d, "error", NULL, "Not logged in");
        return;
    }

    statuslog(56, "Account %s logged out via web client", d->account->acct_name);

    ws_send_account_message(d, "logged_out", NULL, NULL);

    /* Close the connection */
    STATE(d) = CON_EXIT;
}

/*
 * Send return to account menu signal
 * Called when player rents, dies, quits, or suicides
 * Server: {"type":"account","action":"return_to_menu","reason":"rent|death|quit|suicide","data":{"characters":[...]}}
 */
void ws_send_return_to_menu(struct descriptor_data *d, const char *reason)
{
    cJSON *data_obj;

    if (!d || !d->account) return;
    if (!d->websocket) return;

    data_obj = cJSON_CreateObject();
    cJSON_AddItemToObject(data_obj, "characters", ws_build_character_list(d));

    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "type", "account");
    cJSON_AddStringToObject(root, "action", "return_to_menu");
    cJSON_AddStringToObject(root, "reason", reason);
    cJSON_AddItemToObject(root, "data", data_obj);

    char *json_str = cJSON_PrintUnformatted(root);
    if (json_str) {
        websocket_send_text(d, json_str);
        free(json_str);
    }

    cJSON_Delete(root);

    statuslog(56, "Account %s returned to menu: %s", d->account->acct_name, reason);
}

/*
 * Main command dispatcher
 */
void ws_handle_command(struct descriptor_data *d, const char *cmd, cJSON *data)
{
    if (!cmd) return;

    if (strcmp(cmd, "login") == 0) {
        ws_cmd_login(d, data);
    } else if (strcmp(cmd, "register") == 0) {
        ws_cmd_register(d, data);
    } else if (strcmp(cmd, "enter") == 0) {
        ws_cmd_enter(d, data);
    } else if (strcmp(cmd, "game") == 0) {
        ws_cmd_game(d, data);
    } else if (strcmp(cmd, "chargen_options") == 0) {
        ws_cmd_chargen_options(d, data);
    } else if (strcmp(cmd, "roll_stats") == 0) {
        ws_cmd_roll_stats(d, data);
    } else if (strcmp(cmd, "add_bonus") == 0) {
        ws_cmd_add_bonus(d, data);
    } else if (strcmp(cmd, "validate_name") == 0) {
        ws_cmd_validate_name(d, data);
    } else if (strcmp(cmd, "get_hometowns") == 0) {
        ws_cmd_get_hometowns(d, data);
    } else if (strcmp(cmd, "create_character") == 0) {
        ws_cmd_create_character(d, data);
    } else if (strcmp(cmd, "swap_stats") == 0) {
        ws_cmd_swap_stats(d, data);
    } else if (strcmp(cmd, "account_info") == 0) {
        ws_cmd_account_info(d, data);
    } else if (strcmp(cmd, "change_email") == 0) {
        ws_cmd_change_email(d, data);
    } else if (strcmp(cmd, "change_password") == 0) {
        ws_cmd_change_password(d, data);
    } else if (strcmp(cmd, "delete_character") == 0) {
        ws_cmd_delete_character(d, data);
    } else if (strcmp(cmd, "rested_bonus") == 0) {
        ws_cmd_rested_bonus(d, data);
    } else if (strcmp(cmd, "logout") == 0) {
        ws_cmd_logout(d, data);
    } else {
        /* Unknown command - treat as game command */
        if (d->connected == CON_PLAYING) {
            write_to_q(cmd, &d->input, 0);
        }
    }
}

/*
 * Initialize WebSocket handlers
 */
void ws_handlers_init(void)
{
    statuslog(56, "WebSocket command handlers initialized");
}
