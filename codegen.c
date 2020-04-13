/**
 * アセンブラコードを生成する。
 */

#include "tcc.h"

#include <stdio.h>

/*
  変数のアドレスをスタックにpush
  この後実行するコードでは、変数を扱う(アドレスが入っている)ことを
  前提にコードを構成する必要がある。

  変数は全て左辺値として扱う、という規約をここで規定するようになっている。
 */
void gen_lval(Node *node, int layer )
{
    // 直接変数を参照する以外の場合、
    // 代入の左辺値というユースケースのみが想定されるので、ここでひっかける。
    if( node->kind != ND_LVAR )
    {
        error("代入の左辺値が変数ではありません。");
        // exitで終わる。
    }

    printf("  mov rax, rbp\n");
    printf("  sub rax, %d\n", node->offset);
    printf("  push rax\n");
}

/*
  ASTからアセンブリコードを出力
 */
void gen( Node *node, int layer )
{
    ++layer;
    //fprintf(stderr, "layer %d\n", layer);

    switch( node->kind )
    {
    case ND_NUM:
        printf("  push %d\n", node->val);
        return;

    case ND_LVAR:
        // 一旦アドレスをpushしてから
        gen_lval( node, layer );
        // 値を取得する。
        printf("  pop rax\n");
        printf("  mov rax, [rax]\n");
        printf("  push rax\n");
        return;

    case ND_ASSIGN:
        gen_lval( node->lhs, layer );
        gen( node->rhs, layer );

        printf("  pop rdi\n");
        printf("  pop rax\n");
        printf("  mov [rax], rdi\n");
        // あとは右辺値としてて参照するだけだから、直値を入れておくのか。
        // つまり、代入の戻り値は右辺値になっていて、そこに代入はできない文法になっている、はず。
        printf("  push rdi\n");
        return;
    }

    //fprintf(stderr, "trace : %d\n", node->kind);

    // 左右のノードから先にトラバースする。
    gen( node->lhs, layer );
    gen( node->rhs, layer );

    // popして
    printf("  pop rdi\n");
    printf("  pop rax\n");

    // 演算する。
    switch( node->kind )
    {
    case ND_ADD:
        printf("  add rax, rdi\n");
        break;

    case ND_SUB:
        printf("  sub rax, rdi\n");
        break;

    case ND_MUL:
        printf("  imul rax, rdi\n");
        break;

    case ND_DIV:
        printf("  cqo\n");      // RAXを128bitに拡張してRDX|RAXとしてセット
        printf("  idiv rdi\n"); // (RDX|RAX)/RDI = RAX ... RDX
        break;

        // 比較
    case ND_EQU: // "=="
        printf("  cmp rax, rdi\n");
        printf("  sete al\n");
        printf("  movzb rax, al\n");
        break;

    case ND_NEQ: // "!="
        printf("  cmp rax, rdi\n");
        printf("  setne al\n");
        printf("  movzb rax, al\n");
        break;

    case ND_LTH: // "<"
        printf("  cmp rax, rdi\n");
        printf("  setl al\n");
        printf("  movzb rax, al\n");
        break;

    case ND_LEQ: // "<="
        printf("  cmp rax, rdi\n");
        printf("  setle al\n");
        printf("  movzb rax, al\n");
        break;
    }

    printf("  push rax\n");
}
