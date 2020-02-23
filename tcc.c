#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// トークンの種類
typedef enum
{
    TK_RESERVED, // 記号
    TK_NUM,      // 整数トークン
    TK_EOF,      // 入力の終わりを表すトークン
} TokenKind;

typedef struct Token Token;

// トークン型
struct Token{
    TokenKind kind; // トークンの型
    Token    *next; // 次の入力トークン
    int       val;  // kindがTK_NUMの場合、その数値
    char     *str;  // トークン文字列
};

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
bool is_expected_token( char op, Token *i_token )
{
    if( i_token->kind != TK_RESERVED ||
        i_token->str[0] != op )
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
    
/*
  次のトークンが期待している記号の時には、トークンを読み進めて真を返す。
  それ以外の場合には偽を返す。
 */
bool consume( char op )
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
void expect( char op )
{
    if( !is_expected_token( op, token ) )
    {
        error("not '%c'.", op);
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
        error("not number.");
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
Token *new_token( TokenKind kind, Token *cur, char* str )
{
    Token *tok = calloc(1, sizeof(Token));
    tok->kind = kind;
    tok->str = str;
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
        if( *p == '+' || *p == '-' )
        {
            cur = new_token(TK_RESERVED, cur, p++);
            continue;
        }

        // 数字も個別にトークンとして登録
        if( isdigit(*p) )
        {
            cur = new_token(TK_NUM, cur, p);
            cur->val = strtol( p, &p, 10);
            continue;
        }

        // 解釈できない系列はここでエラーに落とす。
        error("トークナイズできません。");
    }

    // EOFは必ず要素として持っているように。
    new_token(TK_EOF, cur, p);
    return head.next;
}

int main( int argc, char **argv )
{
    if( argc != 2 )
    {
        fprintf( stderr, "invalid argments num.\n" );
        return 1;
    }

    //トークナイズする。
    // tokenは出現順にリスト構造をとる。ここで先頭の要素がtokenに代入される。
    token = tokenize( argv[1] );

    // アセンブラの出力
    printf(".intel_syntax noprefix\n");
    printf(".global main\n");
    printf("main:\n");

    // 最初に数字がきている前提
    printf("  mov rax, %d\n", expect_number());

    while( !at_eof() )
    {
        if( consume( '+' ) )
        {
            printf("  add rax, %d\n", expect_number());
            continue;
        }

        // +さもなくば-
        expect( '-' );
        printf("  sub rax, %d\n", expect_number());
        
    }

    printf("  ret\n");
    return 0;
}
