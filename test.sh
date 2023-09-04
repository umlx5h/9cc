#!/bin/bash

make

cat <<EOF | gcc -xc -c -o tmp2.o -
int ret3() { return 3; }
int ret5() { return 5; }
int add(int x, int y) { return x+y; }
int sub(int x, int y) { return x-y; }

int add6(int a, int b, int c, int d, int e, int f) {
  return a+b+c+d+e+f;
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

init

assert 0 'return 0;'
assert 42 'return 42;'
assert 21 'return 5+20-4;'
assert 41 'return  12 + 34 - 5 ;'
assert 47 'return 5+6*7;'
assert 15 'return 5*(9-6);'
assert 4 'return (3+5)/2;'
assert 10 'return -10+20;'
assert 10 'return - -10;'
assert 10 'return - - +10;'

assert 0 'return 0==1;'
assert 1 'return 42==42;'
assert 1 'return 0!=1;'
assert 0 'return 42!=42;'

assert 1 'return 0<1;'
assert 0 'return 1<1;'
assert 0 'return 2<1;'
assert 1 'return 0<=1;'
assert 1 'return 1<=1;'
assert 0 'return 2<=1;'

assert 1 'return 1>0;'
assert 0 'return 1>1;'
assert 0 'return 1>2;'
assert 1 'return 1>=0;'
assert 1 'return 1>=1;'
assert 0 'return 1>=2;'

assert 3 'a=3; return a;'
assert 8 'a=3; z=5; return a+z;'

assert 1 'return 1; 2; 3;'
assert 2 '1; return 2; 3;'
assert 3 '1; 2; return 3;'

assert 3 'foo=3; return foo;'
assert 8 'foo123=3; bar=5; return foo123+bar;'

assert 11 'a=b=3+2*4; return a;'
assert 3 'a=b=foo=3; return b;'

assert 4 'return1234 = 4; return return1234;'

# if
assert 3 'if (0) return 2; return 3;'
assert 3 'if (1-1) return 2; return 3;'
assert 2 'if (1) return 2; return 3;'
assert 2 'if (2-1) return 2; return 3;'

# if else
assert 3 'if (0) return 2; else return 3;'
assert 2 'if (1) return 2; else return 3;'

assert 150 'a=10; if (1) a = a + 5; else a = a + 10; if (0) return a + 10; else a * 10;'

# while
assert 10 'i=0; while(i<10) i=i+1; return i;'

# for
assert 55 'i=0; j=0; for (i=0; i<=10; i=i+1) j=i+j; return j;'
assert 3 'for (;;) return 3; return 5;'

# block while
assert 55 'i=0; j=0; while(i<=10) {j=i+j; i=i+1;} return j;'

# block nest
assert 3 '{1; {2;} return 3;}'
assert 2 '{1; {return 2;} return 3;}'

# function call with no argument
assert 3 'return ret3();'
assert 5 'return ret5();'
assert 6 'a = ret3(); return a * 2;'

# function call with argument (1-6)
assert 8 'return add(3, 5);'
assert 2 'return sub(5, 3);'
assert 21 'return add6(1, 2, 3, 4, 5, 6);'
assert 14 'four = 4; sum = add(four, 3); return sum * 2;'
assert 5 'return add(sub(5, 2), 2);'

echo OK
