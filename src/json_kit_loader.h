#ifndef JSON_KIT_LOADER_H
#define JSON_KIT_LOADER_H

#include "structs.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Attempts to load equipment kits from the JSON configuration for a new character.
 * 
 * @param ch  Pointer to the character receiving the equipment.
 * @return 1  Success: Kits were loaded and applied. Caller should skip hardcoded logic.
 * @return 0  Failure: File missing, parse error, or no matching data. Caller should fall back.
 */
int try_load_json_kits(struct char_data *ch);

/* Developer & Dashboard Integration API */

/**
 * Retrieves the entire equipment kit database as a formatted JSON string.
 * This is the primary sync point for dashboards.
 * 
 * @return  A dynamically allocated string (must be free'd by caller via cJSON_free) 
 *          containing the full JSON structure, or NULL on error.
 */
char *json_kits_get_all(void);

/**
 * Retrieves JSON data for a specific race.
 * 
 * @param race  The "normal" name of the race (e.g., "Human", "Mountain Dwarf").
 * @return      Formatting JSON string for that race segment, or NULL if not found.
 */
char *json_kits_get_race(const char *race);

/**
 * Hot-Reload: Clears internal cache and reloads the newbie_kits.json file from disk.
 * Allows for instant equipment updates without server reboots.
 * 
 * @return 1 on successful reload, 0 otherwise.
 */
int json_kits_reload(void);

/**
 * Checks if a kit is defined for a given race/class combination.
 * 
 * @param race  Race name string.
 * @param cls   Class name string.
 * @return 1 if a specific or global kit exists, 0 otherwise.
 */
int json_kits_has_kit(const char *race, const char *cls);

/**
 * Global Search-and-Replace: Replaces all occurrences of an item across the entire database.
 * 
 * @param old_vnum  The VNUM of the item to be replaced.
 * @param new_vnum  The VNUM of the replacement item.
 * @return          The number of occurrences swapped and saved to disk, or -1 on error.
 */
int json_kits_item_swap(int old_vnum, int new_vnum);

/**
 * Surgically adds or updates a specific kit entry in the JSON database.
 * Non-destructive: missing parent objects (races, etc.) are created automatically.
 * 
 * @param race   Race name (optional if global class kit).
 * @param cls    Class name (optional if race basics).
 * @param align  "good" or "evil" for alignment basics. NULL for standard basic.
 * @param vnums  -1 terminated array of item VNUMs.
 * @return       1 on success (saved to disk), 0 on failure.
 */
int json_kits_update_kit(const char *race, const char *cls, const char *align, int *vnums);

#ifdef __cplusplus
}
#endif

#endif
