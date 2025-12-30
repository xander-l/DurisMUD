#include <errno.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "third_party/cjson/cJSON.h"
#include "json_kit_loader.h"
#include "utility.h"

void logit(const char *filename, const char *format, ...) {
    va_list args;

    /* TODO: Link against the production logger when a full test harness is available. */
    fprintf(stderr, "[%s] ", filename ? filename : "log");
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
    fprintf(stderr, "\n");
}

static int write_fixture(const char *path) {
    const char *json =
        "{"
        "\"start_items\":[1,2],"
        "\"room_items\":[{\"room\":100,\"items\":[3]}],"
        "\"races\":{"
            "\"Human\":{"
                "\"basic\":[10,11],"
                "\"classes\":{\"Warrior\":[20]}"
            "}"
        "},"
        "\"global_classes\":{\"Blighter\":[30]}"
        "}";

    FILE *file = fopen(path, "w");
    if (!file) {
        fprintf(stderr, "Failed to open fixture file %s: %s\n", path, strerror(errno));
        return 0;
    }
    if (fputs(json, file) == EOF) {
        fprintf(stderr, "Failed to write fixture file %s: %s\n", path, strerror(errno));
        fclose(file);
        return 0;
    }
    fclose(file);
    return 1;
}

static void cleanup_paths(const char *base_dir) {
    char path[512];
    snprintf(path, sizeof(path), "%s/lib/etc/newbie_kits.json", base_dir);
    unlink(path);
    snprintf(path, sizeof(path), "%s/lib/etc", base_dir);
    rmdir(path);
    snprintf(path, sizeof(path), "%s/lib", base_dir);
    rmdir(path);
    rmdir(base_dir);
}

static cJSON *parse_json_or_fail(const char *json_str, const char *label) {
    cJSON *root = cJSON_Parse(json_str);
    if (!root) {
        fprintf(stderr, "Failed to parse %s JSON: %s\n", label, cJSON_GetErrorPtr());
    }
    return root;
}

static int array_contains_value(cJSON *array, int value) {
    int count = cJSON_GetArraySize(array);
    for (int i = 0; i < count; i++) {
        cJSON *item = cJSON_GetArrayItem(array, i);
        if (item && cJSON_IsNumber(item) && item->valueint == value) {
            return 1;
        }
    }
    return 0;
}

int main(void) {
    char temp_template[] = "/tmp/json_kit_test_XXXXXX";
    char *temp_dir = mkdtemp(temp_template);
    if (!temp_dir) {
        fprintf(stderr, "Failed to create temp directory: %s\n", strerror(errno));
        return 1;
    }

    char lib_dir[512];
    char etc_dir[512];
    char json_path[512];
    snprintf(lib_dir, sizeof(lib_dir), "%s/lib", temp_dir);
    snprintf(etc_dir, sizeof(etc_dir), "%s/lib/etc", temp_dir);
    snprintf(json_path, sizeof(json_path), "%s/lib/etc/newbie_kits.json", temp_dir);

    if (mkdir(lib_dir, 0755) != 0) {
        fprintf(stderr, "Failed to create lib dir: %s\n", strerror(errno));
        cleanup_paths(temp_dir);
        return 1;
    }
    if (mkdir(etc_dir, 0755) != 0) {
        fprintf(stderr, "Failed to create etc dir: %s\n", strerror(errno));
        cleanup_paths(temp_dir);
        return 1;
    }
    if (!write_fixture(json_path)) {
        cleanup_paths(temp_dir);
        return 1;
    }

    if (chdir(temp_dir) != 0) {
        fprintf(stderr, "Failed to chdir to temp dir: %s\n", strerror(errno));
        cleanup_paths(temp_dir);
        return 1;
    }

    if (!json_kits_reload()) {
        fprintf(stderr, "json_kits_reload failed\n");
        cleanup_paths(temp_dir);
        return 1;
    }

    char *all_json = json_kits_get_all();
    if (!all_json) {
        fprintf(stderr, "json_kits_get_all returned NULL\n");
        cleanup_paths(temp_dir);
        return 1;
    }
    cJSON *all_root = parse_json_or_fail(all_json, "all");
    cJSON_free(all_json);
    if (!all_root) {
        cleanup_paths(temp_dir);
        return 1;
    }

    if (!cJSON_GetObjectItem(all_root, "start_items") ||
        !cJSON_GetObjectItem(all_root, "room_items") ||
        !cJSON_GetObjectItem(all_root, "races") ||
        !cJSON_GetObjectItem(all_root, "global_classes")) {
        fprintf(stderr, "Missing expected top-level keys\n");
        cJSON_Delete(all_root);
        cleanup_paths(temp_dir);
        return 1;
    }
    cJSON_Delete(all_root);

    char *race_json = json_kits_get_race("Human");
    if (!race_json) {
        fprintf(stderr, "json_kits_get_race returned NULL\n");
        cleanup_paths(temp_dir);
        return 1;
    }
    cJSON *race_root = parse_json_or_fail(race_json, "race");
    cJSON_free(race_json);
    if (!race_root) {
        cleanup_paths(temp_dir);
        return 1;
    }
    cJSON *basic = cJSON_GetObjectItem(race_root, "basic");
    cJSON *classes = cJSON_GetObjectItem(race_root, "classes");
    if (!basic || !classes) {
        fprintf(stderr, "Race JSON missing basic/classes\n");
        cJSON_Delete(race_root);
        cleanup_paths(temp_dir);
        return 1;
    }
    cJSON_Delete(race_root);

    if (!json_kits_has_kit("Human", "Warrior")) {
        fprintf(stderr, "Expected json_kits_has_kit to return true for Human/Warrior\n");
        cleanup_paths(temp_dir);
        return 1;
    }
    if (json_kits_has_kit("Human", "Mage")) {
        fprintf(stderr, "Expected json_kits_has_kit to return false for Human/Mage\n");
        cleanup_paths(temp_dir);
        return 1;
    }

    int swapped = json_kits_item_swap(10, 99);
    if (swapped != 1) {
        fprintf(stderr, "Expected json_kits_item_swap to swap 1 item, got %d\n", swapped);
        cleanup_paths(temp_dir);
        return 1;
    }
    if (!json_kits_reload()) {
        fprintf(stderr, "json_kits_reload failed after swap\n");
        cleanup_paths(temp_dir);
        return 1;
    }

    race_json = json_kits_get_race("Human");
    if (!race_json) {
        fprintf(stderr, "json_kits_get_race returned NULL after swap\n");
        cleanup_paths(temp_dir);
        return 1;
    }
    race_root = parse_json_or_fail(race_json, "race after swap");
    cJSON_free(race_json);
    if (!race_root) {
        cleanup_paths(temp_dir);
        return 1;
    }
    basic = cJSON_GetObjectItem(race_root, "basic");
    if (!basic || !array_contains_value(basic, 99) || array_contains_value(basic, 10)) {
        fprintf(stderr, "Swap did not persist in basic array\n");
        cJSON_Delete(race_root);
        cleanup_paths(temp_dir);
        return 1;
    }
    cJSON_Delete(race_root);

    int rogue_items[] = {40, 41, -1};
    if (!json_kits_update_kit("Human", "Rogue", NULL, rogue_items)) {
        fprintf(stderr, "json_kits_update_kit failed for Human/Rogue\n");
        cleanup_paths(temp_dir);
        return 1;
    }
    if (!json_kits_reload()) {
        fprintf(stderr, "json_kits_reload failed after update\n");
        cleanup_paths(temp_dir);
        return 1;
    }

    race_json = json_kits_get_race("Human");
    if (!race_json) {
        fprintf(stderr, "json_kits_get_race returned NULL after update\n");
        cleanup_paths(temp_dir);
        return 1;
    }
    race_root = parse_json_or_fail(race_json, "race after update");
    cJSON_free(race_json);
    if (!race_root) {
        cleanup_paths(temp_dir);
        return 1;
    }
    classes = cJSON_GetObjectItem(race_root, "classes");
    cJSON *rogue = classes ? cJSON_GetObjectItem(classes, "Rogue") : NULL;
    if (!rogue || !array_contains_value(rogue, 40) || !array_contains_value(rogue, 41)) {
        fprintf(stderr, "Updated Rogue kit missing expected items\n");
        cJSON_Delete(race_root);
        cleanup_paths(temp_dir);
        return 1;
    }
    cJSON_Delete(race_root);

    int blighter_items[] = {88, -1};
    if (!json_kits_update_kit(NULL, "Blighter", NULL, blighter_items)) {
        fprintf(stderr, "json_kits_update_kit failed for global Blighter\n");
        cleanup_paths(temp_dir);
        return 1;
    }
    if (!json_kits_reload()) {
        fprintf(stderr, "json_kits_reload failed after global update\n");
        cleanup_paths(temp_dir);
        return 1;
    }

    all_json = json_kits_get_all();
    if (!all_json) {
        fprintf(stderr, "json_kits_get_all returned NULL after global update\n");
        cleanup_paths(temp_dir);
        return 1;
    }
    all_root = parse_json_or_fail(all_json, "all after global update");
    cJSON_free(all_json);
    if (!all_root) {
        cleanup_paths(temp_dir);
        return 1;
    }
    cJSON *globals = cJSON_GetObjectItem(all_root, "global_classes");
    cJSON *blighter = globals ? cJSON_GetObjectItem(globals, "Blighter") : NULL;
    if (!blighter || !array_contains_value(blighter, 88)) {
        fprintf(stderr, "Global Blighter kit missing expected item\n");
        cJSON_Delete(all_root);
        cleanup_paths(temp_dir);
        return 1;
    }
    cJSON_Delete(all_root);

    cleanup_paths(temp_dir);
    return 0;
}
