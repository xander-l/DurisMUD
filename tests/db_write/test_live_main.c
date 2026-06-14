/* ===================================================================
 * test_live_main.c — Entry point for live DB integration tests.
 *
 * Connects to a real MySQL instance, runs the three live test suites:
 *   1. Frag & Item Transfer (frag_leaderboard, account_characters)
 *   2. Locker Stress (lockers, locker_chests, locker_items)
 *   3. Latency Guard (latency_trace, timing measurements)
 *
 * Build: make -f Makefile.live
 * Run:   ./test_live_db [test_name]
 *
 * Env vars for MySQL connection (same as production):
 *   DB_HOST, DB_USER, DB_PASSWD, DB_NAME, DB_PORT
 * =================================================================== */

#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <mysql.h>

/* Live test suite declarations */
int test_live_frags_run(MYSQL *db);
int test_live_locker_run(MYSQL *db);
int test_live_latency_run(MYSQL *db);
int test_live_accounts_run(MYSQL *db);

/* ------------------------------------------------------------------ */
/*  MySQL connection helpers                                          */
/* ------------------------------------------------------------------ */

static const char *env_or(const char *name, const char *fallback)
{
    const char *val = getenv(name);
    return (val && *val) ? val : fallback;
}

static MYSQL *connect_mysql(void)
{
    MYSQL *db = mysql_init(NULL);
    if (!db) {
        fprintf(stderr, "mysql_init failed\n");
        return NULL;
    }

    const char *host   = env_or("DB_HOST",   "127.0.0.1");
    const char *user   = env_or("DB_USER",   "duris");
    const char *passwd = env_or("DB_PASSWD", "duris");
    const char *dbname = env_or("DB_NAME",   "duris_test");
    int         port   = atoi(env_or("DB_PORT", "3306"));

    printf("Connecting to MySQL: %s@%s:%d/%s ... ",
           user, host, port, dbname);
    fflush(stdout);

    if (!mysql_real_connect(db, host, user, passwd, dbname, port, NULL, 0)) {
        fprintf(stderr, "\nMySQL connect failed: %s\n", mysql_error(db));
        mysql_close(db);
        return NULL;
    }

    printf("connected (server=%s)\n", mysql_get_server_info(db));
    return db;
}

/* ------------------------------------------------------------------ */
/*  Test runner                                                        */
/* ------------------------------------------------------------------ */

typedef struct {
    const char *name;
    int (*runner)(MYSQL *db);
} suite_entry;

static suite_entry g_suites[] = {
    { "frags",    test_live_frags_run },
    { "locker",   test_live_locker_run },
    { "latency",  test_live_latency_run },
    { "accounts", test_live_accounts_run },
    { NULL, NULL },
};

int main(int argc, char **argv)
{
    printf("=== Duris Live DB Integration Tests ===\n\n");

    MYSQL *db = connect_mysql();
    if (!db) {
        fprintf(stderr, "\nFATAL: Could not connect to MySQL.\n");
        fprintf(stderr, "Set DB_HOST, DB_USER, DB_PASSWD, DB_NAME, DB_PORT env vars.\n");
        return 1;
    }

    /* Create test schema if tables don't exist */
    printf("Initializing test schema...\n");
    const char *schema_path = "schema.sql";
    /* Try alternate paths */
    FILE *sf = fopen(schema_path, "r");
    if (!sf) sf = fopen("../../tests/db_write/schema.sql", "r");
    if (!sf) sf = fopen("tests/db_write/schema.sql", "r");

    if (sf) {
        fseek(sf, 0, SEEK_END);
        long sz = ftell(sf);
        fseek(sf, 0, SEEK_SET);
        char *sql = (char *)malloc(sz + 1);
        if (sql) {
            size_t rd = fread(sql, 1, sz, sf);
            if (rd != (size_t)sz) fprintf(stderr, "Schema read truncated\n");
            sql[sz] = '\0';
        fclose(sf);

        /* Execute schema by splitting on semicolons (mysql_query
         * does not support multi-statement strings). */
        char *saveptr = NULL;
        char *stmt    = strtok_r(sql, ";", &saveptr);
        while (stmt) {
            /* Skip blank statements */
            while (*stmt == ' ' || *stmt == '\n' || *stmt == '\r') stmt++;
            if (*stmt && strncmp(stmt, "--", 2) != 0) {
                if (mysql_query(db, stmt)) {
                    /* CREATE IF NOT EXISTS — duplicate is non-fatal */
                    if (strstr(mysql_error(db), "already exists") == NULL)
                        fprintf(stderr, "Schema init warning: %s\n", mysql_error(db));
                }
            }
            stmt = strtok_r(NULL, ";", &saveptr);
        }
        free(sql);
        }
        printf("Schema ready.\n");
    } else {
        printf("schema.sql not found — assuming tables already exist.\n");
    }

    int total_fail = 0;

    /* Run specific suite if requested */
    if (argc > 1) {
        for (int s = 0; g_suites[s].name; s++) {
            if (strcmp(argv[1], g_suites[s].name) == 0) {
                total_fail += g_suites[s].runner(db);
                goto done;
            }
        }
        fprintf(stderr, "Unknown suite: %s\n", argv[1]);
        fprintf(stderr, "Available: frags, locker, latency, all\n");
        mysql_close(db);
        return 1;
    }

    /* Run all suites */
    for (int s = 0; g_suites[s].name; s++) {
        total_fail += g_suites[s].runner(db);
    }

done:
    mysql_close(db);

    printf("\n========================================\n");
    if (total_fail == 0)
        printf("  ALL LIVE TESTS PASSED\n");
    else
        printf("  %d LIVE TEST FAILURE(S)\n", total_fail);
    printf("========================================\n");

    return (total_fail > 0) ? 1 : 0;
}
