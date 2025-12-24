/*
 * ws_handlers.h - WebSocket command handlers for DurisMUD
 *
 * Handles JSON commands from web clients:
 * - login: Account authentication
 * - register: Account creation
 * - enter: Enter game with character
 * - game: In-game commands
 */

#ifndef DURIS_WS_HANDLERS_H
#define DURIS_WS_HANDLERS_H

#include "structs.h"
#include <cjson/cJSON.h>

/* Command handler function type */
typedef void (*ws_cmd_handler)(struct descriptor_data *d, cJSON *data);

/* Initialize WebSocket handlers */
void ws_handlers_init(void);

/* Main command dispatcher - called from websocket.c */
void ws_handle_command(struct descriptor_data *d, const char *cmd, cJSON *data);

/* Individual command handlers */
void ws_cmd_login(struct descriptor_data *d, cJSON *data);
void ws_cmd_register(struct descriptor_data *d, cJSON *data);
void ws_cmd_enter(struct descriptor_data *d, cJSON *data);
void ws_cmd_game(struct descriptor_data *d, cJSON *data);
void ws_cmd_chargen_options(struct descriptor_data *d, cJSON *data);
void ws_cmd_roll_stats(struct descriptor_data *d, cJSON *data);
void ws_cmd_create_character(struct descriptor_data *d, cJSON *data);

/* Account menu command handlers */
void ws_cmd_account_info(struct descriptor_data *d, cJSON *data);
void ws_cmd_change_email(struct descriptor_data *d, cJSON *data);
void ws_cmd_change_password(struct descriptor_data *d, cJSON *data);
void ws_cmd_delete_character(struct descriptor_data *d, cJSON *data);
void ws_cmd_rested_bonus(struct descriptor_data *d, cJSON *data);
void ws_cmd_logout(struct descriptor_data *d, cJSON *data);

/* Helper functions */
void ws_send_auth_success(struct descriptor_data *d, const char *account);
void ws_send_auth_failed(struct descriptor_data *d, const char *error);
void ws_send_reconnect_success(struct descriptor_data *d, const char *account, const char *char_name);
void ws_send_full_game_state(struct descriptor_data *d);
void ws_send_text(struct descriptor_data *d, const char *category, const char *text);
void ws_send_system(struct descriptor_data *d, const char *status, const char *message);
void ws_send_return_to_menu(struct descriptor_data *d, const char *reason);

#endif /* DURIS_WS_HANDLERS_H */
