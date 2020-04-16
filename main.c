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
    //dump_tokens( token );

    // 文ごとにASTに。

    // 関数に入るときに初期化される想定だと思うが、ひとまずここに置いておく。
    locals = NULL;

    program();

    // アセンブラの出力
    printf(".intel_syntax noprefix\n");
    printf(".global main\n");
    printf("main:\n");

    // プロローグ
    // 変数26個分の領域を確保する。
    printf("  push rbp\n");     // 
    printf("  mov rbp, rsp\n"); // 次のスタックフレームのrbpを指すように。
    int lvar_offset = (locals == NULL ? 0 : locals->offset+8);
    printf("  sub rsp, %d\n", lvar_offset);
    
    // 先頭の式から順にコード生成
    // @todo 要範囲チェック
    for( int i = 0; code[i]; ++i )
    {
        gen( code[i], 0 );

        // 式の評価結果として、スタックに1つの値が残っている
        // はずなので、スタックが煽れないようにポップしておく。
        printf("  pop rax\n");
    }

    // エピローグ
    // 最後の式の結果がRAXに残っているので、それが返り値になる。
    printf("  mov rsp, rbp\n"); // rspとrbpが同じ位置を指す。
    printf("  pop rbp\n");      // rbpに前のスタックフレームのrbpの値を書き込む。
    // スタックトップが前のスタックフレームのrspの位置なので、そこを指すように。
    printf("  ret\n");

    return 0;
}
