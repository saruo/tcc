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
        if( *p == '+' || *p == '-' ||  *p == '*' || *p == '/'
            || *p == '(' || *p == ')' )
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
        error_at(p, "トークナイズできません。");
    }

    // EOFは必ず要素として持っているように。
    new_token(TK_EOF, cur, p);
    return head.next;
}

// ここからは構文解析の処理

// 抽象構文木のノードの種類
typedef enum {
    ND_ADD, // +
    ND_SUB, // -
    ND_MUL, // *
    ND_DIV, // /
    ND_NUM, // 整数
} NodeKind;

typedef struct Node Node;

// 抽象構文木のノードの型
struct Node
{
    NodeKind kind; // ノードの型
    Node    *lhs;  // 左辺
    Node    *rhs;  // 右辺
    int      val;  // kindがND_NUMの場合のみ使う
};

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

// 前方宣言
Node *expr();

//primary = num | "(" expr ")"
Node *primary()
{
    // 次のトークンが"("なら"(" expr ")"のはず。
    if( consume('(' ) )
    {
        Node *node = expr();
        expect(')');
        return node;
    }

    // それ以外なら数値のはず
    return new_node_num( expect_number() );
}

// 単行演算のための規則
// unary = ("+" | "-")? unary
Node *unary()
{
    if( consume('+') )
    {
        return unary();
    }
    else if ( consume('-') )
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

    for(;;)
    {
        if( consume('*') )
        {
            node = new_node( ND_MUL, node, unary() );
        }
        else if( consume('/') )
        {
            node = new_node( ND_DIV, node, unary() );
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
    Node *node = mul();

    for(;;)
    {
        if( consume('+') )
        {
            node = new_node(ND_ADD, node, mul());
        }
        else if( consume('-') )
        {
            node = new_node(ND_SUB, node, mul());
        }
        else
        {
            return node;
        }
    }
}

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
    }

    printf("  push rax\n");
}

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
