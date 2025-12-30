# Equipment Kit Dashboard Integration Plan

This plan outlines how a developer can integrate the new JSON-based equipment system into a web-based dashboard or external admin tool.

## 1. Core API Functions
The MUD now provides a C API in `json_kit_loader.c` designed for programmatic access:

*   **`json_kits_get_all()`**: Returns the entire `newbie_kits.json` structure as a formatted string. Ideal for initial sync or full-table views.
*   **`json_kits_reload()`**: Forces the MUD to clear its memory and re-parse the JSON file. This allows for **instant hot-swapping** of equipment without rebooting the server.
*   **`json_kits_get_race(name)`**: Returns data for a specific race. Useful for "drill-down" views.

## 2. Integration Archetypes

### A. The "Auditor" (Read-Only)
A dashboard that checks the health of the equipment system.
1.  **Sync**: Call `json_kits_get_all()`.
2.  **Compare**: Iterate through all MUD races (via `race_names_table`) and highlight any that are missing a "basic" kit.
3.  **VNUM Check**: Scan the `basic` and `classes` arrays and check if those item IDs exist in the MUD's world database.

### B. The "Live Editor" (Read-Write)
A UI for admins to update kits on the fly.
1.  **Edit**: Admin uses a web GUI to move items in/out of a kit.
2.  **Save**: The dashboard writes the updated JSON directly to `lib/etc/newbie_kits.json`.
3.  **Deploy**: The dashboard sends a signal to the MUD (via a socket or specialized command) to call `json_kits_reload()`.
4.  **Result**: The next character created immediately receives the new equipment.

## 3. Recommended Technical Approach
If building a web dashboard, you should:
1.  Expose the `json_kits_*` functions via a MUD command (e.g., `wizkit`) that can output raw JSON or be triggered via a Telnet/WebSocket administrative connection.
2.  Use the `json_kits_reload()` function to implement a "Publish Changes" button in the UI.
