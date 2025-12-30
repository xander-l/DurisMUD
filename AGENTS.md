# Agent Notes

- Network access to GitHub raw/tarball endpoints is blocked (curl returns HTTP 403 via CONNECT tunnel), so vendoring third-party sources via GitHub downloads is not possible in this environment.
- `apt-get update` fails with HTTP 403 via the proxy, so installing system packages (e.g., libcjson-dev) is not possible in this environment.
