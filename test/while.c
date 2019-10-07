extern int GET();
extern void * MALLOC(int);
extern void FREE(void *);
extern void PRINT(int);

int main() {
   int i = 0;
   while(i<13){
      i = i + 1;
      PRINT(i);
   }  
}
