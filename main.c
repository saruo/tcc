#include "tcc.h"

#include <stdio.h>


int main( int argc, char **argv )
{
    if( argc != 2 )
    {
        fprintf( stderr, "invalid argments num.\n" );
        return 1;
    }

    // エラー出力ように開始位置をコピー。
    user_input = argv[1];

    //トークナイズする。
    // tokenは出現順にリスト構造をとる。ここで先頭の要素がtokenに代入される。
    token = tokenize( argv[1] );

    // アセンブラの出力
    printf(".intel_syntax noprefix\n");
    printf(".global main\n");
    printf("main:\n");

    // ASTに。
    Node *root = expr();

    // スタックマシンを使う形でアセンブリ化
    gen( root, 0 );

    // 返り値を設定
    printf("  pop rax\n");
    printf("  ret\n");
    return 0;
}
