#ifndef CJSON_H
#define CJSON_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define cJSON_False 1
#define cJSON_True 2
#define cJSON_NULL 4
#define cJSON_Number 8
#define cJSON_String 16
#define cJSON_Array 32
#define cJSON_Object 64

#define cJSON_IsFalse(item) ((item) != NULL && ((item)->type & cJSON_False))
#define cJSON_IsTrue(item) ((item) != NULL && ((item)->type & cJSON_True))
#define cJSON_IsBool(item) ((item) != NULL && (((item)->type & cJSON_True) || ((item)->type & cJSON_False)))
#define cJSON_IsNull(item) ((item) != NULL && ((item)->type & cJSON_NULL))
#define cJSON_IsNumber(item) ((item) != NULL && ((item)->type & cJSON_Number))
#define cJSON_IsString(item) ((item) != NULL && ((item)->type & cJSON_String))
#define cJSON_IsArray(item) ((item) != NULL && ((item)->type & cJSON_Array))
#define cJSON_IsObject(item) ((item) != NULL && ((item)->type & cJSON_Object))

#define cJSON_ArrayForEach(element, array) \
    for (element = (array != NULL) ? (array)->child : NULL; element != NULL; element = element->next)

typedef struct cJSON {
    struct cJSON *next;
    struct cJSON *prev;
    struct cJSON *child;

    int type;

    char *valuestring;
    int valueint;
    double valuedouble;

    char *string;
} cJSON;

cJSON *cJSON_Parse(const char *value);
const char *cJSON_GetErrorPtr(void);

void cJSON_Delete(cJSON *item);
char *cJSON_Print(const cJSON *item);
char *cJSON_PrintUnformatted(const cJSON *item);

cJSON *cJSON_CreateObject(void);
cJSON *cJSON_CreateArray(void);
cJSON *cJSON_CreateNumber(double num);
cJSON *cJSON_CreateString(const char *string);

void cJSON_AddItemToObject(cJSON *object, const char *string, cJSON *item);
cJSON *cJSON_AddObjectToObject(cJSON *object, const char *string);
void cJSON_AddItemToArray(cJSON *array, cJSON *item);

cJSON *cJSON_GetObjectItem(const cJSON *object, const char *string);
int cJSON_GetArraySize(const cJSON *array);
cJSON *cJSON_GetArrayItem(const cJSON *array, int index);

void cJSON_ReplaceItemInObject(cJSON *object, const char *string, cJSON *newitem);
void cJSON_SetNumberValue(cJSON *object, double num);

cJSON *cJSON_AddStringToObject(cJSON *object, const char *name, const char *string);
cJSON *cJSON_AddNumberToObject(cJSON *object, const char *name, double number);
cJSON *cJSON_AddBoolToObject(cJSON *object, const char *name, int boolean);
cJSON *cJSON_AddNullToObject(cJSON *object, const char *name);

void cJSON_free(void *ptr);

#ifdef __cplusplus
}
#endif

#endif
