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
    Close(listenfd);
    Getnameinfo((SA *)&clientaddr, clientlen, hostname, MAXLINE, port, MAXLINE, 0);
    printf("Accepted connection from (%s, %s)\n", hostname, port);

    /* valid check here */

    //클라이언트의 요청이 유효하면 if문을 타라
    if (!(doit(connfd, host_for_server, port_for_server, buf_for_server))) 
    {
      // 프록시가 클라이언트가 되는 준비 (프록시 입장에서 서버인 놈의 호스트와 포트넘버가 인자임)
      printf("프록시 입장에서 서버의 호스트: %s, 포트: %s\n ", host_for_server, port_for_server);
      //출력 결과 -> "프록시 입장에서 서버의 호스트: localhost, 포트: 8001" 

      //텔넷에서 telnet localhost 8000으로 서버에 접속하고(사실 프록시지만..)
      //클라이언트가 GET http://localhost:8001/home.html로 요청을 보낸걸 doit에서 유효성 검사하고
      //서버의 호스트 localhost, 포트넘버 8001를 host_for_server, port_for_server인자로 줌.
      clientfd = Open_clientfd(host_for_server, port_for_server);
      
      //clientfd는 프록시와 찐서버 관계가에서 프록시가 클라이언트이니 클라이언트 식별자
      //connfd는 찐클라이언트와 프록시의 연결
      //buf_for_server는 프록시가 찐서버의 클라이언트로서 request할 텍스트가 담김 "GET /home.html HTTP/1.0" 이런 내용
      send_and_receive(clientfd, connfd, buf_for_server);
      Close(clientfd);
    }
    else printf("URI Error: 올바르지 않은 URI입니다.\n");

    Close(connfd);
    exit(0);

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
  
  // &rio에는 connfd의 정보가 들어있다.
  Rio_readinitb(&rio, fd);
  Rio_readlineb(&rio, buf, MAXLINE);
  // connfd에는 클라이언트가 요청한 정보가 들어있음. 그걸 sscanf로 꺼내오는 중
  // GET + http://localhost:8001/home.html + HTTP/1.1
  sscanf(buf, "%s %s %s", method, uri, version);

  //클라이언트가 요청한 uri를 호스트 정보, 파일 경로, 포트 정보 추출
  if((parse_uri(uri, host, port, new_uri)) == -1)
    return -1;
  //new_buf에 새로 파싱한 정보들을 넣음.
  sprintf(new_buf, "%s %s %s\r\n", method, new_uri, new_version);

    // printf("구버프: %s", buf); //GET http://localhost:8001/home.html
  while (strcmp(buf, "\r\n")) {
    // rio는 connfd의 정보이고 이걸 buf에 넣는다.
    Rio_readlineb(&rio, buf, MAXLINE);
    //newbuf에 새로 파싱한 정보들과 파싱전의 정보들을 넣는다?
    // printf("구버프: %s", buf); //(출력없음)
    // printf("뉴버프: %s", new_buf); //GET /home.html HTTP/1.0
    sprintf(new_buf, "%s%s", new_buf, buf);
    // printf("합쳐진 후 뉴버프: %s", new_buf); //GET /home.html HTTP/1.0
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

  //프록시가 클라이언트로서 찐서버에 보낼 request문을 프록시의clientfd에 알려주면 찐서버는 찐서버의 connfd를 통해 바로 정보를 받는다.
  //즉 프록시 입장에서는 찐서버가 어떻게 받을지 신경 안써도 된다. clientfd에만 넣어주면 됨
  //반대로 찐서버가 보내주는 정보는 바로 clientfd에 들어오기 때문에 따로 받는 과정 신경 쓸 필요 없이 clientfd에 찐 서버가 보내준 내용이 담겨있다.
  Rio_writen(clientfd, buf, MAXLINE);  /* buf -> clientfd -> SERVER */

  /* --- malloc을 활용해서 필요한 컨텐츠 길이만큼만 메모리 사용하기 --- */
  //rio에는 찐서버가 프록시클라이언트에게 보내준 정보가 들어있다.
  Rio_readinitb(&rio, clientfd);

  //찐서버가 보내준 정보rio를 get_buf에 넣는다.
  // 페이지 861 참고, 세번째인자 크기만큼 첫번째인자위치에서 두번째인자로 복사함. 리턴은 실제 전송한 바이트의 수
  n = Rio_readlineb(&rio, get_buf, MAXLINE);

  //writen함수는 두번째 인자에서 첫번째 인자 위치로 최대 세번째 인자 크기만큼 복사한다.
  //리턴은 전송한 바이트의 수
//!!!!!!!!!!!!!찐서버에게 받은 정보(get_buf)를 클라이언트(connfd)에게 넘겨주는 역사적인 과정!!!!!!!!!!!!!!
  //connfd에 넘겨주면 찐클라이언트의 clientfd는 자동으로 알게된다.
  Rio_writen(connfd, get_buf, n);

  //getbuf가 그냥 빈칸이면 while문 패스하고 뭔가 차있으면 while들어옴.
  //get_buf에는 찐서버에게 받은 정보가 들어있다.

  while (strcmp(get_buf, "\r\n")) {  /* 헤더 먼저 받기. */
    printf("getbuf에 들어있던 정보 %s\n", get_buf); //HTTP/1.0 200 OK
    n = Rio_readlineb(&rio, get_buf, MAXLINE);
    printf("Rio한 뒤 getbuf에 있는 정보 %s\n", get_buf); //출력 없음
    
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