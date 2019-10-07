extern int GET();
extern void * MALLOC(int);
extern void FREE(void *);
extern void PRINT(int);

int main() {
   int i;
   for (i=1;i<10;i=i+1){
       PRINT(i);
   }
}
