#include "cJSON.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char *cjson_error_ptr = NULL;

static void set_error(const char *ptr) {
    if (!cjson_error_ptr) {
        cjson_error_ptr = ptr;
    }
}

const char *cJSON_GetErrorPtr(void) {
    return cjson_error_ptr;
}

void cJSON_free(void *ptr) {
    free(ptr);
}

static cJSON *cjson_new_item(void) {
    cJSON *item = (cJSON *)calloc(1, sizeof(cJSON));
    return item;
}

static void skip_whitespace(const char **ptr) {
    while (**ptr && isspace((unsigned char)**ptr)) {
        (*ptr)++;
    }
}

static int hex4_to_int(const char *str, unsigned int *out) {
    unsigned int val = 0;
    for (int i = 0; i < 4; i++) {
        char c = str[i];
        val <<= 4;
        if (c >= '0' && c <= '9') {
            val |= (unsigned int)(c - '0');
        } else if (c >= 'A' && c <= 'F') {
            val |= (unsigned int)(c - 'A' + 10);
        } else if (c >= 'a' && c <= 'f') {
            val |= (unsigned int)(c - 'a' + 10);
        } else {
            return 0;
        }
    }
    *out = val;
    return 1;
}

static int utf8_from_codepoint(unsigned int codepoint, char *out) {
    if (codepoint <= 0x7F) {
        out[0] = (char)codepoint;
        return 1;
    }
    if (codepoint <= 0x7FF) {
        out[0] = (char)(0xC0 | (codepoint >> 6));
        out[1] = (char)(0x80 | (codepoint & 0x3F));
        return 2;
    }
    if (codepoint <= 0xFFFF) {
        out[0] = (char)(0xE0 | (codepoint >> 12));
        out[1] = (char)(0x80 | ((codepoint >> 6) & 0x3F));
        out[2] = (char)(0x80 | (codepoint & 0x3F));
        return 3;
    }
    if (codepoint <= 0x10FFFF) {
        out[0] = (char)(0xF0 | (codepoint >> 18));
        out[1] = (char)(0x80 | ((codepoint >> 12) & 0x3F));
        out[2] = (char)(0x80 | ((codepoint >> 6) & 0x3F));
        out[3] = (char)(0x80 | (codepoint & 0x3F));
        return 4;
    }
    return 0;
}

static char *parse_string(const char **ptr) {
    const char *p = *ptr;
    if (*p != '"') {
        set_error(p);
        return NULL;
    }
    p++;

    size_t cap = 16;
    size_t len = 0;
    char *out = (char *)malloc(cap);
    if (!out) return NULL;

    while (*p && *p != '"') {
        char ch = *p++;
        if (ch == '\\') {
            char esc = *p++;
            switch (esc) {
                case '"': ch = '"'; break;
                case '\\': ch = '\\'; break;
                case '/': ch = '/'; break;
                case 'b': ch = '\b'; break;
                case 'f': ch = '\f'; break;
                case 'n': ch = '\n'; break;
                case 'r': ch = '\r'; break;
                case 't': ch = '\t'; break;
                case 'u': {
                    unsigned int codepoint = 0;
                    if (!hex4_to_int(p, &codepoint)) {
                        free(out);
                        set_error(p);
                        return NULL;
                    }
                    p += 4;
                    char utf8[4];
                    int written = utf8_from_codepoint(codepoint, utf8);
                    if (written <= 0) {
                        free(out);
                        set_error(p);
                        return NULL;
                    }
                    if (len + (size_t)written + 1 > cap) {
                        cap = (cap + written + 1) * 2;
                        char *tmp = (char *)realloc(out, cap);
                        if (!tmp) {
                            free(out);
                            return NULL;
                        }
                        out = tmp;
                    }
                    memcpy(out + len, utf8, (size_t)written);
                    len += (size_t)written;
                    continue;
                }
                default:
                    free(out);
                    set_error(p - 1);
                    return NULL;
            }
        }
        if (len + 2 > cap) {
            cap *= 2;
            char *tmp = (char *)realloc(out, cap);
            if (!tmp) {
                free(out);
                return NULL;
            }
            out = tmp;
        }
        out[len++] = ch;
    }

    if (*p != '"') {
        free(out);
        set_error(p);
        return NULL;
    }
    p++;

    out[len] = '\0';
    *ptr = p;
    return out;
}

static cJSON *parse_value(const char **ptr);

static cJSON *parse_array(const char **ptr) {
    const char *p = *ptr;
    if (*p != '[') {
        set_error(p);
        return NULL;
    }
    p++;
    skip_whitespace(&p);

    cJSON *array = cjson_new_item();
    if (!array) return NULL;
    array->type = cJSON_Array;

    if (*p == ']') {
        p++;
        *ptr = p;
        return array;
    }

    while (*p) {
        skip_whitespace(&p);
        cJSON *item = parse_value(&p);
        if (!item) {
            cJSON_Delete(array);
            return NULL;
        }
        cJSON_AddItemToArray(array, item);
        skip_whitespace(&p);
        if (*p == ',') {
            p++;
            continue;
        }
        if (*p == ']') {
            p++;
            *ptr = p;
            return array;
        }
        set_error(p);
        cJSON_Delete(array);
        return NULL;
    }
    cJSON_Delete(array);
    set_error(p);
    return NULL;
}

static cJSON *parse_object(const char **ptr) {
    const char *p = *ptr;
    if (*p != '{') {
        set_error(p);
        return NULL;
    }
    p++;
    skip_whitespace(&p);

    cJSON *object = cjson_new_item();
    if (!object) return NULL;
    object->type = cJSON_Object;

    if (*p == '}') {
        p++;
        *ptr = p;
        return object;
    }

    while (*p) {
        skip_whitespace(&p);
        char *key = parse_string(&p);
        if (!key) {
            cJSON_Delete(object);
            return NULL;
        }
        skip_whitespace(&p);
        if (*p != ':') {
            free(key);
            cJSON_Delete(object);
            set_error(p);
            return NULL;
        }
        p++;
        skip_whitespace(&p);
        cJSON *value = parse_value(&p);
        if (!value) {
            free(key);
            cJSON_Delete(object);
            return NULL;
        }
        cJSON_AddItemToObject(object, key, value);
        free(key);
        skip_whitespace(&p);
        if (*p == ',') {
            p++;
            continue;
        }
        if (*p == '}') {
            p++;
            *ptr = p;
            return object;
        }
        cJSON_Delete(object);
        set_error(p);
        return NULL;
    }

    cJSON_Delete(object);
    set_error(p);
    return NULL;
}

static cJSON *parse_number(const char **ptr) {
    const char *p = *ptr;
    char *end = NULL;
    double val = strtod(p, &end);
    if (end == p) {
        set_error(p);
        return NULL;
    }
    cJSON *num = cjson_new_item();
    if (!num) return NULL;
    num->type = cJSON_Number;
    num->valuedouble = val;
    num->valueint = (int)val;
    *ptr = end;
    return num;
}

static cJSON *parse_value(const char **ptr) {
    skip_whitespace(ptr);
    const char *p = *ptr;
    if (!p || !*p) {
        set_error(p);
        return NULL;
    }

    if (*p == '"') {
        char *str = parse_string(&p);
        if (!str) return NULL;
        cJSON *item = cjson_new_item();
        if (!item) {
            free(str);
            return NULL;
        }
        item->type = cJSON_String;
        item->valuestring = str;
        *ptr = p;
        return item;
    }

    if (*p == '{') {
        cJSON *obj = parse_object(&p);
        if (!obj) return NULL;
        *ptr = p;
        return obj;
    }

    if (*p == '[') {
        cJSON *arr = parse_array(&p);
        if (!arr) return NULL;
        *ptr = p;
        return arr;
    }

    if (!strncmp(p, "true", 4)) {
        cJSON *item = cjson_new_item();
        if (!item) return NULL;
        item->type = cJSON_True;
        item->valueint = 1;
        *ptr = p + 4;
        return item;
    }

    if (!strncmp(p, "false", 5)) {
        cJSON *item = cjson_new_item();
        if (!item) return NULL;
        item->type = cJSON_False;
        item->valueint = 0;
        *ptr = p + 5;
        return item;
    }

    if (!strncmp(p, "null", 4)) {
        cJSON *item = cjson_new_item();
        if (!item) return NULL;
        item->type = cJSON_NULL;
        *ptr = p + 4;
        return item;
    }

    if (*p == '-' || (*p >= '0' && *p <= '9')) {
        cJSON *num = parse_number(&p);
        if (!num) return NULL;
        *ptr = p;
        return num;
    }

    set_error(p);
    return NULL;
}

cJSON *cJSON_Parse(const char *value) {
    if (!value) return NULL;
    cjson_error_ptr = NULL;
    const char *p = value;
    cJSON *item = parse_value(&p);
    if (!item) return NULL;
    skip_whitespace(&p);
    if (*p) {
        cJSON_Delete(item);
        set_error(p);
        return NULL;
    }
    return item;
}

void cJSON_Delete(cJSON *item) {
    if (!item) return;
    cJSON *child = item->child;
    while (child) {
        cJSON *next = child->next;
        cJSON_Delete(child);
        child = next;
    }
    free(item->valuestring);
    free(item->string);
    free(item);
}

static void append_char(char **buf, size_t *len, size_t *cap, char ch) {
    if (*len + 2 > *cap) {
        *cap = (*cap == 0) ? 64 : (*cap * 2);
        char *tmp = (char *)realloc(*buf, *cap);
        if (!tmp) return;
        *buf = tmp;
    }
    (*buf)[(*len)++] = ch;
    (*buf)[*len] = '\0';
}

static void append_str(char **buf, size_t *len, size_t *cap, const char *str) {
    while (*str) {
        append_char(buf, len, cap, *str++);
    }
}

static void print_string(const char *str, char **buf, size_t *len, size_t *cap) {
    append_char(buf, len, cap, '"');
    for (const unsigned char *p = (const unsigned char *)str; *p; p++) {
        unsigned char ch = *p;
        switch (ch) {
            case '"': append_str(buf, len, cap, "\\\""); break;
            case '\\': append_str(buf, len, cap, "\\\\"); break;
            case '\b': append_str(buf, len, cap, "\\b"); break;
            case '\f': append_str(buf, len, cap, "\\f"); break;
            case '\n': append_str(buf, len, cap, "\\n"); break;
            case '\r': append_str(buf, len, cap, "\\r"); break;
            case '\t': append_str(buf, len, cap, "\\t"); break;
            default:
                if (ch < 0x20) {
                    char tmp[7];
                    snprintf(tmp, sizeof(tmp), "\\u%04x", ch);
                    append_str(buf, len, cap, tmp);
                } else {
                    append_char(buf, len, cap, (char)ch);
                }
                break;
        }
    }
    append_char(buf, len, cap, '"');
}

static void print_value(const cJSON *item, char **buf, size_t *len, size_t *cap);

static void print_array(const cJSON *item, char **buf, size_t *len, size_t *cap) {
    append_char(buf, len, cap, '[');
    const cJSON *child = item->child;
    while (child) {
        print_value(child, buf, len, cap);
        if (child->next) {
            append_char(buf, len, cap, ',');
        }
        child = child->next;
    }
    append_char(buf, len, cap, ']');
}

static void print_object(const cJSON *item, char **buf, size_t *len, size_t *cap) {
    append_char(buf, len, cap, '{');
    const cJSON *child = item->child;
    while (child) {
        if (child->string) {
            print_string(child->string, buf, len, cap);
            append_char(buf, len, cap, ':');
        } else {
            append_str(buf, len, cap, "\"\"");
            append_char(buf, len, cap, ':');
        }
        print_value(child, buf, len, cap);
        if (child->next) {
            append_char(buf, len, cap, ',');
        }
        child = child->next;
    }
    append_char(buf, len, cap, '}');
}

static void print_value(const cJSON *item, char **buf, size_t *len, size_t *cap) {
    if (!item) return;
    if (cJSON_IsNull(item)) {
        append_str(buf, len, cap, "null");
    } else if (cJSON_IsBool(item)) {
        append_str(buf, len, cap, cJSON_IsTrue(item) ? "true" : "false");
    } else if (cJSON_IsNumber(item)) {
        char num[64];
        snprintf(num, sizeof(num), "%.17g", item->valuedouble);
        append_str(buf, len, cap, num);
    } else if (cJSON_IsString(item)) {
        print_string(item->valuestring ? item->valuestring : "", buf, len, cap);
    } else if (cJSON_IsArray(item)) {
        print_array(item, buf, len, cap);
    } else if (cJSON_IsObject(item)) {
        print_object(item, buf, len, cap);
    }
}

char *cJSON_PrintUnformatted(const cJSON *item) {
    if (!item) return NULL;
    size_t len = 0;
    size_t cap = 0;
    char *buf = NULL;
    print_value(item, &buf, &len, &cap);
    if (!buf) return NULL;
    return buf;
}

char *cJSON_Print(const cJSON *item) {
    return cJSON_PrintUnformatted(item);
}

cJSON *cJSON_CreateObject(void) {
    cJSON *item = cjson_new_item();
    if (!item) return NULL;
    item->type = cJSON_Object;
    return item;
}

cJSON *cJSON_CreateArray(void) {
    cJSON *item = cjson_new_item();
    if (!item) return NULL;
    item->type = cJSON_Array;
    return item;
}

cJSON *cJSON_CreateNumber(double num) {
    cJSON *item = cjson_new_item();
    if (!item) return NULL;
    item->type = cJSON_Number;
    item->valuedouble = num;
    item->valueint = (int)num;
    return item;
}

cJSON *cJSON_CreateString(const char *string) {
    cJSON *item = cjson_new_item();
    if (!item) return NULL;
    item->type = cJSON_String;
    item->valuestring = string ? strdup(string) : strdup("");
    if (!item->valuestring) {
        free(item);
        return NULL;
    }
    return item;
}

static void add_item_to_end(cJSON *parent, cJSON *item) {
    if (!parent || !item) return;
    if (!parent->child) {
        parent->child = item;
        return;
    }
    cJSON *child = parent->child;
    while (child->next) {
        child = child->next;
    }
    child->next = item;
    item->prev = child;
}

void cJSON_AddItemToArray(cJSON *array, cJSON *item) {
    if (!array || !item) return;
    add_item_to_end(array, item);
}

void cJSON_AddItemToObject(cJSON *object, const char *string, cJSON *item) {
    if (!object || !string || !item) return;
    if (item->string) {
        free(item->string);
    }
    item->string = strdup(string);
    add_item_to_end(object, item);
}

cJSON *cJSON_AddObjectToObject(cJSON *object, const char *string) {
    cJSON *obj = cJSON_CreateObject();
    if (!obj) return NULL;
    cJSON_AddItemToObject(object, string, obj);
    return obj;
}

cJSON *cJSON_GetObjectItem(const cJSON *object, const char *string) {
    if (!object || !string || !object->child) return NULL;
    cJSON *child = object->child;
    while (child) {
        if (child->string && strcmp(child->string, string) == 0) {
            return child;
        }
        child = child->next;
    }
    return NULL;
}

int cJSON_GetArraySize(const cJSON *array) {
    if (!array || !array->child) return 0;
    int count = 0;
    cJSON *child = array->child;
    while (child) {
        count++;
        child = child->next;
    }
    return count;
}

cJSON *cJSON_GetArrayItem(const cJSON *array, int index) {
    if (!array || index < 0) return NULL;
    cJSON *child = array->child;
    int i = 0;
    while (child) {
        if (i == index) return child;
        i++;
        child = child->next;
    }
    return NULL;
}

static void replace_item_in_parent(cJSON *parent, cJSON *item, cJSON *replacement) {
    if (!parent || !item || !replacement) return;
    replacement->prev = item->prev;
    replacement->next = item->next;
    if (item->prev) {
        item->prev->next = replacement;
    } else {
        parent->child = replacement;
    }
    if (item->next) {
        item->next->prev = replacement;
    }
    item->prev = NULL;
    item->next = NULL;
    cJSON_Delete(item);
}

void cJSON_ReplaceItemInObject(cJSON *object, const char *string, cJSON *newitem) {
    if (!object || !string || !newitem) return;
    cJSON *child = object->child;
    while (child) {
        if (child->string && strcmp(child->string, string) == 0) {
            if (newitem->string) {
                free(newitem->string);
            }
            newitem->string = strdup(string);
            replace_item_in_parent(object, child, newitem);
            return;
        }
        child = child->next;
    }
    cJSON_AddItemToObject(object, string, newitem);
}

void cJSON_SetNumberValue(cJSON *object, double num) {
    if (!object) return;
    object->type = cJSON_Number;
    object->valuedouble = num;
    object->valueint = (int)num;
}

cJSON *cJSON_AddStringToObject(cJSON *object, const char *name, const char *string) {
    cJSON *item = cJSON_CreateString(string);
    if (!item) return NULL;
    cJSON_AddItemToObject(object, name, item);
    return item;
}

cJSON *cJSON_AddNumberToObject(cJSON *object, const char *name, double number) {
    cJSON *item = cJSON_CreateNumber(number);
    if (!item) return NULL;
    cJSON_AddItemToObject(object, name, item);
    return item;
}

cJSON *cJSON_AddBoolToObject(cJSON *object, const char *name, int boolean) {
    cJSON *item = cjson_new_item();
    if (!item) return NULL;
    item->type = boolean ? cJSON_True : cJSON_False;
    item->valueint = boolean ? 1 : 0;
    cJSON_AddItemToObject(object, name, item);
    return item;
}

cJSON *cJSON_AddNullToObject(cJSON *object, const char *name) {
    cJSON *item = cjson_new_item();
    if (!item) return NULL;
    item->type = cJSON_NULL;
    cJSON_AddItemToObject(object, name, item);
    return item;
}
