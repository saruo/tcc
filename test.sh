#!/bin/bash

# 単体テスト用のシェルスクリプト

# 単体テスト用のコード
try() {
    expected="$1"
    input="$2"

    ./tcc "$input" > tmp.s
    gcc -o tmp tmp.s
    ./tmp
    actual="$?"

    if [ "$actual" = "$expected" ]; then
        echo "$input => $actual"
    else
        echo "$input => $expected expected, but got $actual"
        exit 1
    fi
}

# 単体テスト
try 0 '0;'
try 42 '42;'
try 142 '100+42;'
try 21 '5+20-4;'
try 41 ' 12 + 34 - 5 ;'
try 47 '5+6*7;'
try 15 '5*(9-6);'
try 4 '(3+5)/2;'
try 3 '--3;'
try 8 '-(3+5) + 16;'
try 15 '-3*+5 + 30;'
try 1 '(-3+5)/2;'
try 1 '1*+1;'
try 1 '1==1;'
try 0 '1!=1;'
try 1 '1 < 2;'
try 0 '1 > 2;'
try 1 '1 <= 1;'
try 0 '1 >= 2;'
try 1 '( 1 >= 2 ) == 0;'
try 11 'a = 10; a+1;'
try 120 'a = 10; z = 110; a+z;'
try 100 'a = 110; z = 10; a-z;'
try 110 'a = 11; z = 10; a*z;'
# まだ符号なし8bitしか扱えない。
# try 1100 'a = 110; z = 10; a*z;'
try 11 'a = 110; z = 10; a/z;'
# まだ負数は扱えない。
# try "-100" 'a = 10; z = 110; a-z;'

# 複数文字識別子
try 120 'ab = 10; zy = 110; ab+zy;'
try 100 'ab = 110; zy = 10; ab-zy;'
try 110 'ab = 11; zy = 10; ab*zy;'
try 11 'ab = 110; zy = 10; ab/zy;'
try 6 'foo = 1; bar = 2 + 3; foo + bar;'
try 6 'foo = 1; bar = 2 + 3; return foo + bar;'
try 5 'foo = 1; return bar = 2 + 3; return foo + bar;'
echo OK

