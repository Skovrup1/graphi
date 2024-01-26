#include "lib.cpp"
#include <cstdio>

int main() {
    Foo x = Foo(1);

    printf("hello world %d\n", x.GetCond());

    return 0;
}
