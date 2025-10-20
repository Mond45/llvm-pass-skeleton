#include <iostream>

extern "C" void bar(int);

extern "C" void foo(int n) {
  if (n == 0)
    return;
  bar(n - 1);
}

extern "C" void bar(int n) {
  if (n == 0)
    return;
  foo(n - 1);
}

int main() {
  foo(5);
  std::cout << "abc\n";
}
