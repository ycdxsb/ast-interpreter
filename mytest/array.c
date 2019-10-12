extern int GET();
extern void * MALLOC(int);
extern void FREE(void *);
extern void PRINT(int);

int main() {
   int a[3];
   a[0] = 1;
   a[1] = a[0];
   PRINT(a[0]);
}
