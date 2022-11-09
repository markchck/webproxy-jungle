#include <stdio.h>
#include "csapp.h"

/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

/* User Agent 헤더를 상수로 정의하여 코드의 가독성을 해치지 않도록 합니다. */
static const char *user_agent_hdr =
    "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 "
    "Firefox/10.0.3\r\n";

/* PROTOTYPES */
int doit(int fd, char *host, char *port, char *new_buf);
int parse_uri(char *uri, char *host, char *port, char *new_uri);
int send_and_receive(int clientfd, int connfd, char *buf);
void sigchld_handler(int sig);


/*          구현 방법 :
 * port로 들어오는 연결에 대한 listen을 시작한다. (port는 프로그램이 실행될때 CLI인자로 들어옴)
 * Connection이 만들어지면, client의 request를 읽고 parse해야함.
 * Client가 유효한 HTTP request를 보냈는가? 확인하고
 * 적절한 웹 서버로 connection을 만들 수 있다.
 * 그리고 client가 specified했던 object를 요청하자...
 */
int main(int argc, char **argv) {
  int listenfd, connfd, clientfd;
  char hostname[MAXLINE], port[MAXLINE];
  char host_for_server[MAXLINE], port_for_server[MAXLINE], buf_for_server[MAXLINE];
  socklen_t clientlen;
  struct sockaddr_storage clientaddr;

  /* Check command line args */
  if (argc != 2) {
    fprintf(stderr, "usage: %s <port>\n", argv[0]);
    exit(1);
  }

  /* Open listen */
  listenfd = Open_listenfd(argv[1]);
  while (1) {
    clientlen = sizeof(clientaddr);
    connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen);
    if (Fork() == 0) {  /* 동시성 처리를 위해 자식 프로세스 생성 */
      Close(listenfd);

      Getnameinfo((SA *)&clientaddr, clientlen, hostname, MAXLINE, port, MAXLINE, 0);
      printf("Accepted connection from (%s, %s)\n", hostname, port);

      /* valid check here */
      if (!(doit(connfd, host_for_server, port_for_server, buf_for_server))) {
        clientfd = Open_clientfd(host_for_server, port_for_server);
        send_and_receive(clientfd, connfd, buf_for_server);
        Close(clientfd);
      }
      else printf("URI Error: 올바르지 않은 URI입니다.\n");

      Close(connfd);
      exit(0);
    }
    Close(connfd);
  }

  return 0;
}

/*
 * request를 읽고, server에게 보내기 알맞은 형식으로 변환합니다.
 * request가 invalid하다면 -1을, valid하다면 0을 return합니다.
 */
int doit(int fd, char *host, char *port, char *new_buf) {
  char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], new_uri[MAXLINE], version[MAXLINE];
  char new_version[] = "HTTP/1.0";
  rio_t rio;
  
  Rio_readinitb(&rio, fd);
  Rio_readlineb(&rio, buf, MAXLINE);
  sscanf(buf, "%s %s %s", method, uri, version);

  if((parse_uri(uri, host, port, new_uri)) == -1)
    return -1;
  printf("uri: %s, host: %s, port: %s, new_uri: %s\n", uri, host, port,new_uri);
  sprintf(new_buf, "%s %s %s\r\n", method, new_uri, new_version);

  while (strcmp(buf, "\r\n")) {
    Rio_readlineb(&rio, buf, MAXLINE);
    sprintf(new_buf, "%s%s", new_buf, buf);
  }
  
  return 0;
}

/*
 * uri를 parse합니다.
 * host, port, new_uri가 추출됩니다.
 * 올바르지 못한 uri 형식이면 -1을 return 합니다.
 * uri example == "http://localhost:8000/home.html"
 */
int parse_uri(char *uri, char *host, char *port, char *new_uri) {
  char *ptr;

  /* 필요없는 http 부분 잘라서 host 추출 */
  if (!(ptr = strstr(uri, "://")))
    return -1;  /* "://" 가 없으면 unvalid uri */
  ptr += 3;
  strcpy(host, ptr);

  /* new_uri 추출 */
  if ((ptr = strchr(host, '/'))) {
    *ptr = '\0';
    ptr += 1;
    strcpy(new_uri, "/");
    strcat(new_uri, ptr);
  }
  else strcpy(new_uri, "/");

  /* port 추출 */
  if ((ptr = strchr(host, ':'))) {
    *ptr = '\0';
    ptr += 1;
    strcpy(port, ptr);
  }
  else strcpy(port, "80");  /* port가 없을 경우, "80"을 넣어준다 */
  printf("파스uri내에서 찍음 uri: %s,host: %s,port: %s,new_uri:%s\n", uri, host, port, new_uri);
  return 0;
}

/*
 * 버퍼에 있는 정돈된 request를 clientfd를 통해 서버로 보냅니다.
 * 그리고 돌아온 응답을 get_버퍼에 쓰고,
 * 다시 connfd를 통해 클라이언트에게 전달해줍니다.
 */
int send_and_receive(int clientfd, int connfd, char *buf) {
  char get_buf[MAX_CACHE_SIZE];
  char content_len[MAXLINE];
  int content_len_int = 0;
  char *ptr, *contents;
  ssize_t n;
  rio_t rio;

  Rio_writen(clientfd, buf, MAXLINE);  /* buf -> clientfd -> SERVER */

  /* --- malloc을 활용해서 필요한 컨텐츠 길이만큼만 메모리 사용하기 --- */
  Rio_readinitb(&rio, clientfd);
  n = Rio_readlineb(&rio, get_buf, MAXLINE);
  Rio_writen(connfd, get_buf, n);

  while (strcmp(get_buf, "\r\n")) {  /* 헤더 먼저 받기. */
    n = Rio_readlineb(&rio, get_buf, MAXLINE);
    if ((ptr = strstr(get_buf, "length:"))) {
      ptr += 8;
      strcpy(content_len, ptr);
    }
    Rio_writen(connfd, get_buf, n);
  }
  
  content_len_int = atoi(content_len);
  contents = malloc((size_t)content_len_int);
  Rio_readnb(&rio, contents, (size_t)content_len_int);

  Rio_writen(connfd, contents, (size_t)content_len_int);  /* contents -> connfd -> CLIENT */
  free(contents);
  
  /* ----------------------- 요기까지 ----------------------- */

  /* 
   * 아래 코드는 고정크기배열(MAX_CACHE_SIZE)를 사용해서 받아오는 거랍니다.
   * 짧고 좋긴 한데, 이렇게 하면 MAX_CACHE_SIZE보다 큰 파일은 못 받아와요.
   */
  // Rio_readinitb(&rio, clientfd);
  // n = Rio_readnb(&rio, get_buf, MAX_CACHE_SIZE);
  // Rio_writen(connfd, get_buf, n);

  return 0;
}

/* 
 * 자식 프로세스 청소기입니다.
 */
void sigchld_handler(int sig) {
  while (waitpid(-1, NULL, WNOHANG) > 0);
  return;
}