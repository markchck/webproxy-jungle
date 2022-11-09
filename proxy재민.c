#include <stdio.h>
#include "csapp.h"
/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

/* You won't lose style points for including this long line in your code */
static const char *user_agent_hdr = "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 Firefox/10.0.3\r\n";
static const char *conn_hdr = "Connection: close\r\n";
static const char *prox_hdr = "Proxy-Connection: close\r\n";
static const char *host_hdr_format = "Host: %s\r\n";
static const char *requestlint_hdr_format = "GET %s HTTP/1.0\r\n";
static const char *endof_hdr = "\r\n";

static const char *connection_key = "Connection";
static const char *user_agent_key= "User-Agent";
static const char *proxy_connection_key = "Proxy-Connection";
static const char *host_key = "Host";

/*프로토타입*/
void doit(int connfd);
int parse_uri(char *uri,char *hostname,char *path,char *port);
void build_http_header(char *http_header,char *hostname,char *path,int port,rio_t *client_rio);
int connect_endServer(char *hostname,char *port,char *http_header);

//프록시 듣기모드 온
int main(int argc,char **argv)
{
    int listenfd,connfd;
    socklen_t  clientlen;
    char hostname[MAXLINE],port[MAXLINE];

    struct sockaddr_storage clientaddr;/*generic sockaddr struct which is 28 Bytes.The same use as sockaddr*/

    if(argc != 2){
        fprintf(stderr,"usage :%s <port> \n",argv[0]);
        exit(1);
    }
    listenfd = Open_listenfd(argv[1]);
    while(1){
        clientlen = sizeof(clientaddr);
        connfd = Accept(listenfd,(SA *)&clientaddr,&clientlen);

        /*print accepted message*/
        Getnameinfo((SA*)&clientaddr,clientlen,hostname,MAXLINE,port,MAXLINE,0);
        printf("Accepted connection from (%s %s).\n",hostname,port);
        /*sequential handle the client transaction*/
        doit(connfd);
        Close(connfd);
    }
    return 0;
}

void doit(int connfd)
// fd는 connfd(연결된 클라이언트와 서버 정보)
{
  //프록시랑 찐 클라이언트랑 connfd로 연결
  {
    int is_static;
    struct stat sbuf;
    char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE], new_buf[MAXLINE];
    char hostname[MAXLINE], path[MAXLINE], port[MAXLINE];
    rio_t rio;
    int parse_result;

    /* Read request line and headers*/
    //connfd정보를 rio에 줌
    Rio_readinitb(&rio, connfd);
    //rio에 정보를 buf로 넘김
    Rio_readlineb(&rio, buf, MAXLINE);
    printf("Request headers: \n");
    
    //ssacnf는 buf에 담긴 정보(즉, connfd)를 꺼내는 놈 (메소드가 get인지 뭔지, uri가 뭔지, http version은 뭔지)
    sscanf(buf, "%s %s %s", method, uri, version);
    //GET,   naver.com,  http/1.0 클라이언트가 텔넷으로 요청한 정보가 파싱 전 상태로 나옴
    //printf("%s,%s,%s\n", method,uri,version);

    // parse_uri(uri, hostname, path, port);
    // Rio_readinitb(&rio,parse_result);
    // Rio_readlineb(&rio,new_buf, MAXLINE);
    // sprintf(new_buf, "%s %s %s\r\n", hostname, path, port);
    // printf("%s\n", new_buf);
    // printf("%s,%s,%s,%s\n", uri, hostname, path, port);

    parse_uri(uri, hostname, path, port);
    printf("doit 안에서 찍음 uri: %s, hostname: %s, path: %s, port: %s\n", uri, hostname, path, port);
  }
}

int parse_uri(char *uri, char *hostname, char *path, char *port){
  // /hub/index.html,
  // printf("%s,\n", uri);
  char* full_url;
  char* path_start;
  char* port_start;
  
  //full_url: http://naver.com/index.html에서 n이후를 가리킴
  full_url = strstr(uri,"//")+2;
  //path_start: /index.html에서 /를 가리킴
  path_start=strstr(full_url, "/");
  // path_start를 \0으로 만듦으로써 full_url을 naver.com까지만 자름
  *path_start = '\0';

  //path는 index.html~가 됨
  // 이렇게 문자열을 복사하면 안됨 (oit으로 넘어갈 때는 doit의 hostname이 parse_uri가 넘긴 path을 덮어버림 )
  // path = (path_start+1);
  strcpy(path ,path_start+1);
  
  // full_rul을 hostname으로 얕은 복사함(따로 메모리를 저장하지 않고 주소값만 복사)
  // 이렇게 문자열을 복사하면 안됨 (doit으로 넘어갈 때는 doit의 hostname이 parse_uri가 넘긴 hostname을 덮어버림 )
  // hostname = full_url;
  strcpy(hostname, full_url);

  // 이상유무 테스트
    // printf("hostname을 출력: %s\n", hostname); www.cmu.edu
    // printf("path를 출력: %s\n", path); hub/index.html
  
  //hostname과 path를 구분하고, 포트를 입력하는 경우 그 포트를 포트번호로하고, 입력하지 않은 경우 80으로 설정해줘야함.
  port_start=strstr(path, ":");
  if (port_start != NULL){
    *port_start='\0'; //hub/index.html:8001에서 :8001 떼고 hub/index.html 이것만 path로.

    //port는 8001이 들어가있음
    // 이렇게 문자열을 복사하면 안됨 (doit으로 넘어갈 때는 doit의 port가 parse_uri가 넘긴 port를 덮어버림 )  
    // port = port_start +1; 
    strcpy(port ,port_start+1);
  }
  else{
    strcpy(port, "80");
  }
  // else 입력하지 않은 경우는 안해줘도 되는듯? (port는 이미 80으로 선언했고 path도 이미 hub/index.html임)

  printf("파스uri 안에서 찍음 uri: %s, hostname: %s, path: %s, port: %s\n", uri, hostname, path, port);
  return 0;
}
