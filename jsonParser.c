#include "jsonParser.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h> // strncmp
#include <ctype.h> // isspace()
#include <math.h>  // fabs(), ceil()
#include <float.h> // DBL_EPSILON

//
// Main Function of Json Parser
//
int jsonParser(Element *pOutElement, const char *jsonStr)
{
    const char *pCurrChar = jsonStr;
    int resultCode = 0;

    while( *pCurrChar ) 
    {
        const char *pEnd = (const char *)0;
        if( !parseValue( pCurrChar, &pEnd, pOutElement ) ) {
            resultCode = -1;
            break;
        }

        if( pEnd ) {
            while( isspace( *pEnd ) ) { pEnd++; }
            if( *pEnd ) {
                if( *pEnd == ',' || *pEnd == ';' )
                {
                    const char* pTmp = pEnd;
                    pEnd++;
                    while( isspace( *pEnd ) ) { pEnd++; }
                    if( *pEnd ) {
                        pCurrChar = pEnd;
                        resetElement( pOutElement );
                        continue;
                    }
                    else if( *pTmp == ';' ) {
                        break; // Parsing Done
                    }
                    else { // *pTmp == ','
                        fprintf(stderr, "Unexpected Token Character ','\n");
                        resultCode = -1;
                        break;
                    }
                }
                else
                {
                    fprintf(stderr, "Unknown Token Character after Parsing: %c\n", *pCurrChar);
                    resultCode = -1;
                    break;
                }
            }
            else { break; } // Parsing Done
        }
    }

    return resultCode;
}

static int hex2int(char hexCode)
{
    if( 96 < hexCode && hexCode < 103 ) { return hexCode - 'a' + 10; }
    else if( 64 < hexCode && hexCode < 71 ) { return hexCode - 'A' + 10; }
    else if( 47 < hexCode && hexCode < 58 ) { return hexCode - '0'; }

    return 0;
}

static char *_getString(const char *pCurrChar, const char ** pEnd)
{
    const char *pTmp = pCurrChar + 1;
    int charCount = 0;
    char *stringValue = (char *)0;

    // Standard not allow single quote
    if( *pCurrChar != '"' ) { 
        if( pEnd ) { *pEnd = pCurrChar; }
        return (char *)0; 
    }

    // Count Total Characters
    while( *pTmp )
    {
        if( *pTmp == '\\') {
            switch( *++pTmp )
            {
                case '"':
                case '\\':
                case '/':
                case 'b':
                case 'f':
                case 'n':
                case 'r':
                case 't':
                    break;
                case 'u': // Convert UTF-16 to UTF-8
                    if( !isxdigit(pTmp[1]) || !isxdigit(pTmp[2]) || !isxdigit(pTmp[3]) || !isxdigit(pTmp[4]) ) {
                        fprintf(stderr, "Unexpected Token after '\\u': No Hex Digit: %c%c%c%c\n", pTmp[1], pTmp[2], pTmp[3], pTmp[4]);
                        if( pEnd ) { *pEnd = pTmp; }
                        return (char *)0;
                    }
                    pTmp += 4;
                    // Calculate Bytes of UTF-8 Conversion
                    if( pTmp[1] != '0' || pTmp[2] > '7' ) { charCount += 2; }      // 0x0800 ~ 0xFFFF
                    else if( pTmp[2] > '0' || pTmp[3] > '7' ) { charCount += 1; }  // 0x0080 ~ 0x07FF
                    // else // 0x0000 ~ 0x007F
                    break;
                default:
                    fprintf(stderr, "Unexpected Token after '\\': %c\n", *pTmp);
                    if( pEnd ) { *pEnd = pTmp; }
                    return (char *)0;
            }
        }
        else if( *pTmp == '"' ) { // End of String
            break;
        } 
        pTmp++;
        charCount++;
    }

    stringValue = (char *)calloc(charCount + 1, sizeof(char));
    if( !stringValue ) {
        fprintf(stderr, "Critical Error: String Allocation Failed...\n");
        if( pEnd ) { *pEnd = pTmp + 1; }
        return (char *)0;
    }

    // Copy String
    const char *pSrc = pCurrChar + 1;
    char *pDst = stringValue;
    while( pSrc < pTmp ) {
        if( *pSrc == '\\' ) {
            switch( *++pSrc ) {
                case '"' : *pDst++ = '"'; break;
                case '\\': *pDst++ = '\\'; break; 
                case '/' : *pDst++ = '/'; break;
                case 'b' : *pDst++ = '\b'; break;
                case 'f' : *pDst++ = '\f'; break;
                case 'n' : *pDst++ = '\n'; break;
                case 'r' : *pDst++ = '\r'; break;
                case 't' : *pDst++ = '\t'; break;
                case 'u': // Convert UTF-16 to UTF-8
                {
                    int unicode = hex2int(pSrc[1]) << 12
                                | hex2int(pSrc[2]) << 8
                                | hex2int(pSrc[3]) << 4
                                | hex2int(pSrc[4]);
                    if( unicode < 0x80 ) { // U+0000 ~ U+007F -> | 0xxxxxxx |
                        *pDst++ = (char)unicode;
                    }
                    else if( unicode < 0x800 ) { // U+0080 ~ U+07FF -> | 110xxxxx | 10xxxxxx |
                        *pDst++ = (char)(0xC0 + (unicode >> 6));
                        *pDst++ = (char)(0x80 + (unicode & 0x3F));
                    }
                    else { // U+0800 ~ U+FFFF -> | 1110xxxx | 10xxxxxx | 10xxxxxx |
                        *pDst++ = (char)(0xe0 + (unicode >> 12));          // 0xe0 + unicode / 4096
                        *pDst++ = (char)(0x80 + ((unicode >> 6) & 0x3F));  // 0x80 + unicode / 64 % 64
                        *pDst++ = (char)(0x80 + (unicode & 0x3F));         // 0x80 + unicode % 64
                    }
                    pSrc += 4;
                }
                break;
            }
            pSrc++;
        }
        else {
            *pDst++ = *pSrc++;
        }
    }

    if( pEnd ) { *pEnd = (pTmp + 1); }
    return stringValue;
}

const char *parseValue(const char *pCurrChar, const char **pEnd, Element *pElement)
{
    // Trim Left
    while( isspace(*pCurrChar) ) { pCurrChar++; }

    switch( *pCurrChar ) 
    {
        // Try parse as Object
        case '{': return parseObject( pCurrChar, pEnd, pElement );
        // Try parse as Array
        case '[': return parseArray( pCurrChar, pEnd, pElement );
        // Try parse as String
        case '"': return parseString( pCurrChar, pEnd, pElement );
        // Try parse as Number (Octal, Decimal, Hex Integer and Floating Point)
        case '+':
        case '-': 
        case '0': 
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9': 
            return parseNumber( pCurrChar, pEnd, pElement );
        // Try parse as Boolean
        case 't':
        case 'f': 
            return parseBoolean( pCurrChar, pEnd, pElement );
        // Try parse as Null
        case 'n':
            return parseNull( pCurrChar, pEnd, pElement );

        default: break; // Token Error
    }

    fprintf(stderr, "Token Character Error: %c\n", *pCurrChar);
    if( pEnd ) { *pEnd = pCurrChar; }

    return (const char *)0;
}

const char *parseObject(const char *pCurrChar, const char **pEnd, Element *pElement)
{
    if( *pCurrChar != '{' ) { 
        if( pEnd ) { *pEnd = pCurrChar; }
        return (const char *)0; 
    }
    pCurrChar++;

    const char *ptrEnd = (const char *)0;
    ObjectNode *pHead = (ObjectNode *)0;
    ObjectNode *pTail = (ObjectNode *)0;

    // Check Empty Object
    while( isspace(*pCurrChar) ) { pCurrChar++; }
    if( *pCurrChar == '}' ) {
        // Empty Object
        if( pEnd ) { *pEnd = pCurrChar + 1; }
        pElement->type = TYPE_OBJECT;
        pElement->objectValue = (ObjectNode *)0;
        return pCurrChar + 1;
    }

    for(;;) {
        // Parsing Object
        if( *pCurrChar == '"' )
        {
            // Get Key
            char *keyString = _getString(pCurrChar, &ptrEnd);
            if( !keyString ) { 
                if( pEnd ) { *pEnd = ptrEnd; }
                return (const char *)0; 
            }

            pCurrChar = ptrEnd;
            while( isspace(*pCurrChar) ) { pCurrChar++; }
            if( *pCurrChar != ':' ) {
                free(keyString);
                fprintf(stderr, "Object Prasing Error: Unexpected Token Character: %c... ( ':' is required )\n", *pCurrChar);
                if( pEnd ) { *pEnd = pCurrChar; }
                return (const char *)0;
            }
            else { 
                ++pCurrChar; 
                while( isspace(*pCurrChar) ) { pCurrChar++; }
            }

            // Allocate Element
            ObjectNode *pNode = (ObjectNode *)calloc(1, sizeof(ObjectNode));
            if( !pNode ) {
                fprintf(stderr, "Critical Error: Memory Allocation Failed\n");
                free(keyString);
                if( pHead ) { // Release
                    Element e = { .type = TYPE_OBJECT, .objectValue = pHead };
                    resetElement( &e );
                }
                if( pEnd ) { *pEnd = pCurrChar; }
                return (const char *)0;
            }
            else {
                pNode->key = keyString;
                if( pTail ) {
                    pTail->next = pNode;
                    pTail = pNode;
                }
                else {
                    pHead = pNode;
                    pTail = pNode;
                }
            }
            
            // Parse Value Here!!
            const char *ret = parseValue( pCurrChar, &ptrEnd, &(pNode->element) );
            if( !ret ) {
                if( pHead ) { // Release
                    Element e = { .type = TYPE_OBJECT, .objectValue = pHead };
                    resetElement( &e );
                }
                if( pEnd ) { *pEnd = ptrEnd; }
                return (const char *)0;
            }
            pCurrChar = ptrEnd;

            // Check "," or "}"
            while( isspace(*pCurrChar) ) { pCurrChar++; }
            if( *pCurrChar == ',' ) {
                pCurrChar++;
                while( isspace(*pCurrChar) ) { pCurrChar++; }
                continue;
            }
            else if( *pCurrChar == '}' ) {
                // Parsing Done
                pCurrChar++;
                break;
            }
            else {
                fprintf(stderr, "Object Parsing Error: Unexpected Token Character: %c...\n", *pCurrChar );
                if( pEnd ) { *pEnd = pCurrChar; }
                return (const char*)0;
            }

        }
        else {
            fprintf(stderr, "Object Parsing Error: Unexpected Token Character: %c...\n", *pCurrChar );
            if( pEnd ) { *pEnd = pCurrChar; }
            return (const char*)0;
        }
    }

    pElement->type = TYPE_OBJECT;
    pElement->objectValue = pHead;
    if( pEnd ) { *pEnd = pCurrChar; }

    return pCurrChar;
}

const char *parseArray(const char *pCurrChar, const char **pEnd, Element *pElement)
{
    if( *pCurrChar != '[' ) { 
        if( pEnd ) { *pEnd = pCurrChar; }
        return (const char *)0; 
    }
    pCurrChar++;

    const char *ptrEnd = (const char *)0;
    ArrayNode *pHead = (ArrayNode *)0;
    ArrayNode *pTail = (ArrayNode *)0;

    // Check Empty Array
    while( isspace(*pCurrChar) ) { pCurrChar++; }
    if( *pCurrChar == ']' ) {
        // Empty Array
        if( pEnd ) { *pEnd = pCurrChar + 1; }
        pElement->type = TYPE_ARRAY;
        pElement->arrayValue = (ArrayNode *)0;
        return pCurrChar + 1;
    }

    // Prasing Array
    for(;;) {
        // Allocate Element
        ArrayNode *pNode = (ArrayNode *)calloc( 1, sizeof(ArrayNode) );
        if( !pNode ) {
            fprintf(stderr, "Critical Error: Memory Allocation Failed: ArrayNode\n");
            if( pHead ) { // Release
                Element e = { .type = TYPE_ARRAY, .arrayValue = pHead };
                resetElement(&e);
            }
            if( pEnd ) { *pEnd = pCurrChar; }
            return (const char *)0;
        }
        else {
            if( pTail ) {
                pTail->next = pNode;
                pTail = pNode;
            }
            else {
                pHead = pNode;
                pTail = pNode;
            }
        }

        // Parse Value Here!!
        const char *ret = parseValue( pCurrChar, &ptrEnd, &(pNode->element) );
        if( !ret ) {
            if( pHead ) { // Release
                Element e = { .type = TYPE_ARRAY, .arrayValue = pHead };
                resetElement(&e);
            }
            if( pEnd ) { *pEnd = ptrEnd; }
            return (const char *)0;
        }
        pCurrChar = ptrEnd;

        // Check "," or "]"
        while( isspace(*pCurrChar) ) { pCurrChar++; }
        if( *pCurrChar == ',' ) {
            pCurrChar++;
            while( isspace(*pCurrChar) ) { pCurrChar++; }
            continue;
        }
        else if( *pCurrChar == ']' ) {
            // Parsing Done
            pCurrChar++;
            break;
        }
        else {
            fprintf(stderr, "Array Parsing Error: Unexpected Token Character: %c...\n", *pCurrChar );
            if( pEnd ) { *pEnd = pCurrChar; }
            return (const char*)0;
        }
    }

    pElement->type = TYPE_ARRAY;
    pElement->arrayValue = pHead;
    if( pEnd ) { *pEnd = pCurrChar; }

    return pCurrChar;
}

const char *parseString(const char *pCurrChar, const char **pEnd, Element *pElement)
{
    const char *ptrEnd = (const char *)0;
    char *stringValue = _getString( pCurrChar, &ptrEnd );

    if( !stringValue ) { return (const char *)0; }

    pElement->type = TYPE_STRING;
    pElement->stringValue = stringValue;

    if( pEnd ) { *pEnd = ptrEnd; }
    return ptrEnd;
}

const char *parseNumber(const char *pCurrChar, const char **pEnd, Element *pElement)
{
    // Check Octal, Hex, Decimal or Floating Point Number
    char *ptrEnd = (char *)0;
    const char *pTmp = pCurrChar;

    if( *pTmp == '+' || *pTmp == '-' ) { ++pTmp; }
    if( pTmp[0] == '0' && (pTmp[1] == 'x' || isdigit(pTmp[1])) ) { // Octal || Hex
        pElement->type = TYPE_INT_NUMBER;
        pElement->iNumberValue = (int64_t)strtoll(pCurrChar, &ptrEnd, 0);
    }
    else if( isdigit(pTmp[0]) ) { // 48 < firstChar && firstChar < 58
        double val = strtod( pCurrChar, &ptrEnd );
        if( fabs( ceil(val) - val ) <= DBL_EPSILON ) {
            pElement->type = TYPE_INT_NUMBER;
            pElement->iNumberValue = (int64_t)val;
        }
        else {
            pElement->type = TYPE_DBL_NUMBER;
            pElement->dNumberValue = val;
        }
    }
    else {
        fprintf(stderr, "Unexpected Token Character: %c%c...\n", *pCurrChar, *(pCurrChar+1) );
    }

    if( pEnd ) { *pEnd = ptrEnd ? ptrEnd : pCurrChar; }
    return (const char *)ptrEnd;
}

const char *parseBoolean(const char *pCurrChar, const char **pEnd, Element *pElement)
{
    if( *pCurrChar == 't' && !strncmp( pCurrChar, "true", 4 ) ) {
        pElement->type = TYPE_BOOLEAN;
        pElement->iNumberValue = 1;
        if( pEnd ) { *pEnd = pCurrChar + 4; }
        return pCurrChar + 4;
    }
    else if( *pCurrChar == 'f' && !strncmp( pCurrChar, "false", 5 ) ) {
        pElement->type = TYPE_BOOLEAN;
        pElement->iNumberValue = 0;
        if( pEnd ) { *pEnd = pCurrChar + 5; }
        return pCurrChar + 5;
    }
    // else
    fprintf(stderr, "Unexpected Token Character: %c%c...\n", *pCurrChar, *(pCurrChar+1) );
    if( pEnd ) { *pEnd = pCurrChar; }
    return (const char *)0;
}

const char *parseNull(const char *pCurrChar, const char **pEnd, Element *pElement)
{
    if( !strncmp( pCurrChar, "null", 4 ) ) {
        pElement->type = TYPE_NULL;
        pElement->iNumberValue = 0;
        if( pEnd ) { *pEnd = pCurrChar + 4; }
        return pCurrChar + 4;
    }
    // else
    fprintf(stderr, "Unexpected Token Character: %c%c...\n", *pCurrChar, *(pCurrChar+1) );
    if( pEnd ) { *pEnd = pCurrChar; }
    return (const char *)0;
}


void resetElement(Element *pElement)
{
    switch( pElement->type )
    {
        // No Additional Allocation
        case TYPE_NULL:
        case TYPE_DBL_NUMBER:
        case TYPE_INT_NUMBER:
        case TYPE_BOOLEAN:
            break;

        // Release Once
        case TYPE_STRING:
        {
            free(pElement->stringValue);
        }
        break;

        case TYPE_OBJECT:
        {
            ObjectNode *pNode = pElement->objectValue;
            while( pNode ) {
                ObjectNode *pDelNode = pNode;
                pNode = pNode->next;

                resetElement( &(pDelNode->element) );
                free(pDelNode->key);
                free(pDelNode);
            }
        }
        break;

        case TYPE_ARRAY:
        {
            ArrayNode *pNode = pElement->arrayValue;
            while( pNode ) {
                ArrayNode *pDelNode = pNode;
                pNode = pNode->next;

                resetElement( &(pDelNode->element) );
                free(pDelNode);
            }
        }
        break;
    }

    pElement->type = 0;
    pElement->iNumberValue = 0;
}

void printElementSimple(const Element *pElement)
{
    switch( pElement->type )
    {
        case TYPE_NULL:
            printf("null");
            break;

        case TYPE_STRING:
            printf("'%s'", pElement->stringValue);
            break;

        case TYPE_INT_NUMBER:
            printf("%lld", pElement->iNumberValue);
            break;

        case TYPE_DBL_NUMBER:
            printf("%g", pElement->dNumberValue);
            break;

        case TYPE_BOOLEAN:
            printf( ( pElement->iNumberValue ) ? "true" : "false" );
            break;

        case TYPE_OBJECT:
            if( pElement->objectValue ) { 
                int count = 0;
                ObjectNode *pCurr = pElement->objectValue;
                while( pCurr ) { ++count; pCurr = pCurr->next; }
                printf("{ ...(%d) }", count ); 
            }
            else { printf("{ }"); }
            break;

        case TYPE_ARRAY:
            if( pElement->arrayValue ) {
                int count = 0;
                ArrayNode *pCurr = pElement->arrayValue;
                while( pCurr ) { ++count; pCurr = pCurr->next; }
                printf("[ ...(%d) ]", count ); 
            }
            else { printf("[ ]");  }
            break;
    }
}

void printElementDepth1(const Element *pElement)
{
    switch( pElement->type )
    {
        case TYPE_NULL:
            printf("null\n");
            break;

        case TYPE_STRING:
            printf("'%s'\n", pElement->stringValue);
            break;

        case TYPE_INT_NUMBER:
            printf("%lld\n", pElement->iNumberValue);
            break;

        case TYPE_DBL_NUMBER:
            printf("%g\n", pElement->dNumberValue);
            break;

        case TYPE_BOOLEAN:
            printf( ( pElement->iNumberValue ) ? "true\n" : "false\n" );
            break;

        case TYPE_OBJECT:
        {
            ObjectNode *pCurr = pElement->objectValue;
            printf("{");
            while(pCurr) {
                printf("%s:", pCurr->key);
                printElementSimple(&(pCurr->element));
                if(pCurr->next) { printf(", "); }
                pCurr = pCurr->next;
            }
            printf("}\n");
        }
        break;

        case TYPE_ARRAY:
        {
            ArrayNode *pCurr = pElement->arrayValue;
            printf("[");
            while(pCurr) {
                printElementSimple(&(pCurr->element));
                if(pCurr->next) { printf(", "); }
                pCurr = pCurr->next;
            }
            printf("]\n");
        }
        break;

    }
}

void printPadding(int padding) {
    while(padding--) { putc(' ', stdout); }
}

static void _printElementDepthAllImpl(const Element *pElement, int padding)
{
    switch( pElement->type )
    {
        case TYPE_NULL:
            printf("null");
            break;

        case TYPE_STRING:
            printf("\"%s\"", pElement->stringValue);
            break;

        case TYPE_INT_NUMBER:
            printf("%lld", pElement->iNumberValue);
            break;

        case TYPE_DBL_NUMBER:
            printf("%g", pElement->dNumberValue);
            break;

        case TYPE_BOOLEAN:
            printf( ( pElement->iNumberValue ) ? "true" : "false" );
            break;

        case TYPE_OBJECT:
        {
            ObjectNode *pCurr = pElement->objectValue;
            printf("{\n");
            while(pCurr) {
                printPadding(padding + 4);
                printf("\"%s\": ", pCurr->key);
                _printElementDepthAllImpl(&(pCurr->element), padding + 4);
                if(pCurr->next) { printf(", "); }
                printf("\n");
                pCurr = pCurr->next;
            }
            printPadding(padding);
            printf("}");
        }
        break;

        case TYPE_ARRAY:
        {
            ArrayNode *pCurr = pElement->arrayValue;
            printf("[\n");
            while(pCurr) {
                printPadding(padding + 4);
                _printElementDepthAllImpl(&(pCurr->element), padding + 4);
                if(pCurr->next) { printf(", "); }
                printf("\n");
                pCurr = pCurr->next;
            }
            printPadding(padding);
            printf("]");
        }
        break;
    }
}

void printElementDepthAll(const Element *pElement) {
    _printElementDepthAllImpl(pElement, 0);
    putc('\n', stdout);
}