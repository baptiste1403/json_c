#ifndef JSON_H
#define JSON_H

#include <stddef.h>
#include <stdio.h>
#include <stdbool.h>
#include <assert.h>
#include <ctype.h>
#include <string.h>
#include <float.h>

#include "arena.h"

typedef enum {
    OBJECT,
    ARRAY,
    STRING,
    NUMBER,
    BOOLEAN,
    NILL
} JSON_VALUE_TYPE;

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
        bool boolean;
        void* nill;
    };
} JsonValue;

typedef struct JsonElement{
    char* name;
    JsonValue value;
} JsonElement;

JsonObject parse_json_string(const char* json_string, bool* valid);
char* write_json(const JsonObject* json_object);
const JsonValue* get_by_name(const JsonObject* json_object, const char* name);

void object_add_string(JsonObject* json_object, const char* name, const char* value);
void object_add_number(JsonObject* json_object, const char* name, double number);
void object_add_boolean(JsonObject* json_object, const char* name, bool boolean);
void object_add_null(JsonObject* json_object, const char* name);
void object_add_object(JsonObject* json_object, const char* name, JsonObject value);
void object_add_array(JsonObject* json_object, const char* name, JsonArray value);

void array_add_string(JsonArray* json_array, const char* value);
void array_add_number(JsonArray* json_array, double number);
void array_add_boolean(JsonArray* json_array, bool boolean);
void array_add_null(JsonArray* json_array);
void array_add_object(JsonArray* json_array, JsonObject value);
void array_add_array(JsonArray* json_array, JsonArray value);

void print_json_object(const JsonObject* json_object, size_t indent);
void json_cleanup();

#endif // JSON_H