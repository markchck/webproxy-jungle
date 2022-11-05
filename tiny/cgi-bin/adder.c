/*
 * adder.c - a minimal CGI program that adds two numbers together
 */
/* $begin adder */
#include "csapp.h"

int main(void) {
  char *buf, *p;
  char arg1[MAXLINE], arg2[MAXLINE], content[MAXLINE]; //MAXLINE은 8192래 (커서 올리면)
  int n1=0, n2=0;

  //쿼리스트링을받아서(
  // https://www.ibm.com/docs/ko/i/7.4?topic=functions-getenv-search-environment-variables
  if((buf = getenv("QUERY_STRING")) != NULL){
    // string은 널문자까지를 한 스트링으로보니까
    // *p에서 &를 널문자로 바꿔서 '문자1' '문자2' 두 개로 나눠줌(왜냐 널문자가 있으면 스트링이니까)
    p = strchr(buf, '&');
    *p = '\0';
    
    // 뒤에를 앞인자로 카피
    strcpy(arg1, buf);
    // p+1은 문자2를 의미
    strcpy(arg2, p+1);
    
    // 아스키(문자)를 인티져로 바꿔줌
    n1=atoi(arg1);
    n2=atoi(arg2);
  }

  //content라는 배열에다가 뒤에 문장을 넣어준다. buf는 %s를 의미하고
  sprintf(content, "QUERY_STRING =%s", buf);
  sprintf(content, "Welcome to add.com: ");
  sprintf(content, "%sTHE Internet addition portal.\r\n <p>", content);
  sprintf(content, "%sThe answer is: %d +%d = %d\r\n <p>", content, n1, n2, n1+n2);
  sprintf(content, "%sThanks for visiting!\r\n", content);

  printf("Connection: close\r\n");
  printf("Content-length: %d\r\n", (int)strlen(content));
  printf("Content-type: text/html\r\n\r\n");
  printf("%s", content);
  fflush(stdout);

  exit(0);
}
/* $end adder */


