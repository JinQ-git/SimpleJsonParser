#include <stdio.h>
#include <stdlib.h>
#include "jsonParser.h"

const char *COMPLICATE_OBJECT_STRING;
const char *COMPLICATE_ARRAY_STRING;

int main(void)
{
    const char* JSON_STRINGS[] = {
        // Number Test
        "10",
        "012",
        "0x12",
        "+1.23",
        "-0.125e-8",
        // null
        "null",
        // Boolean
        "true",
        "false",
        // String
        "\"Hello World\"",
        "\"Escape\\nTest\"",
        "\"Unicode: \\u0041 (prints 'A')\"",
        // Array
        "[\"Ford\", \"BMW\", \"Fiat\"]",
        "[1,0.1,\"string\",null,true]",
        // Object
        "{\"name\":\"John\", \"age\":30, \"car\":null}",
        // Complicate Object & Arrays
        COMPLICATE_OBJECT_STRING,
        COMPLICATE_ARRAY_STRING,
        // Comma or Semicolon
        "10,20,30",           // result is 30
        "\"a\";\"b\";\"c\"",  // result is c
        "{\"test\": 1};",     // End with semicolon is allowed
        // Error Test
        "0012.12",            // Number Parsing Error
        "False",              // Capital Letter
        "\"Hello\\\" World",  // not close double quote
        "{'a':1}",            // not allow single quote
        "[1,2,]",             // comma
        "{\"a\":1,}",         // comma
        "{url:\"https://www.google.com\"}", // no double quote
        "NULL",               // Captial Letter
        "true1",              // wrong keyword
        "1,",                 // not allow end with comma
    };
    int LENGTH = sizeof(JSON_STRINGS) / sizeof(JSON_STRINGS[0]);

    Element rootElement = { 0, };
    for(int i = 0; i < LENGTH; i++) {
        printf("Test %d: %s\n", i, JSON_STRINGS[i]);
        printf("-------------------------------------------------\n");
        if( !jsonParser( &rootElement, JSON_STRINGS[i]) ) {
            printElementDepthAll( &rootElement );
        }
        resetElement(&rootElement);
        printf("-------------------------------------------------\n");
    }

    return 0;
}

const char *COMPLICATE_OBJECT_STRING = 
"{ \n\
  \"squadName\": \"Super hero squad\", \n\
  \"homeTown\": \"Metro City\", \n\
  \"formed\": 2016, \n\
  \"secretBase\": \"Super tower\", \n\
  \"active\": true, \n\
  \"members\": [ \n\
    { \n\
      \"name\": \"Molecule Man\", \n\
      \"age\": 29, \n\
      \"secretIdentity\": \"Dan Jukes\", \n\
      \"powers\": [ \n\
        \"Radiation resistance\", \n\
        \"Turning tiny\", \n\
        \"Radiation blast\" \n\
      ] \n\
    }, \n\
    { \n\
      \"name\": \"Madame Uppercut\", \n\
      \"age\": 39, \n\
      \"secretIdentity\": \"Jane Wilson\", \n\
      \"powers\": [ \n\
        \"Million tonne punch\", \n\
        \"Damage resistance\", \n\
        \"Superhuman reflexes\" \n\
      ] \n\
    }, \n\
    { \n\
      \"name\": \"Eternal Flame\", \n\
      \"age\": 1000000, \n\
      \"secretIdentity\": \"Unknown\", \n\
      \"powers\": [ \n\
        \"Immortality\", \n\
        \"Heat Immunity\", \n\
        \"Inferno\", \n\
        \"Teleportation\", \n\
        \"Interdimensional travel\" \n\
      ] \n\
    } \n\
  ] \n\
}\n";

const char *COMPLICATE_ARRAY_STRING = 
"[ \n\
    { \n\
        \"name\": \"Molecule Man\", \n\
        \"age\": 29, \n\
        \"secretIdentity\": \"Dan Jukes\", \n\
        \"powers\": [ \n\
        \"Radiation resistance\", \n\
        \"Turning tiny\", \n\
        \"Radiation blast\" \n\
        ] \n\
    }, \n\
    { \n\
        \"name\": \"Madame Uppercut\", \n\
        \"age\": 39, \n\
        \"secretIdentity\": \"Jane Wilson\", \n\
        \"powers\": [ \n\
        \"Million tonne punch\", \n\
        \"Damage resistance\", \n\
        \"Superhuman reflexes\" \n\
        ] \n\
    } \n\
]\n";
