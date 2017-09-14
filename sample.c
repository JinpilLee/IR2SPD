/*
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
*/

float add(float x, float y) {
  return x + y;
}

float mult(float x, float y) {
  return x * y;
}

float ex2(float x) {
  return mult(x, x);
}

float fpga_main(float a, float b) {
  return add(ex2(a), ex2(b));
}
