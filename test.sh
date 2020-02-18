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
try 0 0
try 42 42

echo OK

