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
void parse_uri(char *uri,char *hostname,char *path,int *port);
void build_http_header(char *http_header,char *hostname,char *path,int port,rio_t *client_rio);
int connect_endServer(char *hostname,int port,char *http_header);
void clienterror(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg);

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
    char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];
    char hostname[MAXLINE], path[MAXLINE];
    int port;
    rio_t rio;

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

    if(! ((strcasecmp(method, "GET") == 0) || (strcasecmp(method, "HEAD")==0) ) ){
      // GET말고 다른게 들어오면 tiny는 GET만 처리하는 상황이라 알려줌
      clienterror(connfd, method, "501", "Not implemented", "Tiny does not implement this method");
      return;
    }
    parse_uri(uri, hostname, path, &port);
  }
}

void parse_uri(char *uri, char *hostname, char *path, int *port){
  // /hub/index.html,
  // printf("%s,\n", uri);
  *port = 80;
  char* full_url;
  char* path_start;
  full_url = strstr(uri,"//")+2;
  path_start=strstr(full_url, "/");
  *path_start = '\0';

  // 비로소 full_url이 hostname만 남고
  // 비로소 path가 hub/index.html이 됨
  path = (path_start+1);
  hostname = full_url;



  printf("hostname을 출력: %s\n", hostname);
  printf("path를 출력: %s\n", path);
  
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