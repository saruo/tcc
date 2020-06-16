/**
 * アセンブラコードを生成する。
 */

#include "tcc.h"

#include <stdio.h>
#include <stdbool.h>
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

bool gen_if(Node *node, int layer)
{
    if( node->kind == ND_IF )
    {
        // まず、条件式の処理を行う
        gen( node->lhs, layer );

        // スタックトップに結果が入っているはず。
        printf("  pop rax\n");
        printf("  cmp rax, 0\n");
        static int jump_label_index = 0; // ラベルはif文の数に合わせて適切に設定されている必要がある。
        // ifの中で一貫した番号である必要がある。ifの中でifみたいなことが起こる可能性があるので、ここで確定させておく。
        ++jump_label_index; 

        // とりあえず、elseのラベルは必ず経由するようにしておく。
        printf("  je  .Lelse%03d\n", jump_label_index);

        // if文本体のコード
        Node *body = node->rhs;
        if( body == NULL || body->kind != ND_IFBODY )
        {
            error("if文の本体がありません。");
            // exitでおわる。
        }
        gen( body->lhs, layer );

        printf(".Lelse%03d:\n", jump_label_index);    
        
        // 右側はelse
        Node *else_node = body->rhs;
        if( else_node != NULL )
        {
            gen( else_node->lhs, layer );
        }

        printf(".Lend%03d:\n", jump_label_index);

        return true;
    }
   return false;
}

bool gen_while(Node *node, int layer)
{
    if( node->kind == ND_WHILE )
    {
        static int jump_label_index = 0; // ラベルはwhile文の数に合わせて適切に設定されている必要がある。
        // ここで足しておく理由はif文処理出力部分のコメントを参照。
        ++jump_label_index; 

        // 繰り返し用ラベル
        printf(".Lwhilebegin%03d:\n", jump_label_index);    

        // 条件式の処理を行う
        gen( node->lhs, layer );

        // スタックトップに結果が入っているはず。
        printf("  pop rax\n");
        printf("  cmp rax, 0\n");
              
        // 偽ならジャンプ
        printf("  je  .Lwhileend%03d\n", jump_label_index);

        gen( node->rhs, layer );

        // 式の結果は捨てる(while分の結果は条件式の結果、ということにする。)
        printf("  pop rax\n");

        printf("  jmp  .Lwhilebegin%03d\n", jump_label_index);

        printf(".Lwhileend%03d:\n", jump_label_index);    

        // 比較結果をpush
        printf("  push rax\n");

        return true;
    }
   return false;
}

bool gen_for(Node *node, int layer)
{
    if( node->kind == ND_FOR )
    {
        static int jump_label_index = 0; // ラベルはwhile文の数に合わせて適切に設定されている必要がある。
        // ここで足しておく理由はif文処理出力部分のコメントを参照。
        ++jump_label_index; 

        // 初期化(最初だけlhsに入っているの、今となってはちょっと気持ち悪いけど。。)
        Node *init = NULL;
        if( init = node->lhs )
        {
            if( init->kind != ND_FOR_INIT )
            {
                error("forのinitが想定されます。");
            }

            printf("# for init\n");
            gen( init->lhs, layer );

            // 式の結果は捨てる(for分の結果は条件式の結果、ということにする。)
            printf("  pop rax\n");

            Node *cond = NULL;
            if( cond = init->rhs )
            {
                if( cond->kind != ND_FOR_COND )
                {
                    error("forのconditionが想定されます。");
                }

                // 繰り返し用ラベル
                printf(".Lforbegin%03d:\n", jump_label_index);    

                // 条件式本体
                gen( cond->lhs, layer );

                // スタックトップに結果が入っているはず。
                printf("  pop rax\n");
                printf("  cmp rax, 0\n");

                // 偽ならジャンプ
                printf("  je  .Lforend%03d\n", jump_label_index);

                // ここに処理本体
                gen( node->rhs, layer );
                // 式の結果は捨てる(for分の結果は条件式の結果、ということにする。)
                printf("  pop rax\n");

                // 最後に更新処理
                Node *upd = NULL;
                if( upd = cond->rhs )
                {
                    if( upd->kind != ND_FOR_UPD )
                    {
                        error("forのupdateが想定されます。");
                    }

                    printf("# for update\n");
                    gen( upd->lhs, layer );
                    // 式の結果は捨てる(for分の結果は条件式の結果、ということにする。)
                    printf("  pop rax\n");
                }
                printf("  jmp  .Lforbegin%03d\n", jump_label_index);

                printf(".Lforend%03d:\n", jump_label_index);    
        
                // 比較結果をpush
                printf("  push rax\n");
                return true;
            }
        }
    }
   return false;
}


/*
  ASTからアセンブリコードを出力
 */
void gen( Node *node, int layer )
{
    ++layer;
    //fprintf(stderr, "layer %d\n", layer);

    // まずはleafとして使われるノード種別
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
    }

    //fprintf(stderr, "trace : %d\n", node->kind);

    // leafをもっているが、leafからのアセンブラの生成が特殊になるパターン
    if ( node->kind ==  ND_ASSIGN )
    {
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
    if( node->kind == ND_RETURN )
    {
        gen( node->lhs, layer );
        // 右側は何もない。

        // 戻り値をraxに書き込んでエピローグと同じ処理を走らせる。
        printf("  pop rax\n");
        printf("  mov rsp, rbp\n");
        printf("  pop rbp\n");
        printf("  ret\n");
        return;
    }
    if( gen_if(node, layer) )
    {
        return;
    }

    if( gen_while(node, layer) )
    {
        return;
    }

    if( gen_for(node, layer) )
    {
        return;
    }

    // 以降はleafをもっているノードの処理

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
