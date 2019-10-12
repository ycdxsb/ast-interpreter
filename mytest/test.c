extern int GET();
extern void * MALLOC(int);
extern void FREE(void *);
extern void PRINT(int);

int f(int x) {
  PRINT(x);
  return x + 10;
}

int main() {
   int a;
   int b;
   a = -10;
   b = f(-a);
   PRINT(b);
   PRINT(a);
}
