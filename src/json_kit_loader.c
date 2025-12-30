#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "json_kit_loader.h"
#include "json_utils.h"
#include "db.h"
#include "utils.h"
#include "comm.h"

/* Externs from the codebase */
extern const struct race_names race_names_table[];
extern const struct class_names class_names_table[];
extern int flag2idx(int);

/* The original static function in nanny.c */
static void LoadNewbyShit(P_char ch, int *items);

static cJSON *root_kits = NULL;
static int json_load_attempted = 0;
static char active_path[256] = {0};

/**
 * Internal helper to save the current in-memory JSON structure back to the disk.
 * Uses the 'active_path' identified during the initial load or reload.
 * 
 * @return 1 on successful write, 0 on failure (e.g., file permissions).
 */
static int json_kits_save(void) {
    if (!root_kits || !active_path[0]) return 0;
    char *out = cJSON_Print(root_kits);
    FILE *f = fopen(active_path, "w");
    if (!f) {
        if (out) free(out);
        return 0;
    }
    fputs(out, f);
    fclose(f);
    free(out);
    return 1;
}

/**
 * Recursively traverses the JSON tree to find and replace item VNUMs.
 * 
 * @param node   The current cJSON node being inspected.
 * @param old_v  The VNUM to find.
 * @param new_v  The replacement VNUM.
 * @return       The total number of replacements made in this branch.
 */
static int recursive_item_replace(cJSON *node, int old_v, int new_v) {
    int count = 0;
    if (!node) return 0;
    
    if (cJSON_IsArray(node)) {
        cJSON *item;
        cJSON_ArrayForEach(item, node) {
            if (cJSON_IsNumber(item)) {
                if (item->valueint == old_v) {
                    cJSON_SetNumberValue(item, (double)new_v);
                    count++;
                }
            } else {
                count += recursive_item_replace(item, old_v, new_v);
            }
        }
    } else if (cJSON_IsObject(node)) {
        cJSON *item;
        cJSON_ArrayForEach(item, node) {
            count += recursive_item_replace(item, old_v, new_v);
        }
    }
    return count;
}

/**
 * Internal loader that handles file discovery across multiple relative paths.
 * Identifies the correct 'newbie_kits.json' and parses it into 'root_kits'.
 * 
 * @return 1 on success, 0 on total failure to find/parse the file.
 */
static int load_kit_file() {
    if (json_load_attempted) return (root_kits != NULL);
    json_load_attempted = 1;

    /* Paths are ordered by priority: Local -> Repo Relative -> System Absolute */
    const char *paths[] = {
        "lib/etc/newbie_kits.json",
        "../lib/etc/newbie_kits.json",
        "/usr/local/games/duris/lib/etc/newbie_kits.json"
    };

    FILE *f = NULL;
    for (int i = 0; i < 3; i++) {
        f = fopen(paths[i], "rb");
        if (f) {
            strncpy(active_path, paths[i], 255);
            break;
        }
    }
    
    if (!f) {
        logit(LOG_DEBUG, "JSON Kits: Could not find newbie_kits.json in standard paths.");
        return 0;
    }

    fseek(f, 0, SEEK_END);
    long len = ftell(f);
    fseek(f, 0, SEEK_SET);
    
    if (len <= 0) {
        fclose(f);
        return 0;
    }

    char *data = (char *)malloc(len + 1);
    size_t read_len = fread(data, 1, len, f);
    fclose(f);
    data[read_len] = '\0';

    root_kits = cJSON_Parse(data);
    free(data);

    if (!root_kits) {
        logit(LOG_DEBUG, "JSON Kits: Error parsing newbie_kits.json: %s", cJSON_GetErrorPtr());
        return 0;
    }

    logit(LOG_DEBUG, "JSON Kits: Successfully loaded config.");
    return 1;
}

/**
 * Main integration point for character creation (nanny.c).
 * Sequentially loads: 
 * 1. Global items (bandages)
 * 2. Room items (newbie note)
 * 3. Race baseline kits (good/evil/standard)
 * 4. Class-specific kits for that race.
 * 5. Global class kits (optional fallbacks like Blighter).
 * 
 * @param ch The character to equip.
 * @return 1 if any items were loaded, 0 to trigger hardcoded fallback in nanny.c.
 */
int try_load_json_kits(P_char ch) {
    if (!load_kit_file()) return 0;

    int success = 0;

    /* 1. Load Universal Start Items (e.g. Bandages) */
    cJSON *start_items = cJSON_GetObjectItem(root_kits, "start_items");
    if (start_items && cJSON_IsArray(start_items)) {
        int count = cJSON_GetArraySize(start_items);
        if (count > 0) {
            int *items = (int *)malloc(sizeof(int) * (count + 1));
            for (int i = 0; i < count; i++) {
                items[i] = cJSON_GetArrayItem(start_items, i)->valueint;
            }
            items[count] = -1;
            LoadNewbyShit(ch, items);
            free(items);
            success = 1;
        }
    }

    /* 2. Load Room-Specific items (e.g. The Note) */
    cJSON *room_items = cJSON_GetObjectItem(root_kits, "room_items");
    if (room_items && cJSON_IsArray(room_items)) {
        int i;
        for (i = 0; i < cJSON_GetArraySize(room_items); i++) {
            cJSON *entry = cJSON_GetArrayItem(room_items, i);
            int room_num = json_get_int(entry, "room", -1);
            if (room_num != -1 && world[ch->in_room].number == room_num) {
                cJSON *items_json = cJSON_GetObjectItem(entry, "items");
                if (items_json && cJSON_IsArray(items_json)) {
                    int count = cJSON_GetArraySize(items_json);
                    int *items = (int *)malloc(sizeof(int) * (count + 1));
                    for (int j = 0; j < count; j++) {
                        items[j] = cJSON_GetArrayItem(items_json, j)->valueint;
                    }
                    items[count] = -1;
                    LoadNewbyShit(ch, items);
                    free(items);
                    success = 1;
                }
            }
        }
    }

    /* 3. Load Race kits */
    const char *r_name = race_names_table[(int)GET_RACE(ch)].normal;
    cJSON *races = cJSON_GetObjectItem(root_kits, "races");
    cJSON *race_entry = races ? cJSON_GetObjectItem(races, r_name) : NULL;

    if (race_entry) {
        /* Align-specific or Default Basics */
        cJSON *items_json = NULL;
        if (GET_ALIGNMENT(ch) >= 0) {
            items_json = cJSON_GetObjectItem(race_entry, "basic_good");
        } else {
            items_json = cJSON_GetObjectItem(race_entry, "basic_evil");
        }
        
        if (!items_json) {
            items_json = cJSON_GetObjectItem(race_entry, "basic");
        }

        if (items_json && cJSON_IsArray(items_json)) {
            int count = cJSON_GetArraySize(items_json);
            if (count > 0) {
               int *items = (int *)malloc(sizeof(int) * (count + 1));
               for (int i = 0; i < count; i++) {
                   items[i] = cJSON_GetArrayItem(items_json, i)->valueint;
               }
               items[count] = -1;
               LoadNewbyShit(ch, items);
               free(items);
               success = 1;
            }
        }

        /* Class-Specific items within Race */
        cJSON *classes = cJSON_GetObjectItem(race_entry, "classes");
        if (classes) {
            const char *c_name = class_names_table[flag2idx(ch->player.m_class)].normal;
            cJSON *class_kit = cJSON_GetObjectItem(classes, c_name);
            if (class_kit && cJSON_IsArray(class_kit)) {
                int count = cJSON_GetArraySize(class_kit);
                if (count > 0) {
                    int *items = (int *)malloc(sizeof(int) * (count + 1));
                    for (int i = 0; i < count; i++) {
                        items[i] = cJSON_GetArrayItem(class_kit, i)->valueint;
                    }
                    items[count] = -1;
                    LoadNewbyShit(ch, items);
                    free(items);
                    success = 1;
                }
            }
        }
    }

    /* 4. Load Global Class kits (e.g. Blighter) */
    cJSON *global_classes = cJSON_GetObjectItem(root_kits, "global_classes");
    if (global_classes) {
        const char *c_name = class_names_table[flag2idx(ch->player.m_class)].normal;
        cJSON *global_class_kit = cJSON_GetObjectItem(global_classes, c_name);
        if (global_class_kit && cJSON_IsArray(global_class_kit)) {
            int count = cJSON_GetArraySize(global_class_kit);
            if (count > 0) {
                int *items = (int *)malloc(sizeof(int) * (count + 1));
                for (int i = 0; i < count; i++) {
                   items[i] = cJSON_GetArrayItem(global_class_kit, i)->valueint;
                }
                items[count] = -1;
                LoadNewbyShit(ch, items);
                free(items);
                success = 1;
            }
        }
    }

    return success;
}

/** 
 * API: Returns full JSON string. 
 * Note: Caller must free the returned string via cJSON_free.
 */
char *json_kits_get_all(void) {
    if (!load_kit_file()) return NULL;
    return cJSON_Print(root_kits);
}

/** 
 * API: Returns JSON segment for one race. 
 */
char *json_kits_get_race(const char *race) {
    if (!load_kit_file()) return NULL;
    cJSON *races = cJSON_GetObjectItem(root_kits, "races");
    if (!races) return NULL;
    cJSON *r_entry = cJSON_GetObjectItem(races, race);
    if (!r_entry) return NULL;
    return cJSON_Print(r_entry);
}

/** 
 * API: Hot-reloads the configuration file from disk.
 */
int json_kits_reload(void) {
    if (root_kits) {
        cJSON_Delete(root_kits);
        root_kits = NULL;
    }
    json_load_attempted = 0;
    int result = load_kit_file();
    if (result) {
        logit(LOG_DEBUG, "JSON Kits: Config reloaded successfully.");
    }
    return result;
}

/** 
 * API: Boolean check for kit existence.
 */
int json_kits_has_kit(const char *race, const char *cls) {
    if (!load_kit_file()) return 0;
    cJSON *races = cJSON_GetObjectItem(root_kits, "races");
    if (!races) return 0;
    cJSON *r_entry = cJSON_GetObjectItem(races, race);
    if (!r_entry) return 0;
    
    // Check race-specific classes
    cJSON *classes = cJSON_GetObjectItem(r_entry, "classes");
    if (classes && cJSON_GetObjectItem(classes, cls)) return 1;

    // Check global classes
    cJSON *globals = cJSON_GetObjectItem(root_kits, "global_classes");
    if (globals && cJSON_GetObjectItem(globals, cls)) return 1;

    return 0;
}

/** 
 * API: Surgical item search-and-replace across all kits.
 * Automatically saves on change.
 */
int json_kits_item_swap(int old_vnum, int new_vnum) {
    if (!load_kit_file()) return -1;
    
    int matches = recursive_item_replace(root_kits, old_vnum, new_vnum);
    
    if (matches > 0) {
        if (json_kits_save()) {
            logit(LOG_DEBUG, "JSON Kits: Swapped %d occurrences of %d with %d and saved.", 
                  matches, old_vnum, new_vnum);
        } else {
            logit(LOG_DEBUG, "JSON Kits: Swapped %d items in memory, but FAILED to save to disk.", matches);
        }
    }
    
    return matches;
}

/** 
 * API: Adds or updates a kit entry.
 * Automates parent object creation (non-destructive expansion).
 */
int json_kits_update_kit(const char *race, const char *cls, const char *align, int *vnums) {
    if (!load_kit_file()) return 0;
    if (!vnums) return 0;

    cJSON *target_parent = NULL;
    const char *key = NULL;

    if (race && *race) {
        // Race-based kit branch
        cJSON *races = cJSON_GetObjectItem(root_kits, "races");
        if (!races) {
            races = cJSON_AddObjectToObject(root_kits, "races");
        }
        
        cJSON *r_entry = cJSON_GetObjectItem(races, race);
        if (!r_entry) {
            r_entry = cJSON_AddObjectToObject(races, race);
        }

        if (cls && *cls) {
            // Class within specific race
            cJSON *classes = cJSON_GetObjectItem(r_entry, "classes");
            if (!classes) {
                classes = cJSON_AddObjectToObject(r_entry, "classes");
            }
            target_parent = classes;
            key = cls;
        } else {
            // Race-wide basic branch
            target_parent = r_entry;
            if (align && !strcasecmp(align, "good")) key = "basic_good";
            else if (align && !strcasecmp(align, "evil")) key = "basic_evil";
            else key = "basic";
        }
    } else if (cls && *cls) {
        // Global class kit branch
        cJSON *globals = cJSON_GetObjectItem(root_kits, "global_classes");
        if (!globals) {
            globals = cJSON_AddObjectToObject(root_kits, "global_classes");
        }
        target_parent = globals;
        key = cls;
    }

    if (!target_parent || !key) return 0;

    // Create the new array
    cJSON *new_arr = cJSON_CreateArray();
    for (int i = 0; vnums[i] != -1; i++) {
        cJSON_AddItemToArray(new_arr, cJSON_CreateNumber((double)vnums[i]));
    }

    // Replace existing or append new
    if (cJSON_GetObjectItem(target_parent, key)) {
        cJSON_ReplaceItemInObject(target_parent, key, new_arr);
    } else {
        cJSON_AddItemToObject(target_parent, key, new_arr);
    }

    return json_kits_save();
}
