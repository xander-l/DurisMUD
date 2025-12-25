/*
 * websocket.h - WebSocket protocol support for DurisMUD
 *
 * Implements RFC 6455 WebSocket protocol for native browser clients.
 * Works alongside existing telnet/SSL connections.
 *
 * Port: 4050 (configurable via WS_PORT)
 */

#ifndef WEBSOCKET_H
#define WEBSOCKET_H

#include "structs.h"

/* WebSocket Configuration */
#define WS_PORT             4050
#define WS_MAX_FRAME_SIZE   65536
#define WS_MAGIC_STRING     "258EAFA5-E914-47DA-95CA-C5AB0DC85B11"
#define WS_PING_INTERVAL    30    /* Send ping every 30 seconds */
#define WS_PING_TIMEOUT     60    /* Close if no pong within 60 seconds */

/* WebSocket Opcodes (RFC 6455 Section 5.2) */
#define WS_OPCODE_CONTINUATION  0x00
#define WS_OPCODE_TEXT          0x01
#define WS_OPCODE_BINARY        0x02
#define WS_OPCODE_CLOSE         0x08
#define WS_OPCODE_PING          0x09
#define WS_OPCODE_PONG          0x0A

/* WebSocket Close Codes (RFC 6455 Section 7.4.1) */
#define WS_CLOSE_NORMAL         1000
#define WS_CLOSE_GOING_AWAY     1001
#define WS_CLOSE_PROTOCOL_ERROR 1002
#define WS_CLOSE_UNSUPPORTED    1003
#define WS_CLOSE_NO_STATUS      1005
#define WS_CLOSE_ABNORMAL       1006
#define WS_CLOSE_INVALID_DATA   1007
#define WS_CLOSE_POLICY_VIOLATION 1008
#define WS_CLOSE_MESSAGE_TOO_BIG 1009
#define WS_CLOSE_EXTENSION_REQUIRED 1010
#define WS_CLOSE_INTERNAL_ERROR 1011

/* WebSocket Connection States */
#define WS_STATE_CONNECTING     0   /* HTTP upgrade in progress */
#define WS_STATE_OPEN           1   /* Handshake complete, ready for data */
#define WS_STATE_CLOSING        2   /* Close frame sent, waiting for response */
#define WS_STATE_CLOSED         3   /* Connection closed */

/* WebSocket data attached to descriptor */
struct websocket_data {
    int state;                      /* WS_STATE_* */
    int handshake_done;             /* HTTP upgrade complete */
    char *fragment_buffer;          /* For fragmented messages */
    size_t fragment_len;            /* Length of fragment buffer */
    int fragment_opcode;            /* Opcode of fragmented message */
};

/*
 * Initialization
 */

/* Initialize WebSocket subsystem, returns listening socket or -1 on error */
int websocket_init(int port);

/* Clean up WebSocket subsystem */
void websocket_shutdown(void);

/*
 * Connection Handling
 */

/* Accept new WebSocket connection, returns 0 on success, -1 on error */
int websocket_accept(int listen_fd, struct descriptor_data *d);

/* Process incoming data on WebSocket connection */
int websocket_process_input(struct descriptor_data *d);

/* Check if descriptor has pending WebSocket data */
int websocket_has_pending(struct descriptor_data *d);

/*
 * HTTP Upgrade Handshake
 */

/* Parse HTTP upgrade request, returns 1 if valid, 0 if not WebSocket, -1 on error */
int websocket_parse_handshake(struct descriptor_data *d, const char *buf, size_t len);

/* Send HTTP upgrade response, returns 0 on success */
int websocket_complete_handshake(struct descriptor_data *d, const char *key);

/*
 * Frame Handling
 */

/* Send text frame to WebSocket client */
int websocket_send_text(struct descriptor_data *d, const char *text);

/* Send binary frame to WebSocket client */
int websocket_send_binary(struct descriptor_data *d, const void *data, size_t len);

/* Send JSON message (wrapper for send_text with type/data structure) */
int websocket_send_json(struct descriptor_data *d, const char *type,
                        const char *package, const char *json);

/* Send close frame with optional reason */
int websocket_send_close(struct descriptor_data *d, int code, const char *reason);

/* Send ping frame */
int websocket_send_ping(struct descriptor_data *d);

/* Send pong frame in response to ping */
int websocket_send_pong(struct descriptor_data *d, const char *data, size_t len);

/*
 * Frame Parsing
 */

/* Parse WebSocket frame, returns number of bytes consumed, or -1 on error */
int websocket_parse_frame(struct descriptor_data *d, const char *buf, size_t len,
                          char **payload, size_t *payload_len, int *opcode);

/*
 * Utility Functions
 */

/* Generate WebSocket accept key from client key */
void websocket_generate_accept_key(const char *client_key, char *accept_key);

/* Check if connection is a WebSocket */
#define IS_WEBSOCKET(d) ((d) && (d)->websocket)

/* Close WebSocket connection properly */
void websocket_close(struct descriptor_data *d, int code, const char *reason);

/* Free WebSocket-specific data from descriptor */
void websocket_free(struct descriptor_data *d);

#endif /* WEBSOCKET_H */
