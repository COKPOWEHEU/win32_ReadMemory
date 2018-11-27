//тестовая программа для проверки основной. Кучу раз выводит строку в stderr, дополнительно печатает ее адрес, по которому можно найти и адрес переменной i (+20)
#include <stdio.h>
#include <windows.h>

const int max = 100000;

int main(){
  char str[] = "Some string to test";
  unsigned int i;
  printf("%p\n", str);
  for(i=0; i<max; i++){
    fprintf(stderr, "\r%i / %i %s", i, max, str);
    fflush(stderr);
    Sleep(1);
  }
  fprintf(stderr, "\n");
}
