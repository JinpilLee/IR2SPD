int simple(int a, int b, int c, int d, int e) {
  return a + b + c + d + e;
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
  float tmp2 = tmp + 30.0;
  tmp2 = tmp2 / 10.0;
  return tmp2 * 30.0;
}
