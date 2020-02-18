#include <stdio.h>
#include <stdlib.h>

int main( int argc, char **argv )
{
    if( argc != 2 )
    {
        fprintf( stderr, "invalid argments num.\n" );
        return 1;
    }

    char *p = argv[1];

    // アセンブラの出力
    printf(".intel_syntax noprefix\n");
    printf(".global main\n");
    printf("main:\n");

    // 最初に数字がきている前提
    printf("  mov rax, %ld\n", strtol(p, &p, 10));

    while( *p )
    {
        if( *p == '+' )
        {
            // 続く数値を取り出して足し合わせるコード
            ++p;
            printf("  add rax, %ld\n", strtol(p, &p, 10));
            continue;
        }

        if( *p == '-' )
        {
            // 続く数値を取り出して引き算するコード
            ++p;
            printf("  sub rax, %ld\n", strtol(p, &p, 10));
            continue;
        }

        fprintf(stderr, "unexpected character: '%c'\n", *p);
        return 1;
    }

    printf("  ret\n");

    return 0;
}
