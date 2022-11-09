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

        // /*sequential handle the client transaction*/
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
    int end_serverfd;/*the end server file descriptor*/
    int is_static;
    struct stat sbuf;
    char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];
    char endserver_http_header [MAXLINE];
    char hostname[MAXLINE], path[MAXLINE];
    int port;

    rio_t rio, server_rio; /*rio is client's rio,server_rio is endserver's rio*/
    // int parse_result;
    // char new_version[]="HTTP/1.0";

    /* Read request line and headers*/
    //connfd(프록시가 클라이언트랑 맺은것)정보를 rio에 줌
    Rio_readinitb(&rio, connfd);
    //rio에 정보를 buf로 넘김
    Rio_readlineb(&rio, buf, MAXLINE);
    printf("Request headers: \n");
    
    //ssacnf는 buf에 담긴 정보(즉, connfd)를 꺼내는 놈 (메소드가 get인지 뭔지, uri가 뭔지, http version은 뭔지)
    sscanf(buf, "%s %s %s", method, uri, version);
    //GET,   naver.com,  http/1.0 클라이언트가 텔넷으로 요청한 정보가 파싱 전 상태로 나옴
    //printf("%s,%s,%s\n", method,uri,version);

    if(strcasecmp(method,"GET")){
        printf("Proxy does not implement the method");
        return;
    }
    parse_uri(uri, hostname, path, &port);
    // printf("파스uri 안에서 찍음 uri: %s, hostname: %s, path: %s, port: %d\n", uri, hostname, path, *port);
    // sprintf(new_buf, "%s %s %s\r\n", method, path, new_version);

    //hostname, path, port는 클라이언트가 넣은 url파싱한 후의 정보이고 rio는 connfd의 정보
    build_http_header(endserver_http_header,hostname,path,port,&rio);

    end_serverfd = connect_endServer(hostname, port, endserver_http_header);
    // //클라이언트가 보낸 요청에서 header부분만 읽어 오는 부분 body는 버리곳
    if (end_serverfd<0){
      printf("connection failed\n");
      return;
    }
    Rio_readinitb(&server_rio,end_serverfd);
    Rio_writen(end_serverfd,endserver_http_header,strlen(endserver_http_header));

    /*receive message from end server and send to the client*/
    size_t n;
    while((n=Rio_readlineb(&server_rio,buf,MAXLINE))!=0)
    {
        printf("proxy received %d bytes,then send\n",n);
        Rio_writen(connfd,buf,n);
    }
    Close(end_serverfd);

  }
}

void build_http_header(char *http_header,char *hostname,char *path,int port,rio_t *client_rio)
{
    char buf[MAXLINE],request_hdr[MAXLINE],other_hdr[MAXLINE],host_hdr[MAXLINE];
    /*request line*/
    sprintf(request_hdr,requestlint_hdr_format,path);
    /*get other request header for client rio and change it */
    while(Rio_readlineb(client_rio,buf,MAXLINE)>0)
    {
        if(strcmp(buf,endof_hdr)==0) break;/*EOF*/

        if(!strncasecmp(buf,host_key,strlen(host_key)))/*Host:*/
        {
            strcpy(host_hdr,buf);
            continue;
        }

        if(!strncasecmp(buf,connection_key,strlen(connection_key))
                &&!strncasecmp(buf,proxy_connection_key,strlen(proxy_connection_key))
                &&!strncasecmp(buf,user_agent_key,strlen(user_agent_key)))
        {
            strcat(other_hdr,buf);
        }
    }
    if(strlen(host_hdr)==0)
    {
        sprintf(host_hdr,host_hdr_format,hostname);
    }
    sprintf(http_header,"%s%s%s%s%s%s%s",
            request_hdr,
            host_hdr,
            conn_hdr,
            prox_hdr,
            user_agent_hdr,
            other_hdr,
            endof_hdr);

    return ;
}

/*parse the uri to get hostname,file path ,port*/
void parse_uri(char *uri,char *hostname,char *path,int *port)
{
    *port = 80;
    char* pos = strstr(uri,"//");

    pos = pos!=NULL? pos+2:uri;

    char*pos2 = strstr(pos,":");
    if(pos2!=NULL)
    {
        *pos2 = '\0';
        sscanf(pos,"%s",hostname);
        sscanf(pos2+1,"%d%s",port,path);
    }
    else
    {
        pos2 = strstr(pos,"/");
        if(pos2!=NULL)
        {
            *pos2 = '\0';
            sscanf(pos,"%s",hostname);
            *pos2 = '/';
            sscanf(pos2,"%s",path);
        }
        else
        {
            sscanf(pos,"%s",hostname);
        }
    }
    return;
}

/* 파싱하는데 url이 http://localhost/home.html:8001 이라고 생각하고 짜버림..
http://localhost:8001/home.html 인데.. 시간없어서 파싱 부분 다른 사람 코드로 대체
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
*/

inline int connect_endServer(char *hostname,int port,char *http_header){
  
  // port를 int형으로 선언했는데 http는 항상 string기준으로 보낸단말이야?
  // 그래서 int를 str형으로 바꿔주는 로직이 필요함. (애당초 처음부터 port를 char*타입으로 했으면 됐음.)
  char portStr[100];
  sprintf(portStr,"%d",port);
  return Open_clientfd(hostname, portStr);
}