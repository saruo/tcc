/**
 * tccで使う機能を宣言。
 */

// トークンの種類
typedef enum
{
    TK_RESERVED, // 記号(予約後)
    TK_IDENT,    // 識別子
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

typedef struct LVar LVar;

// ローカル変数の型
struct LVar
{
    LVar *next; // 次の変数かNULL
    char *name; // 変数名
    int   len;  // 名前の長さ
    int   offset; // RBPからのオフセット(RBPって？)
};

// ローカル変数(のリスト)
// 常に先頭を指す実装
extern LVar *locals;

// 抽象構文木のノードの種類
typedef enum {
    ND_EQU,    // ==
    ND_NEQ,    // !=
    ND_LTH,    // <
    ND_LEQ,    // <=
    ND_ADD,    // +
    ND_SUB,    // -
    ND_MUL,    // *
    ND_DIV,    // /
    ND_ASSIGN, // =
    ND_LVAR,   // ローカル変数
    ND_NUM,    // 整数
    ND_RETURN, // return
    ND_IF,     // if
    ND_IFBODY, // if本体
    ND_ELSE,   // else
    ND_WHILE,  // while
    ND_FOR,    // for
    ND_FOR_INIT, // for初期化
    ND_FOR_COND, // for条件
    ND_FOR_UPD,  // for更新
} NodeKind;

typedef struct Node Node;

// 抽象構文木のノードの型
struct Node
{
    NodeKind kind; // ノードの型
    Node    *lhs;  // 左辺
    Node    *rhs;  // 右辺
    int      val;  // kindがND_NUMの場合のみ使う
    int      offset; // kindがND_LVARの場合のみ使う
};


// -- parser.c --

// トークン系列の先頭ポインタ
extern Token *token;

// エラー出力用に開始位置を保持する。
extern char *user_input;

// プログラム全体を保存するための、グローバル変数。
// 文から作ったASTのROOTを順番に入れていく。
#define CodeSize 100
extern Node *code[CodeSize];

// エラーを報告するための関数。
void error( char *fmt, ... );

/*
  入力文字列pをトークナイズしてそれを返す。
*/
Token *tokenize( char *p );

// トークンの系列をデバッグ表示
void dump_tokens(Token *i_token);

// 文ごとにASTに。
// program    = stmt*
void program();


// -- codegen.c --
/*
  ASTからアセンブリコードを出力
 */
void gen( Node *node, int layer );
