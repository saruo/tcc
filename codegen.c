/**
 * アセンブラコードを生成する。
 */

#include "tcc.h"

#include <stdio.h>

/*
  ASTからアセンブリコードを出力
 */
void gen( Node *node, int layer )
{
    ++layer;
    //fprintf(stderr, "layer %d\n", layer);

    if( node->kind == ND_NUM )
    {
        printf("  push %d\n", node->val);
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
