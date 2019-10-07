extern int GET();
extern void * MALLOC(int);
extern void FREE(void *);
extern void PRINT(int);

int main() {
   int a=12;
   int b=-13;
   char c = 'a';
   int d = +14;
   PRINT(a);
   PRINT(b);
   PRINT(c);
   PRINT(d);
}
