#include "jsonParser.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h> // strncmp
#include <ctype.h> // isspace()
#include <math.h>  // fabs(), ceil()
#include <float.h> // DBL_EPSILON

//
// Parsing Functions (Trim Left is required)
//
JsonParsingError parseValue(const char *pCurrChar, const char **pEnd, Element *pElement);
JsonParsingError parseObject(const char *pCurrChar, const char **pEnd, Element *pElement);
JsonParsingError parseArray(const char *pCurrChar, const char **pEnd, Element *pElement);
JsonParsingError parseString(const char *pCurrChar, const char **pEnd, Element *pElement);
JsonParsingError parseNumber(const char *pCurrChar, const char **pEnd, Element *pElement);
JsonParsingError parseBoolean(const char *pCurrChar, const char **pEnd, Element *pElement);
JsonParsingError parseNull(const char *pCurrChar, const char **pEnd, Element *pElement);

// Calculate Error Location for Debug?
static void _errorLocation( JsonErrorInfo *pOut, const char *startPos, const char *endPos )
{
    int ln  = 1;
    int col = 1;

    for( const char *currPos = startPos; currPos <= endPos; currPos++ ) {
        if( *currPos == '\n' ) {
            ++ln;    // add line number
            col = 1; // reset column number
        }
        else if( (((unsigned char)*currPos) & 0xC0) != 0x80 ) {
            ++col;
        }
        // else // 10xxxxxx xxxxxxxx -> ignore
    }

    pOut->line = ln;
    pOut->column = col;
    pOut->position = (int)(endPos - startPos);
}

//
// Main Function of Json Parser
//
JsonParsingError parseJsonString(Element *pOutElement, const char *jsonStr, JsonErrorInfo* pOutErrorInfo)
{
    JsonParsingError ret = JPE_NO_ERROR;
    const char *pCurrChar = jsonStr;
    const char *pEnd = (const char *)0;
    
    if( pOutElement && pCurrChar && *pCurrChar )
    {
        ret = parseValue( pCurrChar, &pEnd, pOutElement );
        if( ret == JPE_NO_ERROR ) {
            pCurrChar = pEnd;
            while( isspace( *pCurrChar ) ) { pCurrChar++; }
            if( *pCurrChar ) { 
                pEnd = pCurrChar;
                ret = JPE_SYNTAX_ERROR;
            }
        }
        // else {
        //     if( *pEnd == '\0' && ret != JPE_SYNTAX_ERROR_END ) {
        //         ret = JPE_SYNTAX_ERROR_END;
        //     }
        // }
    }

    if( pOutErrorInfo ) {
        pOutErrorInfo->error = ret;
        _errorLocation( pOutErrorInfo, jsonStr, pEnd );
    }
    
    return ret;
}

static int hex2int(char hexCode)
{
    if( 96 < hexCode && hexCode < 103 ) { return hexCode - 'a' + 10; }
    else if( 64 < hexCode && hexCode < 71 ) { return hexCode - 'A' + 10; }
    else if( 47 < hexCode && hexCode < 58 ) { return hexCode - '0'; }

    return 0;
}

JsonParsingError _getString(char **pOutString, const char *pCurrChar, const char ** ppEnd)
{
    const char *pTmp = pCurrChar + 1;
    int charCount = 0;
    char *stringValue = (char *)0;

    // Not allow single quote string
    if( *pCurrChar != '"' ) { 
        *ppEnd = pCurrChar;
        return JPE_SYNTAX_ERROR;
    }

    // Count Total Characters (utf-8)
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
                        *ppEnd = pTmp;
                        return JPE_SYNTAX_ERROR_UNICODE_ESCAPE;
                    }
                    pTmp += 4;
                    // Calculate Bytes of UTF-8 Conversion
                    if( pTmp[1] != '0' || pTmp[2] > '7' ) { charCount += 2; }      // 0x0800 ~ 0xFFFF
                    else if( pTmp[2] > '0' || pTmp[3] > '7' ) { charCount += 1; }  // 0x0080 ~ 0x07FF
                    // else // 0x0000 ~ 0x007F
                    break;
                case '\0':
                    *ppEnd = pTmp;
                    return JPE_SYNTAX_ERROR_END;
                default:
                    *ppEnd = pTmp;
                    return JPE_SYNTAX_ERROR_STRING_ESCAPE;
            }
        }
        else if( *pTmp == '"' ) { // End of String
            break;
        } 
        pTmp++;
        charCount++;
    }
    if( *pTmp != '"' ) { return JPE_SYNTAX_ERROR_END; }

    stringValue = (char *)calloc(charCount + 1, sizeof(char));
    if( !stringValue ) {
        *ppEnd = pTmp + 1;
        return JPE_OUT_OF_MEMORY;
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

    *ppEnd = pTmp + 1;
    *pOutString = stringValue;
    return JPE_NO_ERROR;
}

JsonParsingError parseValue(const char *pCurrChar, const char **ppEnd, Element *pElement)
{
    // Trim Left
    while( isspace(*pCurrChar) ) { pCurrChar++; }

    switch( *pCurrChar ) 
    {
        // Try parse as Object
        case '{': return parseObject( pCurrChar, ppEnd, pElement );
        // Try parse as Array
        case '[': return parseArray( pCurrChar, ppEnd, pElement );
        // Try parse as String
        case '"': return parseString( pCurrChar, ppEnd, pElement );
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
            return parseNumber( pCurrChar, ppEnd, pElement );
        // Try parse as Boolean
        case 't':
        case 'f': 
            return parseBoolean( pCurrChar, ppEnd, pElement );
        // Try parse as Null
        case 'n':
            return parseNull( pCurrChar, ppEnd, pElement );
        // End of String
        case '\0':
            *ppEnd = pCurrChar;
            return JPE_SYNTAX_ERROR_END;

        default: break; // Token Error
    }

    *ppEnd = pCurrChar;
    return JPE_SYNTAX_ERROR;
}

JsonParsingError parseObject(const char *pCurrChar, const char **ppEnd, Element *pElement)
{
    if( *pCurrChar != '{' ) { 
        *ppEnd = pCurrChar;
        return JPE_SYNTAX_ERROR;
    }
    pCurrChar++;

    const char *ptrEnd = (const char *)0;
    ObjectNode *pHead = (ObjectNode *)0;
    ObjectNode *pTail = (ObjectNode *)0;

    // Check Empty Object
    while( isspace(*pCurrChar) ) { pCurrChar++; }
    if( *pCurrChar == '}' ) {
        // Empty Object
        *ppEnd = pCurrChar + 1;
        pElement->type = TYPE_OBJECT;
        pElement->objectValue = (ObjectNode *)0;
        return JPE_NO_ERROR;
    }

    for(;;) 
    {
        // Get Key
        char *keyString = (char *)0;
        JsonParsingError ret = _getString(&keyString, pCurrChar, &ptrEnd);
        if( ret != JPE_NO_ERROR ) {
            if( ret == JPE_OUT_OF_MEMORY ) { return ret; }
            // String Parse Error -> No String for Key
            *ppEnd = ptrEnd;
            return JPE_SYNTAX_ERROR_OBJECT_KEY;
        }

        pCurrChar = ptrEnd;
        while( isspace(*pCurrChar) ) { pCurrChar++; }
        if( *pCurrChar != ':' ) {
            free(keyString);
            *ppEnd = pCurrChar;
            return (*pCurrChar == '\0') ? JPE_SYNTAX_ERROR_END : JPE_SYNTAX_ERROR_OBJECT_COLON;
        }
        ++pCurrChar; 
        while( isspace(*pCurrChar) ) { pCurrChar++; }

        // Allocate Element
        ObjectNode *pNode = (ObjectNode *)calloc(1, sizeof(ObjectNode));
        if( !pNode ) {
            free(keyString);
            if( pHead ) { // Release
                Element e = { .type = TYPE_OBJECT, .objectValue = pHead };
                resetElement( &e );
            }
            *ppEnd = pCurrChar;
            return JPE_OUT_OF_MEMORY;
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
        ret = parseValue( pCurrChar, &ptrEnd, &(pNode->element) );
        if( ret != JPE_NO_ERROR ) {
            if( pHead ) { // Release
                Element e = { .type = TYPE_OBJECT, .objectValue = pHead };
                resetElement( &e );
            }
            *ppEnd = ptrEnd;
            return ret == JPE_SYNTAX_ERROR_END ? ret : JPE_SYNTAX_ERROR_OBJECT;
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
            *ppEnd = pCurrChar; 
            return (*pCurrChar == '\0') ? JPE_SYNTAX_ERROR_END : JPE_SYNTAX_ERROR_OBJECT_COMMA;
        }
    }

    pElement->type = TYPE_OBJECT;
    pElement->objectValue = pHead;
    *ppEnd = pCurrChar;

    return JPE_NO_ERROR;
}

JsonParsingError parseArray(const char *pCurrChar, const char **ppEnd, Element *pElement)
{
    if( *pCurrChar != '[' ) { 
        *ppEnd = pCurrChar;
        return JPE_SYNTAX_ERROR;
    }
    pCurrChar++;

    const char *ptrEnd = (const char *)0;
    ArrayNode *pHead = (ArrayNode *)0;
    ArrayNode *pTail = (ArrayNode *)0;

    // Check Empty Array
    while( isspace(*pCurrChar) ) { pCurrChar++; }
    if( *pCurrChar == ']' ) {
        // Empty Array
        *ppEnd = pCurrChar + 1;
        pElement->type = TYPE_ARRAY;
        pElement->arrayValue = (ArrayNode *)0;
        return JPE_NO_ERROR;
    }

    // Prasing Array
    for(;;) {
        // Allocate Element
        ArrayNode *pNode = (ArrayNode *)calloc( 1, sizeof(ArrayNode) );
        if( !pNode ) {
            if( pHead ) { // Release
                Element e = { .type = TYPE_ARRAY, .arrayValue = pHead };
                resetElement(&e);
            }
            *ppEnd = pCurrChar;
            return JPE_OUT_OF_MEMORY;
        }
        else { // Append Node
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
        JsonParsingError ret = parseValue( pCurrChar, &ptrEnd, &(pNode->element) );
        if( ret != JPE_NO_ERROR ) {
            if( pHead ) { // Release
                Element e = { .type = TYPE_ARRAY, .arrayValue = pHead };
                resetElement(&e);
            }
            *ppEnd = ptrEnd;
            return (ret == JPE_SYNTAX_ERROR_END) ? ret : JPE_SYNTAX_ERROR_ARRAY;
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
            *ppEnd = pCurrChar;
            return (*pCurrChar == '\0') ? JPE_SYNTAX_ERROR_END : JPE_SYNTAX_ERROR_ARRAY_COMMA;
        }
    }

    pElement->type = TYPE_ARRAY;
    pElement->arrayValue = pHead;
    *ppEnd = pCurrChar;

    return JPE_NO_ERROR;
}

JsonParsingError parseString(const char *pCurrChar, const char **ppEnd, Element *pElement)
{
    char *stringValue = (char *)0; 
    JsonParsingError ret = _getString( &stringValue, pCurrChar, ppEnd );
    if( ret == JPE_NO_ERROR ) {
        pElement->type = TYPE_STRING;
        pElement->stringValue = stringValue;
    }
    return ret;
}

JsonParsingError parseNumber(const char *pCurrChar, const char **ppEnd, Element *pElement)
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

    if( !ptrEnd ) {
        *ppEnd = pCurrChar;
        return JPE_SYNTAX_ERROR;
    }

    *ppEnd = ptrEnd;
    return JPE_NO_ERROR;
}

JsonParsingError parseBoolean(const char *pCurrChar, const char **ppEnd, Element *pElement)
{
    if( *pCurrChar == 't' && !strncmp( pCurrChar, "true", 4 ) ) {
        pElement->type = TYPE_BOOLEAN;
        pElement->iNumberValue = 1;
        *ppEnd = pCurrChar + 4;
        return JPE_NO_ERROR;
    }
    else if( *pCurrChar == 'f' && !strncmp( pCurrChar, "false", 5 ) ) {
        pElement->type = TYPE_BOOLEAN;
        pElement->iNumberValue = 0;
        *ppEnd = pCurrChar + 5;
        return JPE_NO_ERROR;
    }
    
    *ppEnd = pCurrChar;
    return JPE_SYNTAX_ERROR;
}

JsonParsingError parseNull(const char *pCurrChar, const char **ppEnd, Element *pElement)
{
    if( !strncmp( pCurrChar, "null", 4 ) ) {
        pElement->type = TYPE_NULL;
        pElement->iNumberValue = 0;
        *ppEnd = pCurrChar + 4;
        return JPE_NO_ERROR;
    }
    
    *ppEnd = pCurrChar;
    return JPE_SYNTAX_ERROR;
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

void printJsonErrorInfo(const JsonErrorInfo* pInfo, const char *jsonStr)
{
    if( pInfo ) {
        const char *errMsg = (const char *)0;
        switch(pInfo->error)
        {
            case JPE_OUT_OF_MEMORY:
                fprintf(stderr, "Critical Error: Out of memory...\n");
                return;

            case JPE_NO_ERROR:
                fprintf(stderr, "No Error...\n");
                return;
            
            case JPE_SYNTAX_ERROR:
                fprintf(stderr, "Unexpected token '%c' in JSON at position %d\n", jsonStr[pInfo->position], pInfo->position);
                break;
            case JPE_SYNTAX_ERROR_END:
                fprintf(stderr, "Unexpected end of JSON input\n");
                return;
            case JPE_SYNTAX_ERROR_STRING_ESCAPE:
                fprintf(stderr, "Unexpected control character '%c' after '\\' at position %d\n", jsonStr[pInfo->position], pInfo->position);
                break;
            case JPE_SYNTAX_ERROR_UNICODE_ESCAPE:
                fprintf(stderr, "Unexpected unicode 4 hex digits (+U%c%c%c%c) at position %d\n", jsonStr[pInfo->position + 1], jsonStr[pInfo->position + 2], jsonStr[pInfo->position + 3], jsonStr[pInfo->position + 4], pInfo->position);
                break;
            case JPE_SYNTAX_ERROR_ARRAY:
                fprintf(stderr, "Unexpected token '%c' in JSON (while parsing array) at position %d\n", jsonStr[pInfo->position], pInfo->position);
                break;
            case JPE_SYNTAX_ERROR_ARRAY_COMMA:
                fprintf(stderr, "Unexpected token '%c' in JSON (while parsing array) at position %d: ',' is expected\n", jsonStr[pInfo->position], pInfo->position);
                break;
            case JPE_SYNTAX_ERROR_OBJECT:
                fprintf(stderr, "Unexpected token '%c' in JSON (while parsing object) at position %d\n", jsonStr[pInfo->position], pInfo->position);
                break;
            case JPE_SYNTAX_ERROR_OBJECT_KEY:
                fprintf(stderr, "Unexpected token '%c' in JSON (while parsing object) at position %d: `string` for `key` is expected\n", jsonStr[pInfo->position], pInfo->position);
                break;
            case JPE_SYNTAX_ERROR_OBJECT_COLON:
                fprintf(stderr, "Unexpected token '%c' in JSON (while parsing object) at position %d: ':' is expected\n", jsonStr[pInfo->position], pInfo->position);
                break;
            case JPE_SYNTAX_ERROR_OBJECT_COMMA:
                fprintf(stderr, "Unexpected token '%c' in JSON (while parsing object) at position %d: ',' is expected\n", jsonStr[pInfo->position], pInfo->position);
                break;
            default:
                return;
        }   
        fprintf(stderr, "    at Ln %d, Col %d\n", pInfo->line, pInfo->column);
    }
}