#define ARENA_IMPLEMENTATION
#include "json.h"

typedef enum {
    TK_NO_TOKEN,
    TK_LEXER_ERROR,
    TK_OPEN_CURLY_BRACKET,
    TK_CLOSE_CURLY_BRACKET,
    TK_OPEN_SQUARE_BRACKET,
    TK_CLOSE_SQUARE_BRACKET,
    TK_COMMA,
    TK_COLON,
    TK_STRING,
    TK_NUMBER,
    TK_TRUE,
    TK_FALSE,
    TK_NULL
} TOKEN_TYPE;

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
    TokenList* tokens;
    size_t iterator_index;
} TokenIterator;

typedef struct {
    char* items;
    size_t capacity;
    size_t count;
} StringBuilder;

arena_t arena = {0};
arena_t temp_arena = {0};

void sb_append(StringBuilder* sb, const char* string) {
    for(size_t i = 0; i < strlen(string); i++) {
        arena_da_append(&arena, sb, string[i]);
    }
}

Token token_iterator_next(TokenIterator* tk_iterator) {
    if(tk_iterator->iterator_index < tk_iterator->tokens->count) {
        return tk_iterator->tokens->items[tk_iterator->iterator_index++];
    } else {
        return (Token){TK_NO_TOKEN};
    }
}

Token token_iterator_pick_next(TokenIterator* tk_iterator) {
    if(tk_iterator->iterator_index < tk_iterator->tokens->count) {
        return tk_iterator->tokens->items[tk_iterator->iterator_index];
    } else {
        return (Token){TK_NO_TOKEN};
    }
}

Token new_token_string(TOKEN_TYPE type, const char* value) {
    return (Token){type, .string = arena_strdup(&temp_arena, value)};
}

Token new_token_number(TOKEN_TYPE type, double value) {
    return (Token){type, .number = value};
}

Token lex_symbols(const char** json_string_iterator) {
    size_t cursor = 0;
    Token token;
    char current = (*json_string_iterator)[cursor];
    switch (current) {
        case '{':
            token = new_token_string(TK_OPEN_CURLY_BRACKET, "{");
            cursor++;
            break;
        case '}':
            token = new_token_string(TK_CLOSE_CURLY_BRACKET, "}");
            cursor++;
            break;
        case '[':
            token = new_token_string(TK_OPEN_SQUARE_BRACKET, "[");
            cursor++;
            break;
        case ']':
            token = new_token_string(TK_CLOSE_SQUARE_BRACKET, "]");
            cursor++;
            break;
        case ',':
            token = new_token_string(TK_COMMA, ",");
            cursor++;
            break;
        case ':':
            token = new_token_string(TK_COLON, ":");
            cursor++;
            break;
        default:
            token = (Token){TK_NO_TOKEN};
            break;
    }
    *json_string_iterator += cursor;
    return token;
}

void skip_space(const char** json_string_iterator) {
    size_t cursor = 0;
    char current = (*json_string_iterator)[cursor];
    if(isspace(current)) {
        while (isspace(current)) {
            cursor++;
            current = (*json_string_iterator)[cursor];
        }
    }
    *json_string_iterator += cursor;
}

Token lex_string(const char** json_string_iterator) {
    size_t cursor = 0;
    Token token;
    char current = (*json_string_iterator)[cursor];
    if(current == '"') {
        cursor++;
        current = (*json_string_iterator)[cursor];
        StringBuilder string_data = {0};
        while(current != '"' && current != '\0') {
            arena_da_append(&arena, &string_data, current);
            cursor++;
            current = (*json_string_iterator)[cursor];
        }
        if(current != '\0') {
            arena_da_append(&arena, &string_data, '\0');
            token = new_token_string(TK_STRING, arena_strdup(&temp_arena, string_data.items)); // fixme : remove arena_dup
            cursor++;
            *json_string_iterator += cursor;
        } else {
            token = new_token_string(TK_LEXER_ERROR, "unclosed string");
            *json_string_iterator += strlen(*json_string_iterator);
        }
    } else {
        token = (Token){TK_NO_TOKEN};
    }

    return token;
}

Token lex_number(const char** json_string_iterator) {
    Token token;
    char* eptr = NULL;
    double number = strtod(*json_string_iterator, &eptr);
    if(eptr != *json_string_iterator) {
        token = new_token_number(TK_NUMBER, number);
        *json_string_iterator = eptr;
    } else {
        token = (Token){TK_NO_TOKEN};
    }
    return token;
}

Token lex_atom(const char** json_string_iterator, const char* atom, TOKEN_TYPE atom_type) {
    Token token;
    if(!strncmp(*json_string_iterator, atom, strlen(atom))) {
        token = new_token_string(atom_type, atom);
        *json_string_iterator += strlen(atom);
    } else {
        token = (Token){TK_NO_TOKEN};
    }
    return token;
}

Token next_token(const char** json_string_iterator) {
    Token token = {0};

    skip_space(json_string_iterator);

    token = lex_symbols(json_string_iterator);
    if(token.type != TK_NO_TOKEN) return token;

    //token = lex_space(json_string_iterator);
    //if(token.type != TK_NO_TOKEN) return token;
    
    token = lex_string(json_string_iterator);
    if(token.type != TK_NO_TOKEN) return token;

    token = lex_number(json_string_iterator);
    if(token.type != TK_NO_TOKEN) return token;
    
    token = lex_atom(json_string_iterator, "true", TK_TRUE);
    if(token.type != TK_NO_TOKEN) return token;

    token = lex_atom(json_string_iterator, "false", TK_FALSE);
    if(token.type != TK_NO_TOKEN) return token;

    token = lex_atom(json_string_iterator, "null", TK_NULL);
    if(token.type != TK_NO_TOKEN) return token;

    token = new_token_string(TK_LEXER_ERROR, "unknown symbol");
    *json_string_iterator += strlen(*json_string_iterator);
    return token;
}

TokenList lex_json_string(const char* json_string) {
    const char* json_string_iterator = json_string;
    TokenList tokens = {0};

    while (*json_string_iterator != '\0') {
        arena_da_append(&temp_arena, &tokens, next_token(&json_string_iterator));
    }

    return tokens;
}

JsonObject parse_json_object(TokenIterator* tk_iterator, bool* valid);

JsonArray parse_json_array(TokenIterator* tk_iterator, bool* valid);

JsonValue parse_json_value(TokenIterator* tk_iterator, bool* valid) {
    JsonValue json_value = {0};
    *valid = true;

    Token token = token_iterator_pick_next(tk_iterator);

    bool is_valid;
    switch (token.type) {
        case TK_OPEN_CURLY_BRACKET:
            json_value.type = OBJECT;
            json_value.object = parse_json_object(tk_iterator, &is_valid);
            if(!is_valid) {
                *valid = false;
                return (JsonValue){0};
            }
            break;
        case TK_OPEN_SQUARE_BRACKET:
            json_value.type = ARRAY;
            json_value.array = parse_json_array(tk_iterator, &is_valid);
            if(!is_valid) {
                *valid = false;
                return (JsonValue){0};
            }
            break;
        case TK_STRING:
            json_value.type = STRING;
            json_value.string = arena_strdup(&arena, token.string);
            token_iterator_next(tk_iterator);
            break;
        case TK_NUMBER:
            json_value.type = NUMBER;
            json_value.number = token.number;
            token_iterator_next(tk_iterator);
            break;
        case TK_TRUE:
            json_value.type = BOOLEAN;
            json_value.boolean = true;
            token_iterator_next(tk_iterator);
            break;
        case TK_FALSE:
            json_value.type = BOOLEAN;
            json_value.boolean = false;
            token_iterator_next(tk_iterator);
            break;
        case TK_NULL:
            json_value.type = NILL;
            json_value.nill = NULL;
            token_iterator_next(tk_iterator);
            break;
        default:
            *valid = false;
            return (JsonValue){0};
    }

    return json_value;
}

JsonElement parse_json_element(TokenIterator* tk_iterator, bool* valid) {
    JsonElement json_element = {0};
    *valid = true;

    Token token = token_iterator_next(tk_iterator);
    if(token.type != TK_STRING) {
        *valid = false;
        return (JsonElement){0};
    }
    json_element.name = arena_strdup(&arena, token.string);

    if(token_iterator_next(tk_iterator).type != TK_COLON) {
        *valid = false;
        return (JsonElement){0};
    }

    bool is_valid;
    json_element.value = parse_json_value(tk_iterator, &is_valid);
    if(!is_valid) {
        *valid = false;
        return (JsonElement){0};
    }
    return json_element;
}

JsonArray parse_json_array(TokenIterator* tk_iterator, bool* valid) {
    JsonArray json_array = {0};
    *valid = true;

    if(token_iterator_next(tk_iterator).type != TK_OPEN_SQUARE_BRACKET) {
        *valid = false;
        return (JsonArray){0};
    }

    if(token_iterator_pick_next(tk_iterator).type == TK_CLOSE_SQUARE_BRACKET) {
        token_iterator_next(tk_iterator);
        return json_array;
    }

    bool is_valid;
    JsonValue json_value = parse_json_value(tk_iterator, &is_valid);
    if(!is_valid) {
        *valid = false;
        return (JsonArray){0};
    }

    arena_da_append(&arena, &json_array, json_value);
    while(token_iterator_pick_next(tk_iterator).type == TK_COMMA) {
        token_iterator_next(tk_iterator);
        json_value = parse_json_value(tk_iterator, &is_valid);
        if(!is_valid) {
            *valid = false;
            return (JsonArray){0};
        }
        arena_da_append(&arena, &json_array, json_value);
        
    }

    if(token_iterator_next(tk_iterator).type != TK_CLOSE_SQUARE_BRACKET) {
        *valid = false;
        return (JsonArray){0};
    }

    return json_array;
}

JsonObject parse_json_object(TokenIterator* tk_iterator, bool* valid) {
    JsonObject json_object = {0};
    *valid = true;

    if(token_iterator_next(tk_iterator).type != TK_OPEN_CURLY_BRACKET) {
        *valid = false;
        return (JsonObject){0};
    }

    if(token_iterator_pick_next(tk_iterator).type == TK_CLOSE_CURLY_BRACKET) {
        token_iterator_next(tk_iterator);
        return json_object;
    }

    bool is_valid;
    JsonElement json_element = parse_json_element(tk_iterator, &is_valid);
    if(!is_valid) {
        *valid = false;
        return (JsonObject){0};
    }
    arena_da_append(&arena, &json_object, json_element);
    while(token_iterator_pick_next(tk_iterator).type == TK_COMMA) {
        token_iterator_next(tk_iterator);
        json_element = parse_json_element(tk_iterator, &is_valid);
        if(!is_valid) {
            *valid = false;
            return (JsonObject){0};
        }
        arena_da_append(&arena, &json_object, json_element);
        
    }

    if(token_iterator_next(tk_iterator).type != TK_CLOSE_CURLY_BRACKET) {
        *valid = false;
        return (JsonObject){0};
    }

    return json_object;
}

void print_tokens(const TokenList* tokens) {
    for(size_t i = 0; i < tokens->count; i++) {
        switch (tokens->items[i].type) {
            case TK_LEXER_ERROR:
                printf("token[%ld] : type = LEXER_ERROR; value = %s\n", i, tokens->items[i].string);
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
            case TK_COMMA:
                printf("token[%ld] : type = COMMA\n", i);
                break;
            case TK_COLON:
                printf("token[%ld] : type = COLON\n", i);
                break;
            case TK_STRING:
                printf("token[%ld] : type = STRING; value = \"%s\"\n", i, tokens->items[i].string);
                break;
            case TK_NUMBER:
                printf("token[%ld] : type = NUMBER; value = \"%f\"\n", i, tokens->items[i].number);
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
            case TK_NO_TOKEN:
                assert("Unreachable" && 0);
                break;
            }
    }
}

void print_tabs(size_t nb_tabs) {
    for(size_t i = 0; i < nb_tabs; i++) printf("  ");
}

void print_json_object(const JsonObject* json_object, size_t indent);
void print_json_array(const JsonArray* json_array, size_t indent);

void print_value(const JsonValue* json_value, size_t indent) {
    switch (json_value->type) {
        case OBJECT:
            printf("\n");
            print_json_object(&json_value->object, indent+1);
            break;
        case ARRAY:
            printf("\n");
            print_json_array(&json_value->array, indent+1);
            break;
        case STRING:
            printf("\"%s\"", json_value->string);
            break;
        case NUMBER:
            printf("%f", json_value->number);
            break;
        case BOOLEAN:
            printf("%s", json_value->boolean ? "true" : "false");
            break;
        case NILL:
            printf("null");
            break;
        break;
    }
}

void print_json_array(const JsonArray* json_array, size_t indent) {
    for (size_t i = 0; i < json_array->count; i++) {
        print_tabs(indent);
        print_value(&json_array->items[i], indent);
        printf("\n");
    }
}

void print_json_object(const JsonObject* json_object, size_t indent) {
    for (size_t i = 0; i < json_object->count; i++) {
        JsonElement json_element = json_object->items[i];
        print_tabs(indent);
        printf("%s : ", json_element.name);
        print_value(&json_element.value, indent);
        printf("\n");
    }
}

JsonObject parse_json_string(const char* json_string, bool* valid) {
    TokenList tokens = lex_json_string(json_string);
    TokenIterator tk_iterator = {
        .tokens = &tokens,
        .iterator_index = 0
    };
    bool is_valid;
    JsonObject result = parse_json_object(&tk_iterator, &is_valid);
    *valid = is_valid;
    arena_free(&temp_arena);
    return result;
}

void write_json_object(const JsonObject* json_object, StringBuilder* sb);
void write_json_array(const JsonArray* json_array, StringBuilder* sb);

void write_value(const JsonValue* json_value, StringBuilder* sb) {
    char* double_buf = arena_malloc(&temp_arena, DBL_MAX_10_EXP+2);
    switch (json_value->type) {
        case OBJECT:
            write_json_object(&json_value->object, sb);
            break;
        case ARRAY:
            write_json_array(&json_value->array, sb);
            break;
        case STRING:
            sb_append(sb, "\"");
            sb_append(sb, json_value->string);
            sb_append(sb, "\"");
            break;
        case NUMBER:
            sprintf(double_buf, "%f", json_value->number);
            sb_append(sb, double_buf);
            break;
        case BOOLEAN:
            if(json_value->boolean) {
                sb_append(sb, "true");
            } else {
                sb_append(sb, "false");
            }
            break;
        case NILL:
            sb_append(sb, "null");
            break;
        break;
    }
    arena_free(&temp_arena);
}

void write_json_array(const JsonArray* json_array, StringBuilder* sb) {
    sb_append(sb, "[");
    for (size_t i = 0; i < json_array->count; i++) {
        write_value(&json_array->items[i], sb);
        if(i < json_array->count-1) sb_append(sb, ",");
    }
    sb_append(sb, "]");
}

void write_json_object(const JsonObject* json_object, StringBuilder* sb) {
    sb_append(sb, "{");
    for (size_t i = 0; i < json_object->count; i++) {
        JsonElement json_element = json_object->items[i];
        sb_append(sb, "\"");
        sb_append(sb, json_element.name);
        sb_append(sb, "\"");
        sb_append(sb, ":");
        write_value(&json_element.value, sb);
        if(i < json_object->count-1) sb_append(sb, ",");
    }
    sb_append(sb, "}");
}

char* write_json(const JsonObject* json_object) {
    StringBuilder sb = {0};
    write_json_object(json_object, &sb);
    arena_da_append(&arena, &sb, '\0');
    return sb.items;
}

const JsonValue* get_by_name(const JsonObject* json_object, const char* name) {
    for(size_t i = 0; i < json_object->count; i++) {
        JsonElement* json_element = &json_object->items[i];
        if(!strcmp(json_element->name, name)) return &json_element->value;
    }
    return NULL;
}

void object_add_string(JsonObject* json_object, const char* name, const char* value) {
    JsonElement json_element = {0};
    json_element.name = arena_strdup(&arena,name);
    json_element.value.type = STRING;
    json_element.value.string = arena_strdup(&arena, value);
    arena_da_append(&arena, json_object, json_element);
}

void object_add_number(JsonObject* json_object, const char* name, double number) {
    JsonElement json_element = {0};
    json_element.name = arena_strdup(&arena,name);
    json_element.value.type = NUMBER;
    json_element.value.number = number;
    arena_da_append(&arena, json_object, json_element);
}

void object_add_boolean(JsonObject* json_object, const char* name, bool boolean) {
    JsonElement json_element = {0};
    json_element.name = arena_strdup(&arena,name);
    json_element.value.type = BOOLEAN;
    json_element.value.boolean = boolean;
    arena_da_append(&arena, json_object, json_element);
}

void object_add_null(JsonObject* json_object, const char* name) {
    JsonElement json_element = {0};
    json_element.name = arena_strdup(&arena,name);
    json_element.value.type = NILL;
    json_element.value.nill = NULL;
    arena_da_append(&arena, json_object, json_element);
}

void object_add_object(JsonObject* json_object, const char* name, JsonObject value) {
    JsonElement json_element = {0};
    json_element.name = arena_strdup(&arena,name);
    json_element.value.type = OBJECT;
    json_element.value.object = value;
    arena_da_append(&arena, json_object, json_element);
}

void object_add_array(JsonObject* json_object, const char* name, JsonArray value) {
    JsonElement json_element = {0};
    json_element.name = arena_strdup(&arena,name);
    json_element.value.type = ARRAY;
    json_element.value.array = value;
    arena_da_append(&arena, json_object, json_element);
}

void array_add_string(JsonArray* json_array, const char* value) {
    JsonValue json_value = {0};
    json_value.type = STRING;
    json_value.string = arena_strdup(&arena, value);
    arena_da_append(&arena, json_array, json_value);
}

void array_add_number(JsonArray* json_array, double number) {
    JsonValue json_value = {0};
    json_value.type = NUMBER;
    json_value.number = number;
    arena_da_append(&arena, json_array, json_value);
}

void array_add_boolean(JsonArray* json_array, bool boolean) {
    JsonValue json_value = {0};
    json_value.type = BOOLEAN;
    json_value.boolean = boolean;
    arena_da_append(&arena, json_array, json_value);
}

void array_add_null(JsonArray* json_array) {
    JsonValue json_value = {0};
    json_value.type = NILL;
    json_value.nill = NULL;
    arena_da_append(&arena, json_array, json_value);
}

void array_add_object(JsonArray* json_array, JsonObject value) {
    JsonValue json_value = {0};
    json_value.type = OBJECT;
    json_value.object = value;
    arena_da_append(&arena, json_array, json_value);
}

void array_add_array(JsonArray* json_array, JsonArray value) {
    JsonValue json_value = {0};
    json_value.type = ARRAY;
    json_value.array = value;
    arena_da_append(&arena, json_array, json_value);
}

void json_cleanup() {
    arena_free(&arena);
}