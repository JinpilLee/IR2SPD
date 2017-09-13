int simple(int a, int b, int c, int d, int e) {
  return (a + b + c + d + e )* 100;
}

int redundant(int a, int b, int c) {
  int tmp = a + b;
  int tmp2 = a + b;
  return tmp * (a + b) * tmp2 * (a + b) + c;
}

int no_input(void) {
  return 123 + 123 + 456;
}

void no_output(int a) {
  int tmp = 1000;
  a++;
  a += tmp;
  tmp += 2000;
}

float useparam(float tmp) {
  float tmp2 = tmp + 30;
  tmp2 = tmp2 / 10;
  return tmp2 * 30;
}

#if 0
int myfunc(int x) {
  return x + x * x;
}
#else
extern int myfunc(int x, int y);
#endif

int use_func(int n) {
  return myfunc(n, 123) * (n - 1);
}
