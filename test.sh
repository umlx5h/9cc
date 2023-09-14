#!/bin/bash

make

cat <<EOF | gcc -xc -c -o tmp2.o -
#include <stdio.h>

int ret3() { return 3; }
int ret5() { return 5; }
int add(int x, int y) { return x+y; }
int sub(int x, int y) { return x-y; }

int add6(int a, int b, int c, int d, int e, int f) {
  return a+b+c+d+e+f;
}

void hello(void) {
  printf("Hello world\n");
}

EOF

assert() {
  expected="$1"
  input="$2"

  ./chibicc "$input" > tmp.s
  # gcc -static -o tmp tmp.s tmp2.o
  gcc -o tmp tmp.s tmp2.o
  ./tmp
  actual="$?"

  if [ "$actual" = "$expected" ]; then
    echo "$input => $actual"
  else
    echo "$input => $expected expected, but got $actual"
    exit 1
  fi
}

assert 0 'int main() { return 0; }'
assert 42 'int main() { return 42; }'
assert 21 'int main() { return 5+20-4; }'
assert 41 'int main() { return  12 + 34 - 5 ; }'
assert 47 'int main() { return 5+6*7; }'
assert 15 'int main() { return 5*(9-6); }'
assert 4 'int main() { return (3+5)/2; }'
assert 10 'int main() { return -10+20; }'
assert 10 'int main() { return - -10; }'
assert 10 'int main() { return - - +10; }'

assert 0 'int main() { return 0==1; }'
assert 1 'int main() { return 42==42; }'
assert 1 'int main() { return 0!=1; }'
assert 0 'int main() { return 42!=42; }'

assert 1 'int main() { return 0<1; }'
assert 0 'int main() { return 1<1; }'
assert 0 'int main() { return 2<1; }'
assert 1 'int main() { return 0<=1; }'
assert 1 'int main() { return 1<=1; }'
assert 0 'int main() { return 2<=1; }'

assert 1 'int main() { return 1>0; }'
assert 0 'int main() { return 1>1; }'
assert 0 'int main() { return 1>2; }'
assert 1 'int main() { return 1>=0; }'
assert 1 'int main() { return 1>=1; }'
assert 0 'int main() { return 1>=2; }'

assert 3 'int main() { int a; a=3; return a; }'
assert 8 'int main() { int a; int z; a=3; z=5; return a+z; }'

assert 1 'int main() { return 1; 2; 3; }'
assert 2 'int main() { 1; return 2; 3; }'
assert 3 'int main() { 1; 2; return 3; }'

assert 3 'int main() { int foo=3; return foo; }'
assert 8 'int main() { int foo123=3; int bar=5; return foo123+bar; }'

assert 11 'int main() { int a; int b; a=b=3+2*4; return a; }'
assert 3 'int main() { int a; int b; int foo; a=b=foo=3; return b; }'

assert 4 'int main() { int return1234 = 4; return return1234; }'

# if
assert 3 'int main() { if (0) return 2; return 3; }'
assert 3 'int main() { if (1-1) return 2; return 3; }'
assert 2 'int main() { if (1) return 2; return 3; }'
assert 2 'int main() { if (2-1) return 2; return 3; }'

# if else
assert 3 'int main() { if (0) return 2; else return 3; }'
assert 2 'int main() { if (1) return 2; else return 3; }'

assert 150 'int main() { int a=10; if (1) a = a + 5; else a = a + 10; if (0) return a + 10; else a * 10; }'

# while
assert 10 'int main() { int i=0; while(i<10) i=i+1; return i; }'

# for
assert 55 'int main() { int i=0; int j=0; for (i=0; i<=10; i=i+1) j=i+j; return j; }'
assert 3 'int main() { for (;;) return 3; return 5; }'

# block while
assert 55 'int main() { int i=0; int j=0; while(i<=10) {j=i+j; i=i+1;} return j; }'

# block nest
assert 3 'int main() { {1; {2;} return 3;} }'
assert 2 'int main() { {1; {return 2;} return 3;} }'

# function call with no argument
assert 3 'int main() { return ret3(); }'
assert 5 'int main() { return ret5(); }'
assert 6 'int main() { int a = ret3(); return a * 2; }'

assert 10 'int main() { int a; a = 3 + 2 * 1; return a * 2; }'

# function call with argument (1-6)
assert 8 'int main() { return add(3, 5); }'
assert 2 'int main() { return sub(5, 3); }'
assert 21 'int main() { return add6(1,2,3,4,5,6); }'
assert 14 'int main() { int four = 4; int sum = add(four, 3); return sum * 2; }'
assert 5 'int main() { return add(sub(5, 2), 2); }'

# check rsp alignment in function call
assert 2 'int main() { hello(); return 2; }'
assert 2 'int main() { int padding = 1; hello(); return 2; }'

# function definition with 0 argument
assert 5 'int main() { return foo(); } int foo() { return 3 + 2; }'
assert 6 'int foo() { int a = 1; int b = 2; return a + b; } int main() { int a = foo(); return a * 2; }'

# function definition with up to 6 arguments
assert 7 'int my_add(int i, int j) { int a = 2; return a + i + j; } int main() { return my_add(2, 3); }'
assert 21 'int my_add6(int a, int b, int c, int d, int e, int f) { return a + b + c + d + e + f; } int main() { return my_add6(1, 2, 3, 4, 5, 6); }'

assert 7 'int unused(int i, int j, int k) { int a = 2; return a + j + k; } int main() { return unused(1, 2, 3); }'

# recursive fibonacci 
assert 1 'int fib(int n) { if (n < 2) { return n; } return fib(n-2) + fib(n-1); } int main() { return fib(1); }'
assert 55 'int fib(int n) { if (n < 2) { return n; } return fib(n-2) + fib(n-1); } int main() { return fib(10); }'
assert 233 'int fib(int n) { if (n < 2) { return n; } return fib(n-2) + fib(n-1); } int main() { return fib(13); }'

# unary &, *
assert 3 'int main() { int x=3; *&x; }'
# assert 3 'int main() { int x=3; int *y=&x; z=&y; return **z; }'
assert 5 'int main() { int x=3; int y=5; return *(&x+8); }'
assert 3 'int main() { int x=3; int y=5; return *(&y-8); }'
# assert 5 'int main() { int x=3; int *y=&x; *y=5; return x; }'
assert 7 'int main() { int x=3; int y=5; *(&x+8)=7; return y; }'
assert 7 'int main() { int x=3; int y=5; *(&y-8)=7; return x; }'

echo OK
