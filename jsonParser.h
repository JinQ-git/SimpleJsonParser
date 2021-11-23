#ifndef _JSON_ELEMENT_H_
#define _JSON_ELEMENT_H_

#include <stdint.h>

// JSON SYNTAX [ https://www.json.org/json-en.html ]

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

//
// Main Function of Json Parser
//
int jsonParser(Element *pOutElement, const char *jsonStr);

//
// Parsing Functions
//
const char *parseValue(const char *pCurrChar, const char **pEnd, Element *pElement);
const char *parseObject(const char *pCurrChar, const char **pEnd, Element *pElement);
const char *parseArray(const char *pCurrChar, const char **pEnd, Element *pElement);
const char *parseString(const char *pCurrChar, const char **pEnd, Element *pElement);
const char *parseNumber(const char *pCurrChar, const char **pEnd, Element *pElement);
const char *parseBoolean(const char *pCurrChar, const char **pEnd, Element *pElement);
const char *parseNull(const char *pCurrChar, const char **pEnd, Element *pElement);

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

#endif // _JSON_ELEMENT_H_