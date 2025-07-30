#ifndef JSON_H
#define JSON_H

#include <stddef.h>
#include <stdio.h>
#include <stdbool.h>
#include <assert.h>
#include <ctype.h>
#include <string.h>

#include "arena.h"

typedef enum {
    OBJECT,
    ARRAY,
    STRING,
    NUMBER,
    TRUE,
    FALSE,
    NILL
} JSON_VALUE_TYPE;

typedef enum {
    JSON_TRUE,
    JSON_FALSE,
    JSON_NULL
} JSON_ATOM;

struct JsonElement;

typedef struct {
    struct JsonElement* items;
    size_t capacity;
    size_t count;
} JsonObject;

typedef struct {
    struct JsonValue* items;
    size_t capacity;
    size_t count;
} JsonArray;

typedef struct JsonValue {
    JSON_VALUE_TYPE type;
    union {
        JsonObject object;
        JsonArray array;
        char* string;
        double number;
        JSON_ATOM atom;
    };
} JsonValue;

typedef struct JsonElement{
    char* name;
    JsonValue value;
} JsonElement;

JsonObject parse_json_string(char* json_string, bool* valid);
const JsonValue* getByName(const JsonObject* json_object, const char* name);
void print_json_object(const JsonObject* json_object, size_t indent);
void json_cleanup();

#endif // JSON_H