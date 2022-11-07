#include <stdio.h>
#include "csapp.c"

/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

/* You won't lose style points for including this long line in your code */
static const char *user_agent_hdr =
    "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 "
    "Firefox/10.0.3\r\n";

/* PROTOTYPES */
void clienterror(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg);
void read_requesthdrs(rio_t *rp);
int parse();
int throw_to_server(int fd);
void throw_to_client(int fd);
void doit(int fd);

/*          For implement:
 * listen incoming connections on a port -> number는 command line에 특정될 것.
 * Connection이 만들어지면, client의 request를 읽고 parse해야함.
 * Client가 유효한 HTTP request를 보냈는가? 확인하고
 * 적절한 웹 서버로 connection을 만들 수 있다.
 * 그리고 client가 specified했던 object를 요청하자...
 */
int main(int argc, char **argv) {
  int listenfd, connfd;
  char hostname[MAXLINE], port[MAXLINE];
  socklen_t clientlen;
  struct sockaddr_storage clientaddr;

  if (argc != 2) {
    fprintf(stderr, "usage: %s <port>\n", argv[0]);
    exit(1);
  }

  listenfd = Open_listenfd(argv[1]);
  while (1) {
    clientlen = sizeof(clientaddr);
    connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen);
    Getnameinfo((SA *)&clientaddr, clientlen, hostname, MAXLINE, port, MAXLINE, 0);
    printf("Accepted connection from (%s, %s)\n", hostname, port);
    if(throw_to_server(connfd))
      throw_to_client(connfd);
    Close(connfd);
  }

  printf("%s", user_agent_hdr);

  return 0;
}

int throw_to_server(int fd) {
  
}
void do_client(int fd){
  //서버한테 요청해요
  //서버에게 응답 받아요
  //내가 서버였을 때, 
  //찐 클라이언트에게 write해
}

void doit(int fd){
  //parsing해요
  //validcheck해요
  //vaild하면 요청해요 #client되어줘요
  //do_client
}
