/*
 * gmcp.h - GMCP (Generic MUD Communication Protocol) for DurisMUD
 *
 * Implements GMCP protocol for both telnet (Mudlet/cMUD) and WebSocket clients.
 * Same data, different transport layer.
 *
 * Mudlet/IRE Compatible Package Names:
 *   - Room.Info
 *   - Char.Vitals
 *   - Char.Status
 *   - Char.Affects
 *   - Combat.Update
 *   - Comm.Channel
 */

#ifndef GMCP_H
#define GMCP_H

#include "structs.h"

/* GMCP Telnet Codes (RFC 1572 compliant) */
#define TELNET_IAC      255
#define TELNET_SB       250     /* Subnegotiation Begin */
#define TELNET_SE       240     /* Subnegotiation End */
#define TELNET_WILL     251
#define TELNET_WONT     252
#define TELNET_DO       253
#define TELNET_DONT     254
#define TELOPT_GMCP     201     /* GMCP option code */

/* GMCP Package Names */
#define GMCP_PKG_ROOM_INFO      "Room.Info"
#define GMCP_PKG_ROOM_MAP       "Room.Map"
#define GMCP_PKG_CHAR_VITALS    "Char.Vitals"
#define GMCP_PKG_CHAR_STATUS    "Char.Status"
#define GMCP_PKG_CHAR_AFFECTS   "Char.Affects"
#define GMCP_PKG_COMBAT_UPDATE  "Combat.Update"
#define GMCP_PKG_COMM_CHANNEL   "Comm.Channel"
#define GMCP_PKG_CHAR_SKILLS    "Char.Skills"
#define GMCP_PKG_CHAR_ITEMS     "Char.Items"
#define GMCP_PKG_QUEST_STATUS   "Quest.Status"
#define GMCP_PKG_GROUP_STATUS   "Group.Status"

/*
 * Initialization & Negotiation
 */

/* Send GMCP WILL to start negotiation (called during connection setup) */
void gmcp_negotiate(struct descriptor_data *d);

/* Handle GMCP DO/DONT response from client */
void gmcp_handle_negotiation(struct descriptor_data *d, int cmd);

/* Parse incoming GMCP data from telnet client */
void gmcp_handle_input(struct descriptor_data *d, const char *data, size_t len);

/*
 * Core Send Function
 * Handles both telnet (IAC SB GMCP) and WebSocket (JSON) transport
 */
void gmcp_send(struct descriptor_data *d, const char *package, const char *json);

/*
 * Room Information
 * Sends room data when character enters a room
 */
void gmcp_room_info(struct char_data *ch);

/*
 * Room Map (for wilderness zones)
 * Sends ASCII map when character is in surface/underdark/alatorin zones
 */
void gmcp_room_map(struct char_data *ch);

/* Send pre-generated map buffer via GMCP (called from map.c) */
void gmcp_send_room_map(struct char_data *ch, const char *map_buf);

/*
 * Room Update Throttling
 * Mark rooms as "dirty" on entity changes, flush periodically
 */
void gmcp_mark_room_dirty(int room_number);
void gmcp_flush_dirty_rooms(void);

/*
 * Character Vitals
 * Sends HP/Mana/Move when they change
 */
void gmcp_char_vitals(struct char_data *ch);

/* Send vitals to everyone in the group */
void gmcp_group_vitals(struct char_data *ch);

/*
 * Group Status
 * Sends group member list with HP/Move/position for group panel
 */
void gmcp_send_group_status(struct char_data *ch);

/*
 * Character Status
 * Sends level, class, race, etc. (on login, level up)
 */
void gmcp_char_status(struct char_data *ch);

/*
 * Character Affects
 * Sends active buffs/debuffs when they change
 */
void gmcp_char_affects(struct char_data *ch);

/*
 * Combat Updates
 * Sends combat round information
 */
void gmcp_combat_update(struct char_data *ch, struct char_data *victim,
                        int damage, const char *damage_type, int critical);

/* Send target health update */
void gmcp_combat_target(struct char_data *ch, struct char_data *victim);

/* Combat ended */
void gmcp_combat_end(struct char_data *ch);

/*
 * Communication Channels
 * Sends channel messages (gossip, guild, tell, etc.)
 */
void gmcp_comm_channel(struct char_data *ch, const char *channel,
                       const char *sender, const char *text);

/*
 * Channel Communication (extended)
 * Sends channel messages with alignment info (for nchat)
 */
void gmcp_comm_channel_ex(struct char_data *ch, const char *channel,
                          const char *sender, const char *text,
                          const char *alignment);

/*
 * Quest Status
 * Sends current bartender quest status
 */
void gmcp_quest_status(struct char_data *ch);

/* Send to all who can hear the channel */
void gmcp_broadcast_channel(const char *channel, const char *sender,
                            const char *text, struct char_data *exclude);

/*
 * Utility Functions
 */

/* Check if character has GMCP enabled (client negotiated AND not toggled off) */
#define GMCP_ENABLED(ch) ((ch) && (ch)->desc && (ch)->desc->gmcp_enabled && \
                          !PLR3_FLAGGED(ch, PLR3_NOGMCP))

/* Check if descriptor has GMCP enabled */
#define DESC_GMCP_ENABLED(d) ((d) && (d)->gmcp_enabled)

/* Send GMCP to character if enabled */
#define GMCP_TO_CHAR(ch, pkg, json) \
    do { if (GMCP_ENABLED(ch)) gmcp_send((ch)->desc, (pkg), (json)); } while(0)

#endif /* GMCP_H */
