#include <stddef.h>
#include <stdio.h>
#include <assert.h>
#include <ctype.h>
#include <string.h>

#define ARENA_IMPLEMENTATION
#include "arena.h"

typedef enum {
    TK_LEXER_ERROR,
    TK_OPEN_CURLY_BRACKET,
    TK_CLOSE_CURLY_BRACKET,
    TK_OPEN_SQUARE_BRACKET,
    TK_CLOSE_SQUARE_BRACKET,
    TK_WHITESPACE,
    TK_COMMA,
    TK_COLON,
    TK_STRING,
    TK_NUMBER,
    TK_TRUE,
    TK_FALSE,
    TK_NULL
} TOKEN_TYPE;

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

typedef struct {
    TOKEN_TYPE type;
    union {
        char* string;
        double number;
    };
} Token;

typedef struct {
    Token* items;
    size_t capacity;
    size_t count;
} TokenList;

typedef struct {
    char* items;
    size_t capacity;
    size_t count;
} StringBuilder;

typedef struct {
    char* name;
} JsonKey;

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

typedef struct {
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
    JsonKey key;
    JsonValue value;
} JsonElement;

arena_t arena = {0};

Token new_token_string(TOKEN_TYPE type, const char* value) {
    return (Token){type, .string = arena_strdup(&arena, value)};
}

Token new_token_number(TOKEN_TYPE type, double value) {
    return (Token){type, .number = value};
}

Token next_token(char** json_string_iterator) {
    size_t cursor = 0;
    Token token= {0};
    char current = (*json_string_iterator)[cursor];
    switch (current) {
        case '{':
            token = new_token_string(TK_OPEN_CURLY_BRACKET, "{");
            cursor++;
            *json_string_iterator += cursor;
            return token;
        case '}':
            token = new_token_string(TK_CLOSE_CURLY_BRACKET, "}");
            cursor++;
            *json_string_iterator += cursor;
            return token;
        case '[':
            token = new_token_string(TK_OPEN_SQUARE_BRACKET, "[");
            cursor++;
            *json_string_iterator += cursor;
            return token;
        case ']':
            token = new_token_string(TK_CLOSE_SQUARE_BRACKET, "]");
            cursor++;
            *json_string_iterator += cursor;
            return token;
        case ',':
            token = new_token_string(TK_COMMA, ",");
            cursor++;
            *json_string_iterator += cursor;
            return token;
        case ':':
            token = new_token_string(TK_COLON, ":");
            cursor++;
            *json_string_iterator += cursor;
            return token;
        default:
            break;
    }
    if(isspace(current)) {
        while (isspace(current)) {
            cursor++;
            current = (*json_string_iterator)[cursor];
        }
        token = new_token_string(TK_WHITESPACE, "");
        *json_string_iterator += cursor;
        return token;
    }
    if(current == '"') {
        cursor++;
        current = (*json_string_iterator)[cursor];
        StringBuilder string_data = {0};
        while(current != '"' && current != '\0') {
            arena_da_append(&arena, &string_data, current);
            cursor++;
            current = (*json_string_iterator)[cursor];
        }
        if(current == '\0') {
            token = new_token_string(TK_LEXER_ERROR, "unclosed string");
            *json_string_iterator += strlen(*json_string_iterator);
            return token;
        }
        
        arena_da_append(&arena, &string_data, '\0');
        token = new_token_string(TK_STRING, arena_strdup(&arena, string_data.items));
        cursor++;
        *json_string_iterator += cursor;
        return token;
    }
    char* eptr = NULL;
    double number = strtod(*json_string_iterator, &eptr);
    if(eptr != *json_string_iterator) {
        token = new_token_number(TK_NUMBER, number);
        *json_string_iterator = eptr;
        return token;
    }
    if(!strncmp(*json_string_iterator, "true", 4)) {
        token = new_token_string(TK_TRUE, "true");
        *json_string_iterator += 4;
        return token;
    }
    if(!strncmp(*json_string_iterator, "false", 5)) {
        token = new_token_string(TK_FALSE, "false");
        *json_string_iterator += 5;
        return token;
    }
    if(!strncmp(*json_string_iterator, "null", 4)) {
        token = new_token_string(TK_NULL, "null");
        *json_string_iterator += 4;
        return token;
    }
    token = new_token_string(TK_LEXER_ERROR, "unknown symbol");
    *json_string_iterator += strlen(*json_string_iterator);
    return token;
}

TokenList lex_string(char* json_string) {
    char* json_string_iterator = json_string;
    TokenList tokens = {0};

    while (*json_string_iterator != '\0') {
        arena_da_append(&arena, &tokens, next_token(&json_string_iterator));
    }

    return tokens;
}



int main() {
    char* test = "{\
        \"glossary\": {\
            \"title\": \"example glossary\",\
            \"GlossDiv\": {\
                \"title\": \"S\",\
                \"GlossList\": {\
                    \"GlossEntry\": {\
                        \"ID\": \"SGML\",\
                        \"SortAs\": \"SGML\",\
                        \"GlossTerm\": \"Standard Generalized Markup Language\",\
                        \"Acronym\": \"SGML\",\
                        \"Abbrev\": \"ISO 8879:1986\",\
                        \"GlossDef\": {\
                            \"para\": \"A meta-markup language, used to create markup languages such as DocBook.\",\
                            \"GlossSeeAlso\": [\"GML\", \"XML\"]\
                        },\
                        \"GlossSee\": \"markup\"\
                    }\
                }\
            }\
        }\
    }";

    TokenList tokens = lex_string(test);

    for(size_t i = 0; i < tokens.count; i++) {
        switch (tokens.items[i].type) {
            case TK_LEXER_ERROR:
                printf("token[%ld] : type = LEXER_ERROR; value = %s\n", i, tokens.items[i].string);
                break;
            case TK_OPEN_CURLY_BRACKET:
                printf("token[%ld] : type = OPEN_CURLY_BRACKET\n", i);
                break;
            case TK_CLOSE_CURLY_BRACKET:
                printf("token[%ld] : type = CLOSE_CURLY_BRACKET\n", i);
                break;
            case TK_OPEN_SQUARE_BRACKET:
                printf("token[%ld] : type = OPEN_SQUARE_BRACKET\n", i);
                break;
            case TK_CLOSE_SQUARE_BRACKET:
                printf("token[%ld] : type = CLOSE_SQUARE_BRACKET\n", i);
                break;
            case TK_WHITESPACE:
                printf("token[%ld] : type = WHITESPACE\n", i);
                break;
            case TK_COMMA:
                printf("token[%ld] : type = COMMA\n", i);
                break;
            case TK_COLON:
                printf("token[%ld] : type = COLON\n", i);
                break;
            case TK_STRING:
                printf("token[%ld] : type = STRING; value = \"%s\"\n", i, tokens.items[i].string);
                break;
            case TK_NUMBER:
                printf("token[%ld] : type = NUMBER; value = \"%f\"\n", i, tokens.items[i].number);
                break;
            case TK_TRUE:
                printf("token[%ld] : type = TRUE\n", i);
                break;
            case TK_FALSE:
                printf("token[%ld] : type = FALSE\n", i);
                break;
            case TK_NULL:
                printf("token[%ld] : type = NILL\n", i);
                break;
        }
    }

    arena_free(&arena);
    return 0;
}