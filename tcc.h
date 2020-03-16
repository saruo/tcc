/**
 * tccで使う機能を宣言。
 */

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
    int       len;  // トークン文字列の長さ
};

// 抽象構文木のノードの種類
typedef enum {
    ND_EQU, // ==
    ND_NEQ, // !=
    ND_LTH, // <
    ND_LEQ, // <=
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

// -- parser.c --

// トークン系列の先頭ポインタ
extern Token *token;

// エラー出力用に開始位置を保持する。
extern char *user_input;

/*
  入力文字列pをトークナイズしてそれを返す。
*/
Token *tokenize( char *p );

// expr = mul ("+" mul | "-" mul)*
Node *expr();



// -- codegen.c --
/*
  ASTからアセンブリコードを出力
 */
void gen( Node *node, int layer );
