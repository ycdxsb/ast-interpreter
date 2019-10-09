extern int GET();
extern void * MALLOC(int);
extern void FREE(void *);
extern void PRINT(int);

int main() {
   int a=12;
   int b=-13;
   if(a<b){
      PRINT(a);
   }else{
      PRINT(b);
   }
}
