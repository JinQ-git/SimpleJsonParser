#ifndef _JSON_PARSER_H_
#define _JSON_PARSER_H_

#include <stdint.h>

// JSON SYNTAX [ https://www.json.org/json-en.html ]

typedef enum
{
    // System Error
    JPE_OUT_OF_MEMORY = -1,
    // No Error
    JPE_NO_ERROR = 0,
    // Syntax Error (Unexpected Token)
    JPE_SYNTAX_ERROR                = 100, // Unexpected Token
    JPE_SYNTAX_ERROR_END            = 101, // End of string while parsing
    JPE_SYNTAX_ERROR_STRING_ESCAPE  = 110, // Invalid Escape Character
    JPE_SYNTAX_ERROR_UNICODE_ESCAPE = 111, // Invalid Unicode Character after \u
    JPE_SYNTAX_ERROR_ARRAY          = 120, // Unexpected Token while parsing array
    JPE_SYNTAX_ERROR_ARRAY_COMMA    = 121, // Missing Comma?
    JPE_SYNTAX_ERROR_OBJECT         = 130, // Unexpected Token while parsing object
    JPE_SYNTAX_ERROR_OBJECT_KEY     = 131, // Invalid Object Key
    JPE_SYNTAX_ERROR_OBJECT_COLON   = 132, // Missing Colon?
    JPE_SYNTAX_ERROR_OBJECT_COMMA   = 133  // Missing Comma?
} JsonParsingError;

typedef enum
{
    TYPE_NULL        = 0,
    TYPE_OBJECT      = 1,
    TYPE_ARRAY       = 2,
    TYPE_STRING      = 3,
    TYPE_DBL_NUMBER  = 4,
    TYPE_INT_NUMBER  = 5,
    TYPE_BOOLEAN     = 6
} ElementType;

struct tagObjectNode;
struct tagArrayNode;

typedef struct tagElement
{
    ElementType              type;
    union {
        double               dNumberValue;
        int64_t              iNumberValue; // also boolean value
        char                 *stringValue; // const char*
        struct tagObjectNode *objectValue;
        struct tagArrayNode  *arrayValue;
    };
} Element;

typedef struct tagObjectNode
{
    struct tagElement     element;
    char                 *key; // const char*
    struct tagObjectNode *next;
} ObjectNode;

typedef struct tagArrayNode
{
    struct tagElement    element;
    struct tagArrayNode *next;
} ArrayNode;

typedef struct tagJsonErrorInfo
{
    JsonParsingError error;
    int              line;     // line number start from 1
    int              column;   // column number start from 1
    int              position; // zero-based byte index
} JsonErrorInfo;

//
// Main Function of Json Parser
// ** Input Josn String must be UTF-8 Encoded Text
// ** pOutErrorInfo can be `NULL`, if do not need any error information.
//
JsonParsingError parseJsonString(Element *pOutElement, const char *jsonStr, JsonErrorInfo *pOutErrorInfo);

//
// Release Element
//
void resetElement(Element *pElement);

//
// Print Result
//
void printElementSimple(const Element *pElement);
void printElementDepth1(const Element *pElement);
void printElementDepthAll(const Element *pElement);

//
// Print Error
//
void printJsonErrorInfo(const JsonErrorInfo* pInfo, const char *jsonStr);

#endif // _JSON_PARSER_H_
