/*
 * websocket.c - WebSocket protocol implementation for DurisMUD
 *
 * Implements RFC 6455 WebSocket protocol for native browser clients.
 * Handles HTTP upgrade handshake, frame parsing, and message routing.
 *
 * Copyright (c) 2025 DurisMUD Development Team
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <openssl/sha.h>
#include <openssl/bio.h>
#include <openssl/evp.h>
#include <openssl/buffer.h>

#include "websocket.h"
#include "ws_handlers.h"
#include "json_utils.h"
#include "structs.h"
#include "prototypes.h"
#include "comm.h"
#include "utils.h"
#include "db.h"

/* External declarations */
extern struct descriptor_data *descriptor_list;

/* Static variables */
static int ws_listen_fd = -1;

/*
 * Base64 encoding helper
 */
static char *base64_encode(const unsigned char *input, int length) {
    BIO *bio, *b64;
    BUF_MEM *bufferPtr;
    char *output;

    b64 = BIO_new(BIO_f_base64());
    bio = BIO_new(BIO_s_mem());
    bio = BIO_push(b64, bio);

    BIO_set_flags(bio, BIO_FLAGS_BASE64_NO_NL);
    BIO_write(bio, input, length);
    BIO_flush(bio);
    BIO_get_mem_ptr(bio, &bufferPtr);

    output = (char *)malloc(bufferPtr->length + 1);
    if (output) {
        memcpy(output, bufferPtr->data, bufferPtr->length);
        output[bufferPtr->length] = '\0';
    }

    BIO_free_all(bio);
    return output;
}

/*
 * Generate WebSocket accept key from client key
 * RFC 6455 Section 4.2.2
 */
void websocket_generate_accept_key(const char *client_key, char *accept_key) {
    char concat[256];
    unsigned char sha1_hash[SHA_DIGEST_LENGTH];

    /* Concatenate client key with magic string */
    snprintf(concat, sizeof(concat), "%s%s", client_key, WS_MAGIC_STRING);

    /* SHA-1 hash */
    SHA1((unsigned char *)concat, strlen(concat), sha1_hash);

    /* Base64 encode */
    char *encoded = base64_encode(sha1_hash, SHA_DIGEST_LENGTH);
    if (encoded) {
        strcpy(accept_key, encoded);
        free(encoded);
    }
}

/*
 * Initialize WebSocket subsystem
 */
int websocket_init(int port) {
    struct sockaddr_in sa;
    int opt = 1;

    ws_listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (ws_listen_fd < 0) {
        perror("websocket_init: socket");
        return -1;
    }

    /* Allow socket reuse */
    if (setsockopt(ws_listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("websocket_init: setsockopt SO_REUSEADDR");
        close(ws_listen_fd);
        return -1;
    }

    /* Set up address */
    memset(&sa, 0, sizeof(sa));
    sa.sin_family = AF_INET;
    sa.sin_port = htons(port);
    sa.sin_addr.s_addr = INADDR_ANY;

    /* Bind */
    if (bind(ws_listen_fd, (struct sockaddr *)&sa, sizeof(sa)) < 0) {
        perror("websocket_init: bind");
        close(ws_listen_fd);
        return -1;
    }

    /* Listen */
    if (listen(ws_listen_fd, 40) < 0) {
        perror("websocket_init: listen");
        close(ws_listen_fd);
        return -1;
    }

    /* Non-blocking */
    fcntl(ws_listen_fd, F_SETFL, O_NONBLOCK);

    statuslog(56, "WebSocket server listening on port %d", port);
    return ws_listen_fd;
}

/*
 * Shutdown WebSocket subsystem
 */
void websocket_shutdown(void) {
    if (ws_listen_fd >= 0) {
        close(ws_listen_fd);
        ws_listen_fd = -1;
    }
}

/*
 * Accept new WebSocket connection
 */
int websocket_accept(int listen_fd, struct descriptor_data *d) {
    struct sockaddr_in peer;
    socklen_t peer_len = sizeof(peer);
    int new_fd;
    int opt = 1;

    new_fd = accept(listen_fd, (struct sockaddr *)&peer, &peer_len);
    if (new_fd < 0) {
        if (errno != EWOULDBLOCK && errno != EAGAIN) {
            perror("websocket_accept: accept");
        }
        return -1;
    }

    /* TCP_NODELAY for low latency */
    setsockopt(new_fd, IPPROTO_TCP, TCP_NODELAY, &opt, sizeof(opt));

    /* Non-blocking */
    fcntl(new_fd, F_SETFL, O_NONBLOCK);

    /* Initialize descriptor for WebSocket */
    d->descriptor = new_fd;
    d->websocket = 1;
    d->gmcp_enabled = 1;  /* WebSocket clients always have GMCP enabled */
    d->ws_state = WS_STATE_CONNECTING;
    d->ws_handshake_done = 0;

    /* Get peer address */
    strncpy(d->host, inet_ntoa(peer.sin_addr), sizeof(d->host) - 1);

    statuslog(56, "WebSocket connection from %s", d->host);

    return 0;
}

/*
 * Parse HTTP upgrade request
 * Returns 1 if valid WebSocket upgrade, 0 if not, -1 on error
 */
int websocket_parse_handshake(struct descriptor_data *d, const char *buf, size_t len) {
    char *line, *saveptr;
    char *request = NULL;
    char ws_key[128] = {0};
    int is_upgrade = 0;
    int is_websocket = 0;
    int version_ok = 0;

    /* Make a copy for strtok */
    request = (char *)malloc(len + 1);
    if (!request) return -1;
    memcpy(request, buf, len);
    request[len] = '\0';

    /* Parse headers line by line */
    line = strtok_r(request, "\r\n", &saveptr);
    while (line) {
        /* Check for GET request */
        if (strncmp(line, "GET ", 4) == 0) {
            /* Valid HTTP GET */
        }
        /* Upgrade header */
        else if (strncasecmp(line, "Upgrade:", 8) == 0) {
            char *value = line + 8;
            while (*value == ' ') value++;
            if (strcasecmp(value, "websocket") == 0) {
                is_websocket = 1;
            }
        }
        /* Connection header */
        else if (strncasecmp(line, "Connection:", 11) == 0) {
            char *value = line + 11;
            while (*value == ' ') value++;
            if (strcasestr(value, "Upgrade") != NULL) {
                is_upgrade = 1;
            }
        }
        /* WebSocket key */
        else if (strncasecmp(line, "Sec-WebSocket-Key:", 18) == 0) {
            char *value = line + 18;
            while (*value == ' ') value++;
            strncpy(ws_key, value, sizeof(ws_key) - 1);
        }
        /* WebSocket version */
        else if (strncasecmp(line, "Sec-WebSocket-Version:", 22) == 0) {
            char *value = line + 22;
            while (*value == ' ') value++;
            if (atoi(value) == 13) {
                version_ok = 1;
            }
        }

        line = strtok_r(NULL, "\r\n", &saveptr);
    }

    free(request);

    /* Validate all requirements */
    if (!is_upgrade || !is_websocket || !version_ok || ws_key[0] == '\0') {
        return 0;
    }

    /* Complete handshake */
    return websocket_complete_handshake(d, ws_key);
}

/*
 * Send HTTP upgrade response
 */
int websocket_complete_handshake(struct descriptor_data *d, const char *key) {
    char accept_key[64];
    char response[512];
    int len;
    struct descriptor_data *k, *next_k;

    /* Generate accept key */
    websocket_generate_accept_key(key, accept_key);

    /* Build response */
    len = snprintf(response, sizeof(response),
        "HTTP/1.1 101 Switching Protocols\r\n"
        "Upgrade: websocket\r\n"
        "Connection: Upgrade\r\n"
        "Sec-WebSocket-Accept: %s\r\n"
        "\r\n",
        accept_key);

    /* Send response */
    if (write(d->descriptor, response, len) != len) {
        return -1;
    }

    d->ws_state = WS_STATE_OPEN;
    d->ws_handshake_done = 1;
    d->connected = 60;  /* CON_GET_ACCT_NAME - ready for account login */

    /*
     * DUPLICATE CONNECTION CHECK:
     * Kick any other unauthenticated WebSocket connections from the same IP.
     * This handles HMR, page refresh, and rapid reconnection scenarios.
     */
    for (k = descriptor_list; k; k = next_k) {
        next_k = k->next;

        /* Skip self */
        if (k == d) continue;

        /* Only kick unauthenticated WebSocket connections from same IP */
        if (k->websocket && k->ws_handshake_done &&
            !k->account &&  /* Not logged in yet */
            k->connected != CON_PLAYING &&
            strcmp(k->host, d->host) == 0) {

            statuslog(56, "WebSocket: Kicking stale connection from %s (new connection established)",
                      k->host);

            ws_send_system(k, "kicked", "New connection established from your browser.");
            websocket_close(k, WS_CLOSE_NORMAL, "New connection");
            close_socket(k);
        }
    }

    statuslog(56, "WebSocket handshake complete for %s", d->host);

    /* Send welcome message - client is ready for login */
    ws_send_system(d, "connected", "Welcome to NewDuris MUD!");

    return 1;
}

/*
 * Build and send a WebSocket frame
 */
static int websocket_send_frame(struct descriptor_data *d, int opcode,
                                const void *data, size_t len) {
    unsigned char *frame;
    size_t frame_len;
    size_t offset = 0;
    ssize_t sent;

    if (!d || d->descriptor < 0) return -1;

    /* Calculate frame size */
    if (len <= 125) {
        frame_len = 2 + len;
    } else if (len <= 65535) {
        frame_len = 4 + len;
    } else {
        frame_len = 10 + len;
    }

    frame = (unsigned char *)malloc(frame_len);
    if (!frame) return -1;

    /* First byte: FIN + opcode */
    frame[offset++] = 0x80 | (opcode & 0x0F);

    /* Second byte: payload length (server doesn't mask) */
    if (len <= 125) {
        frame[offset++] = (unsigned char)len;
    } else if (len <= 65535) {
        frame[offset++] = 126;
        frame[offset++] = (len >> 8) & 0xFF;
        frame[offset++] = len & 0xFF;
    } else {
        frame[offset++] = 127;
        frame[offset++] = 0;
        frame[offset++] = 0;
        frame[offset++] = 0;
        frame[offset++] = 0;
        frame[offset++] = (len >> 24) & 0xFF;
        frame[offset++] = (len >> 16) & 0xFF;
        frame[offset++] = (len >> 8) & 0xFF;
        frame[offset++] = len & 0xFF;
    }

    /* Payload */
    if (len > 0 && data) {
        memcpy(frame + offset, data, len);
    }

    /* Send frame */
    sent = write(d->descriptor, frame, frame_len);
    free(frame);

    if (sent < 0) {
        if (errno != EWOULDBLOCK && errno != EAGAIN) {
            return -1;
        }
        /* Would block - queue for later? For now, drop */
        return 0;
    }

    return (sent == (ssize_t)frame_len) ? 0 : -1;
}

/*
 * Send text frame
 */
int websocket_send_text(struct descriptor_data *d, const char *text) {
    if (!text) return -1;
    return websocket_send_frame(d, WS_OPCODE_TEXT, text, strlen(text));
}

/*
 * Send binary frame
 */
int websocket_send_binary(struct descriptor_data *d, const void *data, size_t len) {
    return websocket_send_frame(d, WS_OPCODE_BINARY, data, len);
}

/*
 * Send JSON message with type wrapper
 */
int websocket_send_json(struct descriptor_data *d, const char *type,
                        const char *package, const char *json) {
    char *message;
    int result;

    if (package) {
        /* GMCP-style message */
        message = json_build_gmcp_message(package, json);
    } else {
        /* Simple message - json is already complete */
        message = strdup(json);
    }

    if (!message) return -1;

    result = websocket_send_text(d, message);
    free(message);
    return result;
}

/*
 * Send close frame
 */
int websocket_send_close(struct descriptor_data *d, int code, const char *reason) {
    unsigned char payload[128];
    size_t len = 0;

    if (code > 0) {
        payload[0] = (code >> 8) & 0xFF;
        payload[1] = code & 0xFF;
        len = 2;

        if (reason) {
            size_t reason_len = strlen(reason);
            if (reason_len > sizeof(payload) - 2) {
                reason_len = sizeof(payload) - 2;
            }
            memcpy(payload + 2, reason, reason_len);
            len += reason_len;
        }
    }

    d->ws_state = WS_STATE_CLOSING;
    return websocket_send_frame(d, WS_OPCODE_CLOSE, payload, len);
}

/*
 * Send ping frame
 */
int websocket_send_ping(struct descriptor_data *d) {
    return websocket_send_frame(d, WS_OPCODE_PING, NULL, 0);
}

/*
 * Send pong frame
 */
int websocket_send_pong(struct descriptor_data *d, const char *data, size_t len) {
    return websocket_send_frame(d, WS_OPCODE_PONG, data, len);
}

/*
 * Parse WebSocket frame
 * Returns bytes consumed, or -1 on error, 0 if need more data
 */
int websocket_parse_frame(struct descriptor_data *d, const char *buf, size_t len,
                          char **payload, size_t *payload_len, int *opcode) {
    size_t offset = 0;
    size_t frame_len;
    size_t data_len;
    int fin, op, mask_bit;
    unsigned char mask_key[4];
    size_t i;

    *payload = NULL;
    *payload_len = 0;
    *opcode = -1;

    /* Need at least 2 bytes for header */
    if (len < 2) return 0;

    /* Parse first byte */
    fin = (buf[0] >> 7) & 0x01;
    op = buf[0] & 0x0F;
    offset++;

    /* Parse second byte */
    mask_bit = (buf[1] >> 7) & 0x01;
    data_len = buf[1] & 0x7F;
    offset++;

    /* Extended length */
    if (data_len == 126) {
        if (len < 4) return 0;
        data_len = ((unsigned char)buf[2] << 8) | (unsigned char)buf[3];
        offset += 2;
    } else if (data_len == 127) {
        if (len < 10) return 0;
        /* Only support up to 32-bit length for sanity */
        data_len = ((unsigned char)buf[6] << 24) |
                   ((unsigned char)buf[7] << 16) |
                   ((unsigned char)buf[8] << 8) |
                   (unsigned char)buf[9];
        offset += 8;
    }

    /* Sanity check */
    if (data_len > WS_MAX_FRAME_SIZE) {
        return -1;
    }

    /* Mask key (client must mask) */
    if (mask_bit) {
        if (len < offset + 4) return 0;
        memcpy(mask_key, buf + offset, 4);
        offset += 4;
    }

    /* Check if we have full payload */
    frame_len = offset + data_len;
    if (len < frame_len) return 0;

    /* Allocate and unmask payload */
    if (data_len > 0) {
        *payload = (char *)malloc(data_len + 1);
        if (!*payload) return -1;

        memcpy(*payload, buf + offset, data_len);

        if (mask_bit) {
            for (i = 0; i < data_len; i++) {
                (*payload)[i] ^= mask_key[i % 4];
            }
        }

        (*payload)[data_len] = '\0';
        *payload_len = data_len;
    }

    *opcode = op;

    /* Handle control frames */
    if (op == WS_OPCODE_CLOSE) {
        websocket_send_close(d, WS_CLOSE_NORMAL, NULL);
        d->ws_state = WS_STATE_CLOSED;
    } else if (op == WS_OPCODE_PING) {
        websocket_send_pong(d, *payload, *payload_len);
        free(*payload);
        *payload = NULL;
        *payload_len = 0;
    } else if (op == WS_OPCODE_PONG) {
        /* Ignore pong */
        free(*payload);
        *payload = NULL;
        *payload_len = 0;
    }

    return (int)frame_len;
}

/*
 * Close WebSocket connection properly
 */
void websocket_close(struct descriptor_data *d, int code, const char *reason) {
    if (!d) return;

    if (d->ws_state == WS_STATE_OPEN) {
        websocket_send_close(d, code, reason);
    }

    d->ws_state = WS_STATE_CLOSED;
}

/*
 * Free WebSocket-specific data
 */
void websocket_free(struct descriptor_data *d) {
    if (!d) return;

    /* Free fragment buffer if any */
    if (d->ws_fragment_buffer) {
        free(d->ws_fragment_buffer);
        d->ws_fragment_buffer = NULL;
    }

    d->websocket = 0;
    d->ws_state = WS_STATE_CLOSED;
}

/*
 * Process incoming WebSocket data
 * Called from game loop when descriptor has data
 */
int websocket_process_input(struct descriptor_data *d) {
    char buf[4096];
    ssize_t bytes_read;
    int consumed;
    char *payload;
    size_t payload_len;
    int opcode;

    if (!d || d->descriptor < 0) return -1;

    /* Read data */
    bytes_read = read(d->descriptor, buf, sizeof(buf) - 1);

    if (bytes_read < 0) {
        if (errno == EWOULDBLOCK || errno == EAGAIN) {
            return 0;
        }
        return -1;
    }

    if (bytes_read == 0) {
        /* Connection closed */
        return -1;
    }

    buf[bytes_read] = '\0';

    /* Handle handshake if not done */
    if (!d->ws_handshake_done) {
        int result = websocket_parse_handshake(d, buf, bytes_read);
        if (result <= 0) {
            /* Invalid handshake - close connection */
            return -1;
        }
        return 0;
    }

    /* Parse WebSocket frames */
    consumed = websocket_parse_frame(d, buf, bytes_read, &payload, &payload_len, &opcode);

    if (consumed < 0) {
        return -1;
    }

    if (consumed == 0) {
        /* Need more data - for now, drop partial frames */
        return 0;
    }

    /* Handle text frame (JSON command) */
    if (opcode == WS_OPCODE_TEXT && payload) {
        /* Parse JSON and extract command/data */
        cJSON *json = cJSON_Parse(payload);
        if (json) {
            const char *type = NULL;
            const char *cmd = NULL;
            cJSON *type_item = cJSON_GetObjectItem(json, "type");
            cJSON *cmd_item = cJSON_GetObjectItem(json, "cmd");
            cJSON *data_item = cJSON_GetObjectItem(json, "data");

            if (type_item && cJSON_IsString(type_item))
                type = type_item->valuestring;
            if (cmd_item && cJSON_IsString(cmd_item))
                cmd = cmd_item->valuestring;

            if (type && strcmp(type, "cmd") == 0 && cmd) {
                /* Use WebSocket command handler */
                ws_handle_command(d, cmd, data_item);
            } else if (d->connected == CON_PLAYING) {
                /* In-game: pass raw text as command */
                if (data_item && cJSON_IsString(data_item)) {
                    write_to_q(data_item->valuestring, &d->input, 0);
                } else if (cmd) {
                    write_to_q(cmd, &d->input, 0);
                }
            }
            cJSON_Delete(json);
        } else {
            /* Not valid JSON - treat as raw text command if in game */
            if (d->connected == CON_PLAYING) {
                write_to_q(payload, &d->input, 0);
            }
        }
        free(payload);
    }

    return 0;
}

/*
 * Check if descriptor has pending WebSocket data
 */
int websocket_has_pending(struct descriptor_data *d) {
    if (!d) return 0;
    return (d->ws_fragment_buffer != NULL && d->ws_fragment_len > 0);
}
