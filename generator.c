#include "stdio.h"
#include "stdlib.h"
#include "stdalign.h"

#define array_count(array) (sizeof(array) / sizeof((array)[0]))
#define zero_struct(s) (memset(&s, 0, sizeof(s)))

// TODO(alexander): special asserts
#define assert_enum(T, v) assert((v) > 0 && (v) < T##_Count && "enum value out of range")
#define assert_power_of_two(x) assert((((x) & ((x) - 1)) == 0) && "x is not power of two")


typedef int bool;
#define true 1
#define false 0

typedef struct {
    char* data;
    size_t count;
} string;

typedef struct {
    char* contents;
    size_t size;
} Read_File_Result;

Read_File_Result
read_entire_file(const char* filepath) {
    Read_File_Result result;
    zero_struct(result);
    
    FILE* file;
    fopen_s(&file, filepath, "rb");
    if (!file) {
        printf("File `%s` was not found!", filepath);
        return result;
    }
    
    fseek(file, 0, SEEK_END);
    size_t file_size = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    
    result.contents = malloc(file_size);
    result.size = file_size;
    fread(result.contents, result.size, 1, file);
    fclose(file);
    return result;
}

bool
write_entire_file(const char* filepath, string contents) {
    FILE* file = fopen(filepath, "wb");
    if (!file) {
        printf("Failed to open `%s` for writing!", filepath);
        return false;
    }
    fwrite(contents.data, contents.count, 1, file);
    fclose(file);
    
    return true;
}

typedef struct {
    string text;
    char symbol;
    int number;
    bool newline;
} Token;

typedef struct {
    char* base;
    char* curr;
    char* end;
    Token peeked;
} Tokenizer;

inline bool 
is_digit(char c) {
    return c >= '0' && c <= '9';
}

inline bool
is_end_of_line(char c) {
    return c == '\n' || c == '\r';
}

inline bool
is_whitespace(char c) {
    return c == ' ' || c == '\t' || c == '\v' || c == '\f' || is_end_of_line(c);
}

Token
next_token(Tokenizer* t) {
    Token result;
    
    if (t->peeked.symbol) {
        result = t->peeked;
        t->peeked.symbol = 0;
        return result;
    }
    
    result.number = -1;
    result.symbol = *t->curr;
    result.text.data = t->curr;
    result.text.count = 0;
    result.newline = 0;
    
    char c = *t->curr++;
    if (is_whitespace(c)) {
        if (is_end_of_line(c)) {
            result.newline = true;
        } else {
            while (t->curr < t->end && is_whitespace(*t->curr)) {
                if (is_end_of_line(c)) {
                    result.newline = true;
                    break;
                }
                t->curr++;
            }
        }
    } else if (is_digit(c)) {
        result.number = c - '0';
        while (t->curr < t->end && is_digit(*t->curr)) {
            c = *t->curr++;
            result.number = result.number * 10 + c - '0';
        }
    } else {
        while (t->curr < t->end && !is_whitespace(*t->curr)) {
            t->curr++;
        }
    }
    
    result.text.count = (int) (t->curr - result.text.data);
    
    return result;
}

Token
peek_token(Tokenizer* t) {
    if (!t->peeked.symbol) {
        t->peeked = next_token(t);
    }
    return t->peeked;
}

typedef enum {
    CodeBlockLanguage_None,
    CodeBlockLanguage_C
} Code_Block_Language;

typedef enum {
    Dom_None,
    Dom_Inline_Text,
    Dom_Paragraph,
    Dom_Heading,
    Dom_Link,
    Dom_Date,
    Dom_Code_Block,
} Dom_Node_Type;

typedef enum {
    TextStyle_None = 0,
    TextStyle_Italics = 1<<0,
    TextStyle_Bold = 1<<1,
    TextStyle_Code = 1<<2,
} Text_Style;

typedef struct Dom_Node Dom_Node;
struct Dom_Node {
    Dom_Node_Type type;
    
    string text;
    Text_Style text_style;
    
    Dom_Node* next;
    int depth;
    
    union {
        struct {
            Dom_Node* first_item;
        } paragraph;
        
        struct {
            int level;
        } heading;
        
        struct {
            Dom_Node* first_item;
        } unordered_list;
        
        struct {
            Dom_Node* second_item;
        } ordered_list;
        
        struct {
            string source;
        } link;
        
        struct {
            int year;
            int month;
            int day;
        } date;
        
        struct {
            Code_Block_Language language;
        } code_block;
    };
};

typedef struct {
    Dom_Node* root;
} Dom;


typedef struct Memory_Block_Header Memory_Block_Header;
struct Memory_Block_Header {
    Memory_Block_Header* prev;
    Memory_Block_Header* next;
    size_t size;
    size_t size_used;
};

typedef struct {
    char* base;
    size_t size;
    size_t curr_used;
    size_t prev_used;
    size_t min_block_size;
} Memory_Arena;

#define ARENA_DEFAULT_BLOCK_SIZE 10240; // 10 kB

// NOTE(Alexander): align has to be a power of two.
inline size_t
align_forward(size_t address, size_t align) {
    size_t modulo = address & (align - 1);
    if (modulo != 0) {
        address += align - modulo;
    }
    return address;
}

void*
arena_push_size(Memory_Arena* arena, size_t size) {
    const size_t align = 16; // TODO(Alexander): hard coded alignment
    size_t current = (size_t) (arena->base + arena->curr_used);
    size_t offset = align_forward(current, align) - (size_t) arena->base;
    
    if (offset + size > arena->size) {
        if (arena->min_block_size == 0) {
            arena->min_block_size = ARENA_DEFAULT_BLOCK_SIZE;
        }
        
        void* block = calloc(1, arena->min_block_size);
        Memory_Block_Header* header = (Memory_Block_Header*) block;
        
        if (arena->base) {
            Memory_Block_Header* prev_header = (Memory_Block_Header*) block;
            prev_header->size_used = arena->curr_used;
            prev_header->next = header;
            header->prev = prev_header;
        }
        
        arena->base = block;
        arena->curr_used = sizeof(Memory_Block_Header);
        arena->prev_used = arena->curr_used;
        arena->size = arena->min_block_size;
        
        current = (size_t) arena->base + arena->curr_used;
        offset = align_forward(current, align) - (size_t) arena->base;
    }
    
    void* result = arena->base + offset;
    arena->prev_used = arena->curr_used;
    arena->curr_used = offset + size;
    
    return result;
}

#define arena_push_struct(arena, type) (type*) arena_push_size(arena, sizeof(type))


Dom
read_markdown_file(const char* filename) {
    Dom result;
    zero_struct(result);
    
    Read_File_Result file = read_entire_file(filename);
    Tokenizer tokenizer;
    Tokenizer* t = &tokenizer;
    t->base = file.contents;
    t->curr = t->base;
    t->end = t->curr + file.size;
    
    // Allocate memory block to store the DOM in
    
    Memory_Arena dom_arena;
    zero_struct(dom_arena);
    
    result.root = arena_push_struct(&dom_arena, Dom_Node);
    
    size_t x = sizeof(Dom_Node);
    
    while (true) {
        Token token = next_token(t);
        if (!token.symbol) {
            break;
        }
        
        if (token.symbol == '#') {
            printf("h%d\n", (int) token.text.count);
        }
        
        if (token.symbol == '*') {
            printf("unordered_list\n");
        }
        
        if (token.number >= 0) {
            printf("ordered_list\n");
        }
    }
    
    return result;
}

int
main(int argc, char* argv[]) {
    if (argc < 2) {
        return;
    }
    
    char* filename = argv[1];
    Dom file = read_markdown_file(filename);
    
    (void) file;
    
    return 0;
}

