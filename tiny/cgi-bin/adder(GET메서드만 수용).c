/*
 * form-adder.c - a minimal CGI program that adds two numbers together
 */
#include "../csapp.h"

int main(void) {
  char *buf, *p, *second_args_start, *first_args_start;
  char arg1[MAXLINE], arg2[MAXLINE], content[MAXLINE];
  int n1=0, n2=0;

  /* Extract the two arguments */
  // if ((buf = getenv("QUERY_STRING")) != NULL) {
  //   // buf = ?first=1&second=2
  //   p = strchr(buf, '&');
  //   // *p = '\0';
  //   second_args_start = strchr(p, '=');
  //   strcpy(arg2, second_args_start+1);

  //   first_args_start= strchr(buf, '=');
  //   strcpy(arg1, first_args_start+1);
    
  //   n1 = atoi(arg1);
  //   n2 = atoi(arg2);
  // }

  if ((buf = getenv("QUERY_STRING")) != NULL) {
    // buf = ?first=1&second=2
    p = strchr(buf, '&');
    *p = '\0';
    sscanf(buf, "first=%d", &n1);
    sscanf(buf, "second=%d", &n2);
  }

  //석규 방식
  // if ((buf = getenv("QUERY_STRING")) != NULL)
  // // // if ((buf = "?first=1&second=2") != NULL) {
  // {
  //   p = strchr(buf, '&');
  //   // *p = '\0';
  //   strcpy(arg1, buf);
  //   strcpy(arg2, p+1);

  //   p = strchr(arg1, '=');
  //   *p = '\0';
  //   strcpy(arg1, p+1);
    
  //   p = strchr(arg2, '=');
  //   *p = '\0';
  //   strcpy(arg2, p+1);

  //   n1 = atoi(arg1);
  //   n2 = atoi(arg2);
  // }


  /* Make the response body */
  // printf("%s,%s", buf, p);

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
  
  printf("%s", content);
  fflush(stdout);

  exit(0);
}