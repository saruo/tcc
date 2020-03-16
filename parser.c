/**
 * 構文解析コード
 */

#include "tcc.h"

#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

// 現在着目しているトークン
Token *token;

/*
  トークンの情報をダンプする。
 */
void dump_token( Token *i_token )
{
    fprintf(stderr, "kind %d\n", i_token->kind);
}
/*
  想定どおりのトークンか？
 */
bool is_expected_token( char *op, Token *i_token )
{
    if( i_token->kind != TK_RESERVED ||
        strlen(op) != i_token->len ||
        0 != memcmp(i_token->str, op, token->len) // 一致するなら0なので
        )
    {
        return false;
    }
    else
    {
        return true;
    }
}

/*
  エラーを報告するための関数。
  printfと同じ引数をとる。
 */
void error( char *fmt, ... )
{
    va_list ap;
    va_start( ap, fmt );
    vfprintf( stderr, fmt, ap );
    fprintf(stderr, "\n");
    exit(1);
}

// エラー出力用に開始位置を保持する。
char *user_input;
/*
  エラー表示のために入力プログラムの文字列にアクセスするポインタが必要。

  @note 今の状態だとソースは1行なので、1行のことだけを想定できれば良い。
*/
void error_at( char *loc, char *fmt, ... )
{
    va_list ap;
    va_start(ap, fmt);
    
    // オフセットを計算
    int pos = loc - user_input;

    fprintf(stderr, "%s\n", user_input);
    fprintf(stderr, "%*s", pos, ""); // pos個の空白を出力
    fprintf(stderr, "^ ");
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    exit(1);
}

/*
  次のトークンが期待している記号の時には、トークンを読み進めて真を返す。
  それ以外の場合には偽を返す。
 */
bool consume( char *op )
{
    if( !is_expected_token( op, token ) )
    {
        return false;
    }

    token = token->next;
    return true;
}

/*
  次のトークンが期待している記号の時には、トークンを1つ読み進める。
  それ以外の場合にはエラーを報告する。
 */
void expect( char *op )
{
    if( !is_expected_token( op, token ) )
    {
        error_at(token->str, "not '%c'.", op);
    }
    token = token->next;
}

/*
  次のトークンが数値の場合、トークンを1つ読み進めてその数値を返す。
  それ以外の場合はエラーを報告する。
*/
int expect_number()
{
    if( token->kind != TK_NUM )
    {
        error_at(token->str, "not number.");
    }
    int val = token->val;
    token = token->next;
    return val;
}

/*
  入力の最後か？
 */
bool at_eof()
{
    return token->kind == TK_EOF;
}

/*
  新しいトークンを作成してcurにつなげる。
 */
Token *new_token( TokenKind kind, Token *cur, char* str, int length )
{
    Token *tok = calloc(1, sizeof(Token));
    tok->kind = kind;
    tok->str = str;
    tok->len = length; // とりあえず一律入れてみる。
    cur->next = tok;

    // TODO : valもなんかの値で初期化しとかないとなんとなく気持ち悪いけど。

    //dump_token( tok );

    return tok;
}

/*
  入力文字列pをトークナイズしてそれを返す。
*/
Token *tokenize( char *p )
{
    Token head;
    head.next = NULL;
    Token *cur = &head;

    while( *p )
    {
        // 空白はスキップ
        if( isspace(*p) )
        {
            p++;
            continue;
        }

        // 演算子はトークンとして登録
        if( *p == '=' && *(p+1) == '=' )
        {
            cur = new_token(TK_RESERVED, cur, p, 2);
            p += 2;
            continue;
        }
        else if( *p == '!' && *(p+1) == '=' )
        {
            cur = new_token(TK_RESERVED, cur, p, 2);
            p += 2;
            continue;
        }
        else if( *p == '<' )
        {
            if( *(p+1) == '=' )
            {
                cur = new_token(TK_RESERVED, cur, p, 2);
                p += 2;
            }else{
                cur = new_token(TK_RESERVED, cur, p++, 1);
            }
            continue;
        }
        else if( *p == '>' )
        {
            if( *(p+1) == '=' )
            {
                cur = new_token(TK_RESERVED, cur, p, 2);
                p += 2;
            }else{
                cur = new_token(TK_RESERVED, cur, p++, 1);
            }
            continue;
        }
        else if( *p == '+' || *p == '-' ||  *p == '*' || *p == '/'
            || *p == '(' || *p == ')'
            )
        {
            cur = new_token(TK_RESERVED, cur, p++, 1);
            continue;
        }

        // 数字も個別にトークンとして登録
        if( isdigit(*p) )
        {
            cur = new_token(TK_NUM, cur, p, 0);
            cur->val = strtol( p, &p, 10);
            continue;
        }

        // 解釈できない系列はここでエラーに落とす。
        error_at(p, "トークナイズできません。");
    }

    // EOFは必ず要素として持っているように。
    new_token(TK_EOF, cur, p, 0);
    return head.next;
}

// ここからは構文解析の処理

/*
  ノード作成
 */
Node *new_node(NodeKind kind, Node *lhs, Node *rhs)
{
    Node *node = calloc(1, sizeof(Node));
    node->kind = kind;
    node->lhs  = lhs;
    node->rhs  = rhs;
    //fprintf(stderr, "new node %d\n", kind);
    return node;
}

/*
  数値ノードの作成
 */
Node *new_node_num(int val)
{
    Node *node = calloc(1, sizeof(Node));
    node->kind = ND_NUM;
    node->val = val;
    node->rhs = NULL;
    node->lhs = NULL;
    //fprintf(stderr, "new num node %d\n", val);
    return node;
}

// 構文規則に沿って関数を定義(再帰下降構文解析法の実装)

//primary = num | "(" expr ")"
Node *primary()
{
    // 次のトークンが"("なら"(" expr ")"のはず。
    if( consume("(" ) )
    {
        Node *node = expr();
        expect(")");
        return node;
    }

    // それ以外なら数値のはず
    return new_node_num( expect_number() );
}

// 単行演算のための規則
// unary = ("+" | "-")? unary
Node *unary()
{
    if( consume("+") )
    {
        return unary();
    }
    else if ( consume("-") )
    {
        // 0 - Xとすることで、符号を反転させる。
        return new_node(ND_SUB, new_node_num(0), unary());
    }
    return primary();
}


// mul  = unary ("*" unary | "/" unary)*
Node *mul()
{
    Node *node = unary();

    for(;;) // * => 0回以上の繰り返し
    {
        if( consume("*") )
        {
            node = new_node( ND_MUL, node, unary() );
        }
        else if( consume("/") )
        {
            node = new_node( ND_DIV, node, unary() );
        }
        else
        {
            return node;
        }
    }
}

// add = mul ( "+" mul | "-" mul )*
Node *add()
{
    Node *node = mul();

    for(;;) // * => 0回以上の繰り返し
    {
        if( consume("+") )
        {
            node = new_node( ND_ADD, node, mul() );
        }
        else if( consume("-") )
        {
            node = new_node( ND_SUB, node, mul() );
        }
        else
        {
            return node;
        }
    }
}

// relational = add ( "<" add | "<=" add | ">" add | ">=" add )*
Node *relational()
{
    Node *node = add();

    for(;;) // * => 0回以上の繰り返し
    {
        if( consume("<") )
        {
            node = new_node( ND_LTH, node, add() );
        }
        else if( consume("<=") )
        {
            node = new_node( ND_LEQ, node, add() );
        }
        if( consume(">") )
        {
            // 専用の演算として定義するのではなく、両辺を入れ替える。
            node = new_node( ND_LEQ, add(), node );
        }
        else if( consume(">=") )
        {
            // 専用の演算として定義するのではなく、両辺を入れ替える。
            node = new_node( ND_LTH, add(), node );
        }
        else
        {
            return node;
        }
    }
}

// equality = relational ("==" relational | "!=" relational)*
Node *equality()
{
    Node *node = relational();

    for(;;) // * => 0回以上の繰り返し
    {
        if( consume("==") )
        {
            node = new_node( ND_EQU, node, relational() );
        }
        else if( consume("!=") )
        {
            node = new_node( ND_NEQ, node, relational() );
        }
        else
        {
            return node;
        }
    }
}

// expr = mul ("+" mul | "-" mul)*
Node *expr()
{
    // この層は冗長な気がするけど、意味のわかりやすさのために残してある？
    return equality();
}

