/* $begin tinymain */
/*
 * tiny.c - A simple, iterative HTTP/1.0 Web server that uses the
 *     GET method to serve static and dynamic content.
 *
 * Updated 11/2019 droh
 *   - Fixed sprintf() aliasing issue in serve_static(), and clienterror().
 */
#include "csapp.h"

void doit(int fd);
void read_requesthdrs(rio_t *rp);
int parse_uri(char *uri, char *filename, char *cgiargs);
void serve_static(int fd, char *filename, int filesize);
void get_filetype(char *filename, char *filetype);
void serve_dynamic(int fd, char *filename, char *cgiargs);
void clienterror(int fd, char *cause, char *errnum, char *shortmsg,
                 char *longmsg);

int main(int argc, char **argv) {
  int listenfd, connfd;
  char hostname[MAXLINE], port[MAXLINE];
  socklen_t clientlen;
  struct sockaddr_storage clientaddr;

  /* Check command line args */
  if (argc != 2) {
    fprintf(stderr, "usage: %s <port>\n", argv[0]);
    exit(1);
  }
  //리슨 소켓만들고 listenfd라는 식별자 반환
  listenfd = Open_listenfd(argv[1]);

  // 서버니까 청취모드 시작
  while (1) {
    clientlen = sizeof(clientaddr);
    //클라이언트가 connect요청 보냈고 서버는 accept
    //connfd식별자에는 연결이 맺어진 클라이언트와 서버의 정보가 들어있음 
    connfd = Accept(listenfd, (SA *)&clientaddr,&clientlen);  // line:netp:tiny:accept
    
    //client주소를 스트링으로 변환하고 hostname과 port에 복사
    Getnameinfo((SA *)&clientaddr, clientlen, hostname, MAXLINE, port, MAXLINE,0);
    printf("Accepted connection from (%s, %s)\n", hostname, port);
    doit(connfd);   // line:netp:tiny:doit
    Close(connfd);  // line:netp:tiny:close
  }
}
void doit(int fd)
// fd는 connfd(연결된 클라이언트와 서버 정보)
{
  int is_static;
  struct stat sbuf;
  char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];
  char filename[MAXLINE], cgiargs[MAXLINE];
  rio_t rio;
  
  /* Read request line and headers*/
  //connfd정보를 rio에 줌
  Rio_readinitb(&rio, fd);
  //rio에 정보를 buf로 넘김
  Rio_readlineb(&rio, buf, MAXLINE);
  printf("Request headers: \n");
  // 예) HTTP/1.0 200 OK 출력하고
  printf("%s", buf);
  //ssacnf는 buf에 담긴 정보(즉, connfd) 를 꺼내는 중 (메소드가 get인지 뭔지, uri가 뭔지, http version은 뭔지)
  sscanf(buf, "%s %s %s", method, uri, version);
  //strcasecmp는 비교하는거라 빼기라고 생각하면 됨. method가 "GET"이면 두번째 인자인 "GET"이랑 빼면 0이니까 False이고 if문에 안들어감.
  if(strcasecmp(method, "GET")){
    // GET말고 다른게 들어오면 tiny는 GET만 처리하는 상황이라 알려줌
    clienterror(fd, method, "501", "Not implemented", "Tiny does not implement this method");
    return;
  }
  //클라이언트가 요청한 정보를 읽고 출력하는 함수
  //요청한 정보를 헤더부분(P916의 그림 11.24의 5,6라인)이라고 특정할 수 있는 이유는 그냥 엔터 치기전(7번라인)까지가 해더이기 때문\r\n
  read_requesthdrs(&rio);

  //sscanf에서 connfd의 uri정보를 uri라는 변수에 담아놓았음
  //parse를 거치면서 (파일주소?인자)에서 파일주소만 남게됨(ex. /cgi-bin/addr?1234에서 /cgi-bin/addr만 남음) 
  is_static = parse_uri(uri, filename, cgiargs);

  //stat은 파일의 상태를 sbuf에 넘김. 0보다 작으면 없다는 의미
  if (stat(filename, &sbuf) < 0 ){
    clienterror(fd, filename, "404", "Not found", "Tiny couldn't find this file");
    return;
  }
  // 파일이 존재하면?
  if(is_static){ /*serve static content*/
  // 파일 권한체크 (읽을 수 있니? 보통파일이니?)
    if(!(S_ISREG(sbuf.st_mode)) || !(S_IRUSR & sbuf.st_mode)){
      clienterror(fd, filename, "403", "Forbidden", "Tiny couldn't read the file");
      return;
    }
    // 다 통과했으면 정적파일 serve함수로 이동
    serve_static(fd, filename, sbuf.st_size);
  }
  //정적 파일이 아니면? ->동적이면?
  else {
    if(!(S_ISREG(sbuf.st_mode)) || !(S_IXUSR & sbuf.st_mode)) {
      clienterror(fd, filename, "403", "Forbidden", "Tiny couldn't run the CGI program");
      return;
    }
    //동적 컨텐츠 함수 실행
    serve_dynamic(fd, filename, cgiargs);
  }
}
void clienterror(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg){
  char buf[MAXLINE], body[MAXBUF];

  sprintf(body, "<html><title>Tiny Error</title>");
  sprintf(body, "%s<body bgcolor=""ffffff"">\r\n", body);
  sprintf(body, "%s%s: %s\r\n", body, longmsg, cause);
  sprintf(body, "%s<p>%s: %s\r\n", body, longmsg, cause);
  sprintf(body, "%s<hr><em>The Tiny Web server</em>\r\n", body);

  sprintf(buf, "HTTP/1.0 %s %s\r\n", errnum, shortmsg);
  Rio_writen(fd, buf, strlen(buf));
  sprintf(buf, "Content-type: text/html\r\n");
  Rio_writen(fd, buf, strlen(buf));
  sprintf(buf, "Content-length: %d\r\n\r\n", (int)strlen(body));
  Rio_writen(fd, buf, strlen(buf));
  Rio_writen(fd, body, strlen(body));
}
void read_requesthdrs(rio_t *rp){
  //rp에는 connfd정보 담김
  char buf[MAXLINE];
  // connfd정보를 buf에 담음
  Rio_readlineb(rp, buf, MAXLINE);

  //connfd에 담긴 정보가 없다면(비어있다면) 개행문자 \r\n과 같을거고 strcmp는 두 인자가 같으면 0을 뱉음.
  //그럼 while문 내부인 connfd정보를 개행문자가 나올 때까지 찍는 짓을 안하고 그냥 return으로 넘어간다는 소리
  while(strcmp(buf, "\r\n")){
    Rio_readlineb(rp, buf, MAXLINE);
    printf("%s", buf);
  }
  return;
}
int parse_uri(char *uri, char *filename, char *cgiargs){
  char *ptr;
  //strstr은 스트링에서 두번째 인자와 같은 스트링이 있으면 그 문자열의 첫 시작 주소를 뱉음
  //나중에 다루겠지만 cgi-bin은 동적컨텐츠들이 담겨있는 곳 (P.915)
  if(!strstr(uri, "cgi-bin")){
    //strcpy는 카피하는 함수. 두 번째를 첫 번째로
    strcpy(cgiargs, "");
    strcpy(filename, ".");
    //strcat은 문자열을 연결함.(두번째를 첫번째와 연결)
    strcat(filename, uri);

    //만약 url이 /로 오면 home.html을 보여준다(/home.html매번 쓰기 귀찮..)
    if(uri[strlen(uri)-1] == '/')
      strcat(filename, "home.html");
    return 1;
  }
  // 정적컨텐츠를 리턴해야하는 상황이면?
  else{
    //uri에서 ?로 시작하는 곳을 찾아
    //?는 url에서 ?앞은 파일 이름이고 ?뒤는 인자이다. (P.918)
    ptr = index(uri, '?');
    if(ptr){
      strcpy(cgiargs, ptr+1);
      // ?를 널문자로 만들어버림
      // 왜? a = www.naver.com?120이었는데 ?를 \0으로 만들면 문자열은 널문자 만나면 중단되니까 a에서 ?뒤를 버려버리는 효과가 있음
      *ptr = '\0';
    }
    else
      strcpy(cgiargs, "");
    strcpy(filename, ".");
    strcat(filename, uri);
    return 0;
  }
}
void serve_static(int fd, char *filename, int filesize){
  int srcfd;
  char *srcp, filetype[MAXLINE], buf[MAXBUF];

  //get_filetype으로 어떤 타입인지 가져옴(html인지, jpg인지, gif인지..)
  get_filetype(filename, filetype);
  //P921 그림 11.28의 7번줄에 나온 것처럼 출력하기 위한 작업을 시작
  //sprinf는 buf에 두번째 인자를 저장
  sprintf(buf, "HTTP/1.0 200 OK\r\n");
  // %s에는 "HTTP/1.0 200 OK\r\n"이게 있고 바로 Server: Tiny Web server\r\n가 오고 그게 buf에 덮어써짐
  sprintf(buf, "%sServer: Tiny Web server\r\n", buf);
  sprintf(buf,"%sConnection: close\r\n",buf);
  sprintf(buf,"%sContent-length: %d\r\n",buf,filesize);
  // r\n\r\n 두번함으로써 서버가 내가 보내주는건 여기까지야~ (빈줄 하나 추가한 것)
  sprintf(buf,"%sContnent-type: %s\r\n\r\n", buf, filetype);

  // buf적힌거를 식별자fd(파일임)로 넘김
  Rio_writen(fd, buf, strlen(buf));
  printf("Response headers:\n");

  //printf할때 찍히는 함수(buf에 담겨있던 내용)
  // Response headers:
  // HTTP/1.0 200 OK
  // Server: Tiny Web server
  // Connection: close
  // Content-length: 120
  // Contnent-type: text/html
  printf("%s", buf);
  
  //파일을 열고 그 파일에 대응되는 식별자를 하나 주고
  srcfd=Open(filename, O_RDONLY,0);

  // 숙제문제 11.9(mmap으로 메모리 매핑시)
    // //Mmap으로 파일의 공간을 배정한다.
    // //Mmap은 파일 내용을 메모리에 매핑 
    // srcp = Mmap(0,filesize, PROT_READ, MAP_PRIVATE, srcfd, 0);

    // // Mmap으로 메모리에 담았으니 srcfd식별자 반환해줘(안그러면 메모리 누수 발생)
    // Close(srcfd);
    // // 사용자가 요청한 정적 파일을 fd식별자에 넣고 
    // Rio_writen(fd, srcp, filesize);
    // // Mmap으로 만든 메모리도 반환
    // Munmap(srcp,filesize);

  // 숙제문제 11.9(malloc으로 메모리 매핑시)
    srcp = (char*)Malloc(filesize);
    Rio_readn(srcfd, srcp, filesize);
    Close(srcfd);
    // 사용자가 요청한 정적 파일을 fd식별자에 넣고 
    Rio_writen(fd, srcp, filesize);
    free(srcp);
}

void get_filetype(char *filename, char *filetype){
  if(strstr(filename, ".html"))
    strcpy(filetype, "text/html");

  else if (strstr(filename, ".gif"))
    strcpy(filetype, "image/gif");

  else if (strstr(filename, ".png"))
    strcpy(filetype, "image/png");

  else if (strstr(filename, ".jpg"))
    strcpy(filetype, "image/jpeg");

  else if (strstr(filename, ".mp4"))
    strcpy(filetype, "video/mp4");

  else
    strcpy(filetype, "text/plain");
}

void serve_dynamic(int fd, char *filename, char *cgiargs){
  char buf[MAXLINE], *emptylist[]={NULL};
  
  sprintf(buf, "HTTP/1.0 200 OK\r\n");
  Rio_writen(fd, buf, strlen(buf));
  sprintf(buf, "Server: Tiny Web Server\r\n");
  Rio_writen(fd, buf, strlen(buf));
  if (Fork()== 0){
    // 자식프로세스를 하나 만들고

    // setenv는 환경변수 값을 추가하거나 바꾸는 함수
    // 환경변수 내에 QUERY_STRING가 존재하면 cgiargs로 덮어쓰고, 없으면 추가한다.
    setenv("QUERY_STRING", cgiargs,1);
    //Dup2 식별자를 복사한다.
    Dup2(fd, STDOUT_FILENO);
    // Execve 파일을 실행한다.
    Execve(filename, emptylist, environ);
  }
  Wait(NULL);
}
