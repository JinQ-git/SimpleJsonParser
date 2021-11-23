#include <stdio.h>
#include <stdlib.h>
#include "jsonParser.h"

//
// From File
//

int main(void)
{
    const char* FILE_PATH = "Fox.gltf";

    long  fileSize     = 0;
    char *fileContents = (char *)0;

    // Read Json File
    {
        FILE *fp = fopen( FILE_PATH, "rb" );
        if( !fp ) {
            fprintf(stderr, "File Open Error\n");
            return -1;
        }

        if( fseek(fp, 0L, SEEK_END) ) {
            fprintf( stderr, "File Error: fseek() return non-zero\n");
            fclose(fp);
            return -1;
        }
        fileSize = ftell(fp);
        fseek(fp, 0L, SEEK_SET); // rewind(fp);

        if( fileSize < 1 ) {
            fprintf( stderr, "File Error: File size is less equal zero (%ld)\n", fileSize );
            fclose(fp);
            return -1;
        }

        fileContents = (char *)calloc( (size_t)fileSize+1, sizeof(char) );
        if( !fileContents ) {
            fprintf( stderr, "Memory Allocation Failed: calloc() return nullptr\n" );
            fclose(fp);
            return -1;
        }

        long readBytes = (long)fread( fileContents, sizeof(char), fileSize, fp );
        fclose(fp);
        if( readBytes != fileSize ) {
            fprintf( stderr, "File Read Error: only read %ld bytes out of %ld bytes\n", readBytes, fileSize );
            free( fileContents );
            return -1;
        }
    }

    // Start parse Here!!
    Element rootElement = { 0, };
    int resultCode = jsonParser( &rootElement, fileContents );

    // return 0 if success
    if( !resultCode ) {
        printElementDepthAll( &rootElement );
    }

    free(fileContents);
    resetElement(&rootElement);
    return resultCode;
}

