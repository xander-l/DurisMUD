# Specialization Module Guide

This repository is compiled as C++20 even though most implementation files use
`.c` names. The specialization module follows that convention while exposing a
small, C++-friendly API to the rest of the server.

## Ownership

| Concern | Owner |
|---|---|
| Specialization names | `src/specialization_data.c` |
| Legacy race/class/spec eligibility | `src/specialization_data.c` |
| Name and existence queries | `src/specialization_data.c` |
| Player specialization predicates | `src/specialization_data.c` |
| Specialization commands and flow | `src/specializations.c` |
| Gameplay effects | Existing combat, spell, skill, and class subsystems |

Gameplay code should not access `specdata` directly. Include
`specialization_data.h` and use the query functions instead.

## Index contract

- Class masks such as `CLASS_WARRIOR` are converted with `flag2idx()`.
- Class-table indexes are zero-based.
- Specialization IDs are one-based within each class.
- `MAX_SPEC` is the table width; valid table indexes are `0 .. MAX_SPEC - 1`.
- Index `0` is the empty/no-specialization slot.
- `SPEC_ALL` is only a sentinel in the legacy eligibility table. It is not a
  player specialization ID.
- Invalid name indexes return an empty string.
- Null characters return `false` from the state predicates.

## Adding a specialization safely

Use this sequence for a new specialization:

1. Add or reserve the class-local `SPEC_*` ID in `src/structs.h`.
2. Add its display name in the matching row of `specdata` in
   `src/specialization_data.c`.
3. Add the permitted race/class/spec rows to the legacy eligibility table in
   the same file. Preserve the existing policy; do not silently broaden or
   narrow eligibility in a refactor.
4. Add or update the relevant skill `rlevel`/`maxlearn` entries.
5. Implement gameplay effects in the appropriate subsystem, using
   `specialization_matches()` or `GET_SPEC()` rather than reading
   `player.spec` or `specdata` directly.
6. Add a focused regression case for the new name, eligibility, skill gate,
   and gameplay effect.
7. Run the checks and clean build:

```bash
python3 tools/check_specialization_modularity.py
cd src
make clean
make -j"$(nproc)"
```

## Copy-friendly implementation pattern

For a new effect, copy this shape and change only the class/spec/behavior:

```cpp
if (specialization_matches(ch, CLASS_WARRIOR, SPEC_GUARDIAN))
{
    // Existing behavior for this specialization.
}
```

For display or lookup code:

```cpp
const char *name = specialization_name(ch->player.m_class, ch->player.spec);
```

Do not add a new `extern specdata` declaration or duplicate an eligibility
array in another subsystem.

## Current scope

This module is deliberately behavior-preserving. It centralizes specialization
data and state/eligibility queries, but it does not yet convert every unique
combat or spell effect into a declarative bonus table. That larger behavioral
rewrite should be a separate, tested change rather than being hidden inside the
initial modularization.
