#!/bin/bash
# ==============================================================================
# DurisMUD Docker Entrypoint
#
# Starts MySQL, handles fresh volume initialization, then launches the MUD.
# ==============================================================================

set -e

echo "=== DurisMUD Docker Entrypoint ==="

# ---- MySQL data directory check ---------------------------------------------
if [ ! -d /var/lib/mysql/mysql ]; then
    echo "MySQL data directory is empty (fresh volume). Initializing..."
    chown -R mysql:mysql /var/lib/mysql
    mysqld --initialize-insecure --user=mysql
    mysqld_safe --skip-grant-tables &
    sleep 5

    echo "Creating databases and users..."
    mysql -u root -e "FLUSH PRIVILEGES;"
    mysql -u root -e "CREATE DATABASE IF NOT EXISTS duris_dev;"
    mysql -u root -e "CREATE DATABASE IF NOT EXISTS duris;"
    mysql -u root -e "CREATE USER IF NOT EXISTS 'duris'@'localhost' IDENTIFIED BY 'duris';"
    mysql -u root -e "CREATE USER IF NOT EXISTS 'duris'@'127.0.0.1' IDENTIFIED BY 'duris';"
    mysql -u root -e "CREATE USER IF NOT EXISTS 'duris'@'%' IDENTIFIED BY 'duris';"
    mysql -u root -e "GRANT ALL PRIVILEGES ON duris_dev.* TO 'duris'@'localhost' WITH GRANT OPTION;"
    mysql -u root -e "GRANT ALL PRIVILEGES ON duris_dev.* TO 'duris'@'127.0.0.1' WITH GRANT OPTION;"
    mysql -u root -e "GRANT ALL PRIVILEGES ON duris_dev.* TO 'duris'@'%' WITH GRANT OPTION;"
    mysql -u root -e "GRANT ALL PRIVILEGES ON duris.* TO 'duris'@'localhost' WITH GRANT OPTION;"
    mysql -u root -e "GRANT ALL PRIVILEGES ON duris.* TO 'duris'@'127.0.0.1' WITH GRANT OPTION;"
    mysql -u root -e "GRANT ALL PRIVILEGES ON duris.* TO 'duris'@'%' WITH GRANT OPTION;"
    mysql -u root -e "FLUSH PRIVILEGES;"

    echo "Importing base schema..."
    mysql -u root duris_dev < /duris/src/duris.sql
    mysql -u root duris < /duris/src/duris.sql
    mysql -u root duris_dev < /duris/migrations/run_this_one.sql
    mysql -u root duris < /duris/migrations/run_this_one.sql

    # ---- Incremental migrations ----------------------------------------
    # When MIGRATION_AUTO_RUNNER is defined in CFLAGS, the MUD binary
    # handles all incremental migrations during boot (via auto-runner).
    # When the flag is off, uncomment the lines below for shell-level
    # migration execution.
    # (mysql -u root duris_dev < /duris/migrations/schema_migration_v16_item_events.sql || true)
    # (mysql -u root duris < /duris/migrations/schema_migration_v16_item_events.sql || true)

    killall mysqld || true
    sleep 2
    rm -f /var/run/mysqld/mysqld.sock.lock /var/run/mysqld/mysqld.sock
fi

# ---- Start MySQL ------------------------------------------------------------
rm -f /var/run/mysqld/mysqld.sock.lock /var/run/mysqld/mysqld.sock
echo "Starting MySQL..."
mysqld_safe &

echo "Waiting for MySQL to be ready..."
for i in $(seq 1 30); do
    if mysqladmin ping -h 127.0.0.1 --silent 2>/dev/null; then
        echo "MySQL is ready."
        break
    fi
    if [ "$i" -eq 30 ]; then
        echo "ERROR: MySQL failed to start within 30 seconds."
        exit 1
    fi
    sleep 1
done

# Verify connectivity
mysql -h 127.0.0.1 -u duris -pduris duris_dev -e "SELECT 1;" >/dev/null 2>&1 || {
    echo "WARNING: Re-creating duris user..."
    mysql -u root -e "CREATE USER IF NOT EXISTS 'duris'@'localhost' IDENTIFIED BY 'duris';" 2>/dev/null || true
    mysql -u root -e "CREATE USER IF NOT EXISTS 'duris'@'127.0.0.1' IDENTIFIED BY 'duris';" 2>/dev/null || true
    mysql -u root -e "CREATE USER IF NOT EXISTS 'duris'@'%' IDENTIFIED BY 'duris';" 2>/dev/null || true
    mysql -u root -e "GRANT ALL PRIVILEGES ON duris_dev.* TO 'duris'@'localhost' WITH GRANT OPTION;" 2>/dev/null || true
    mysql -u root -e "GRANT ALL PRIVILEGES ON duris_dev.* TO 'duris'@'127.0.0.1' WITH GRANT OPTION;" 2>/dev/null || true
    mysql -u root -e "GRANT ALL PRIVILEGES ON duris_dev.* TO 'duris'@'%' WITH GRANT OPTION;" 2>/dev/null || true
    mysql -u root -e "GRANT ALL PRIVILEGES ON duris.* TO 'duris'@'localhost' WITH GRANT OPTION;" 2>/dev/null || true
    mysql -u root -e "GRANT ALL PRIVILEGES ON duris.* TO 'duris'@'127.0.0.1' WITH GRANT OPTION;" 2>/dev/null || true
    mysql -u root -e "GRANT ALL PRIVILEGES ON duris.* TO 'duris'@'%' WITH GRANT OPTION;" 2>/dev/null || true
    mysql -u root -e "FLUSH PRIVILEGES;" 2>/dev/null || true
}

# ---- Start MUD via cycle_mud.sh (handles migrations, area build, reboot loop) ------
echo "Starting DurisMUD via cycle_mud.sh..."

cleanup() {
    echo "Shutting down DurisMUD..."
    kill $(jobs -p) 2>/dev/null || true
    wait
}
trap cleanup SIGTERM SIGINT

exec ./cycle_mud.sh
