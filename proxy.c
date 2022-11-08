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

int main(int argc, char **argv) {
    int listenfd, connfd;
    char hostname[MAXLINE], port[MAXLINE];
    socklen_t clientlen;
    struct sockaddr_storage clientaddr;
    
    /*check command-line args*/
    if(argc != 2){
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
        exit(1);
    }
    listenfd = Open_listenfd(argv[1]);
    while(1){
        clientlen = sizeof(clientaddr);
        connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen);
        Getnameinfo((SA *) &clientaddr, clientlen, hostname, MAXLINE, port, MAXLINE, 0);
        printf("Accepted connection from(%s, %s)\n", hostname, port);
        doit(connfd);
        Close(connfd);
    }
    printf("%s", user_agent_hdr);
    return 0;
}

void doit(int connfd){
    char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];
    /*store the request line arguments*/
    char hostname[MAXLINE], path[MAXLINE];
    int port;

    rio_t rio;
    /*Read request line and headers*/
    Rio_readinitb(&rio,connfd);
    Rio_readlineb(&rio, buf, MAXLINE);
    printf("Request headers:\n");
    printf("%s", buf);
    sscanf(buf, "%s %s %s", method, uri, version);
    
    if (strcasecmp(method, "GET")){
        printf("Proxy does not implement the method");
        return;
    }

    /*parse the uri to get hostanme, file path, port*/
    parse_uri(uri, hostname, path, &port);

    /*build the http header which will send to the end server*/
    //여기서 end server는 프록시가 대리하고 있는 서버
    // build_http_header();

}

// parse_uri(char *uri, char *hostname, char *path, int *port){
    
// }

