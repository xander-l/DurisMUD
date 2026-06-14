# ==============================================================================
# DurisMUD Docker Build
#
# Canonical build flow:
#   1. Build area compiler tools from C source (areas/src/Makefile)
#   2. Run area build pipeline (areas/m_slow → world.* files)
#   3. Build main MUD binary (src/Makefile → dms)
#   4. Start MySQL, then launch the MUD
#
# Build:   docker build -t durismud .
# Run:     docker compose up --build
# ==============================================================================

FROM ubuntu:24.04

ENV DEBIAN_FRONTEND=noninteractive

# ---- System packages --------------------------------------------------------
RUN apt-get update && apt-get install -y \
        build-essential \
        g++ \
        make \
        mysql-server \
        libmysqlclient-dev \
        libxml2-dev \
        zlib1g-dev \
        libgnutls28-dev \
        libssl-dev \
        libcjson-dev \
        libhiredis-dev \
        libcrypt-dev \
        gawk \
        dos2unix \
        binutils \
        curl \
        netcat-openbsd \
    && rm -rf /var/lib/apt/lists/*

RUN mkdir -p /var/run/mysqld && chown mysql:mysql /var/run/mysqld

# ---- Copy source ------------------------------------------------------------
WORKDIR /duris
COPY . /duris

# ---- MySQL setup ------------------------------------------------------------
RUN mysqld_safe & \
    sleep 5 && \
    mysql -u root -e "CREATE DATABASE IF NOT EXISTS duris_dev;" && \
    mysql -u root -e "CREATE DATABASE IF NOT EXISTS duris;" && \
    mysql -u root -e "CREATE USER IF NOT EXISTS 'duris'@'localhost' IDENTIFIED BY 'duris';" && \
    mysql -u root -e "CREATE USER IF NOT EXISTS 'duris'@'127.0.0.1' IDENTIFIED BY 'duris';" && \
    mysql -u root -e "CREATE USER IF NOT EXISTS 'duris'@'%' IDENTIFIED BY 'duris';" && \
    mysql -u root -e "GRANT ALL PRIVILEGES ON duris_dev.* TO 'duris'@'localhost' WITH GRANT OPTION;" && \
    mysql -u root -e "GRANT ALL PRIVILEGES ON duris_dev.* TO 'duris'@'127.0.0.1' WITH GRANT OPTION;" && \
    mysql -u root -e "GRANT ALL PRIVILEGES ON duris_dev.* TO 'duris'@'%' WITH GRANT OPTION;" && \
    mysql -u root -e "GRANT ALL PRIVILEGES ON duris.* TO 'duris'@'localhost' WITH GRANT OPTION;" && \
    mysql -u root -e "GRANT ALL PRIVILEGES ON duris.* TO 'duris'@'127.0.0.1' WITH GRANT OPTION;" && \
    mysql -u root -e "GRANT ALL PRIVILEGES ON duris.* TO 'duris'@'%' WITH GRANT OPTION;" && \
    mysql -u root -e "FLUSH PRIVILEGES;" && \
    mysql -u root duris_dev < /duris/src/duris.sql && \
    mysql -u root duris < /duris/src/duris.sql && \
    mysql -u root duris_dev < /duris/migrations/run_this_one.sql && \
    mysql -u root duris < /duris/migrations/run_this_one.sql && \
    (mysql -u root duris_dev < /duris/sql/migrations/add_frag_leaderboard_tables.sql || true) && \
    (mysql -u root duris < /duris/sql/migrations/add_frag_leaderboard_tables.sql || true) && \
    (mysql -u root duris_dev < /duris/migrations/schema_migration_v16_item_events.sql || true) && \
    (mysql -u root duris < /duris/migrations/schema_migration_v16_item_events.sql || true) && \
    killall mysqld || true

# ---- Build area compiler tools FROM SOURCE (areas/src/Makefile) --------------
RUN cd /duris/areas/src && \
    make clean 2>/dev/null || true && \
    make

# ---- Run area build pipeline ------------------------------------------------
# Convert shell scripts from CRLF (Windows checkout) and make executable
RUN cd /duris/areas && \
    dos2unix m_slow m_quick make_all moveall make_lookup 2>/dev/null || true && \
    chmod +x m_slow m_quick make_all moveall make_lookup && \
    ./m_slow

# ---- Build the main MUD binary ----------------------------------------------
RUN cd /duris/src && \
    make clean 2>/dev/null || true && \
    make && \
    cp dms_new /duris/dms

# ---- SSL certificates -------------------------------------------------------
RUN cd /duris && \
    ln -sf localhost.crt duris.crt && \
    ln -sf localhost.key duris.key

# ---- Create required directories --------------------------------------------
RUN mkdir -p /duris/logs/log \
             /duris/logs/old-logs \
             /duris/logs/player-log \
             /duris/Players/Backup \
             /duris/db/Backup

# ---- Startup scripts --------------------------------------------------------
COPY entrypoint.sh /duris/entrypoint.sh
RUN chmod +x /duris/entrypoint.sh
COPY cycle_mud.sh /duris/cycle_mud.sh
RUN chmod +x /duris/cycle_mud.sh

EXPOSE 7777

WORKDIR /duris
CMD ["/duris/entrypoint.sh"]
