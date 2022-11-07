#include "../csapp.h"

int main(void){
  char *buf, *p;
  char arg1[MAXLINE],arg2[MAXLINE], content[MAXLINE];
  int n1=0, n2=0;

  /* Extract the two arguments */
  if ((buf = getenv("QUERY_STRING")) != NULL) {
    //buf에는 ?first=1&second=2가 담겨있다.
    // p는 second=2 이게 됨
    p = strchr(buf, '&');
    // buf는 first=1 이게 됨.
    *p = '\0';

    // buf : ?first=1
    sscanf(buf, "first=%d", &n1);
    sscanf(p+1, "second=%d", &n2);
  }

    /* Make the response body */
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
}