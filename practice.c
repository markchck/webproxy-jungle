#include <stdio.h>
void parse_uri(int *port);

int main(void){
  int port;
  port = 1;
  printf("넘기기전 port의 주소 : %p\n", &port);
  parse_uri(&port);
}

void parse_uri(int *port){
    printf("넘긴후 port의 주소 : %p\n", port);
    // printf("%i\n", *port);
}