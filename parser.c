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

// プログラム全体を保存するための、グローバル変数。
// 文から作ったASTのROOTを順番に入れていく。
Node *code[CodeSize];

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
  アルファベット？
 */
bool is_alpha( char c )
{
   return 'a' <= c && c <= 'z'
       || 'A' <= c && c <= 'Z';
 }
/*
  アルファベットまたは数値またはアンダーバー?
 */
bool is_alnum( char c )
{
    return is_alpha( c )
        || isdigit( c )
        || c == '_';
}

/*
  isdigitと同じノリで、先頭文字を見て変数かどうかを判定。
 */
bool is_ident(char c)
{
    return is_alpha( c )
        || c == '_';
}

/*
  strtolと同じノリで識別子名の長さだけ、ポインタを進める。
  @return 識別子の文字数
*/
int str_to_ident( char *start, char **end )
{
    int count = 0;
    if( NULL != start && NULL != end )
    {
        // 1文字目だけは特別対応
        if( is_ident( *start ) )
        {
            ++count;
            ++start;
            for( ; is_alnum( *start ); ++start, ++count ){}
        }else{
            error_at( start, "ident is not ident ..." );
        }
        for( ; is_ident(*start ); ++start, ++count ){}
        *end = start;
    }else{
        // NULLがきてる時点でフォロー不能なので即終了で。
        error("#include <string.h> is null.");
    }
    return count;
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
        if( *p == '=' )
        {
            if( *(p+1) == '=' )
            {
                cur = new_token(TK_RESERVED, cur, p, 2);
                p += 2;
            }else
            {
                // それ以外は代入と判定。
                cur = new_token(TK_RESERVED, cur, p++, 1);
            }
            
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
                 || *p == ';'
            )
        {
            cur = new_token(TK_RESERVED, cur, p++, 1);
            continue;
        }

        // return
        if( strncmp( p, "return", 6 ) == 0
            && !is_alnum( *(p+6) ) //  returnXXXみたいな識別子を間違えてトークナイズしないように。
            )
        {
            // 予約語トークンとして処理
            cur = new_token( TK_RESERVED, cur, p, 0 );
            cur->len = 6;
            p += 6;
            continue;
        }
            
        // 小文字のa-z1文字を変数として使えるように。
        if( is_ident( *p ) )
        {
            cur = new_token(TK_IDENT, cur, p, 1);
            cur->len = str_to_ident( p, &p );
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
        error_at(p, "トークナイズできません。(*p = %c(%d))", *p, *p);
    }

    // EOFは必ず要素として持っているように。
    new_token(TK_EOF, cur, p, 0);
    return head.next;
}

// トークンの系列をデバッグ表示
void dump_tokens(Token *i_token)
{
    for( Token *a_token = i_token; a_token; a_token = a_token->next )
    {
        fprintf(stderr, "token : %d\n", a_token->kind);
    }
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

// ローカル変数(のリスト)
// 常に先頭を指す実装
LVar *locals;

/*
  変数を名前で検索する。見つからなかった場合はNULLを返す。
 */
LVar *find_lvar(Token *tok)
{
    for ( LVar *var = locals; var; var = var->next )
    {
        if( var->len == tok->len && !memcmp( tok->str, var->name, var->len ) )
        {
            return var;
        }
    }

    return NULL;
}

/*
  次のトークンが期待している記号の時には、トークンを読み進めて真を返す。
  それ以外の場合には偽を返す。

  consumeは消費しない場合も処理を進める場合に使う。
  逆に消費しないと継続できないような場合はexpectを使って、失敗したらエラーで止める。
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
  次のトークンが変数の場合は1つ進めてそれを返す。
  トークンでないときはNULLを返す。
 */
Node *consume_ident()
{
    if( token->kind != TK_IDENT )
    {
        return NULL;
    }
    
    // まずはノードを作る
    Node *node = calloc(1, sizeof(Node));
    node->kind = ND_LVAR;

    LVar *lvar = find_lvar( token );
    if( lvar )
    {
        // すでに追加済みの変数であれば、アクセスに必要なオフセットを設定しておく。
        node->offset = lvar->offset;
    }else{
        // ない場合は作って初期設定をおこなう
        lvar = calloc(1, sizeof(LVar));
        lvar->next = locals;
        lvar->name = token->str;
        lvar->len  = token->len;
        lvar->offset = locals == NULL ? 0 : locals->offset + 8;
        node->offset = lvar->offset;

        // 最終的に先頭にインサート。offset的には先頭ほど値が大きくなる。
        locals = lvar;

        // callocで０初期化済みだが念のため
        node->rhs = NULL;
        node->lhs = NULL;
    }

    token = token->next;
    return node;
}

/*
  次のトークンが期待している記号の時には、トークンを1つ読み進める。
  それ以外の場合にはエラーを報告する。
 */
void expect( char *op )
{
    if( !is_expected_token( op, token ) )
    {
        error_at(token->str, "expected '%c'(%d), but '%c'(%d).",
                 *op, *op, *token->str, *token->str);
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

// 構文規則に沿って関数を定義(再帰下降構文解析法の実装)

// 前方宣言
// expr = assign
Node *expr();

//primary = num | ident | "(" expr ")"
Node *primary()
{
    Node *node = consume_ident();
    if( node )
    {
        return node;
    }
        
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

// assign = equality ("=" assign)?
Node *assign()
{
    Node *node = equality();
    if( consume("=") )
    {
        node = new_node( ND_ASSIGN, node, assign() );
    }
    return node;
}

// expr = assign
Node *expr()
{
    // この層は冗長な気がするけど、意味のわかりやすさのために残してある？
    return assign();
}

// stmt       = expr ";"
//            = | return expr ";"
Node *stmt()
{
    Node *node = NULL;
    if( consume("return") )
    {
        node = new_node( ND_RETURN, expr(), NULL );
    }else{
        node = expr();
    }
    expect(";");
    return node;
}

// 文ごとにASTに。
// program    = stmt*
void program()
{
    int i = 0;
    while( !at_eof() )
    {
        code[ i++ ] = stmt();
    }
    code[ i ] = NULL;
}
