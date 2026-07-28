#ifndef _SPECIALIZATION_DATA_H_
#define _SPECIALIZATION_DATA_H_

#include "structs.h"

#ifndef SPEC_ALL
#define SPEC_ALL 0
#endif

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Specialization IDs are one-based within each class.  The zero index is
 * reserved for "no specialization" and MAX_SPEC is the table width.
 * Class masks are converted to the same zero-based index used by specdata.
 */
const char *specialization_name(uint m_class, int spec_index);
const char *specialization_name_by_index(int class_index, int spec_index);
bool specialization_exists(uint m_class, int spec_index);
bool specialization_exists_by_index(int class_index, int spec_index);

/* These predicates are the only state/eligibility queries callers need. */
bool specialization_is_active(P_char ch);
bool specialization_matches(P_char ch, uint m_class, int spec);
bool specialization_is_allowed_race_spec(int race, uint m_class, int spec);

#ifdef __cplusplus
}
#endif

#endif
