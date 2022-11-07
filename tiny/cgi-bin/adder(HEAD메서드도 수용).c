/*
 * form-adder.c - a minimal CGI program that adds two numbers together
 */
#include "../csapp.h"

int main(void) {
  char *buf, *p, *second_args_start, *first_args_start, *method;
  char arg1[MAXLINE], arg2[MAXLINE], content[MAXLINE];
  int n1=0, n2=0;

  /* Extract the two arguments */
  if ((buf = getenv("QUERY_STRING")) != NULL) {
    // buf = ?first=1&second=2
    p = strchr(buf, '&');
    // *p = '\0';
    second_args_start = strchr(p, '=');
    strcpy(arg2, second_args_start+1);

    first_args_start= strchr(buf, '=');
    strcpy(arg1, first_args_start+1);
    
    n1 = atoi(arg1);
    n2 = atoi(arg2);
  }

  /* Make the response body */
  // printf("%s,%s", buf, p);

  // HEAD매서드도 받는 로직
  method= getenv("REQUEST_METHOD");

  sprintf(content, "QUERY_STRING=%s", buf);
  sprintf(content, "Welcome to add.com: ");
  sprintf(content, "%sTHE Internet addition portal.\r\n<p>", content);
  sprintf(content, "%sThe answer is: %d + %d = %d\r\n<p>",
      content, n1, n2, n1 + n2);
  sprintf(content, "%sThanks for visiting!\r\n", content);

  /* Generate the HTTP response */
  printf("Connection: close\r\n");
  printf("Content-length: %d\r\n", (int)strlen(content));
  printf("Content-type: text/html\r\n\r\n");

  //HEAD메서드도 받게하려면
  if(strcasecmp(method,"HEAD") !=0) {
     printf("%s", content);
  }
  fflush(stdout);
  exit(0);
}