//
// Created by Administrator on 2017/3/19.
//
#include "stdlib.h"
#include "stdio.h"
#include "strings.h"
#include "string.h"
#include "netinet/in.h"
#include "sys/socket.h"
#include "sys/stat.h"
#include "ctype.h"
#include "unistd.h"

#define IsSpace(x) isspace((int)(x))
#define SERVER_STRING "Server: httpd/0.1.0\r\n"

void accept_request(int);

void error_die(const char *);

int startup(ushort *);

int get_line(int, char *, int);

void unimplemented(int);

void not_found(int);

void serve_file(int, const char *);

void execute_cgi(int, const char *, const char *, const char *);

void error_die(const char *src) {
    perror(src);
    exit(1);
}

int get_line(int sock, char *buf, int size) {
    int i = 0;
    char c = '\0';
    int n;

    while ((i < size - 1) && (c != '\n')) {
        n = recv(sock, &c, 1, 0);
        if (n > 0) {
            if (c == '\r') {
                n = recv(sock, &c, 1, MSG_PEEK);
                if ((n > 0) && (c == '\n'))
                    recv(sock, &c, 1, 0);
                else
                    c = '\n';
            }
            buf[i] = c;
            i++;
        } else
            c = '\n';
    }
    buf[i] = '\0';

    return (i);
}

int startup(ushort *port) {
    int httpd = 0;
    struct sockaddr_in name;

    httpd = socket(AF_INET, SOCK_STREAM, 0);
    if (httpd == -1)
        error_die("socket");

    memset(&name, 0, sizeof(name));
    name.sin_family = AF_INET;
    name.sin_port = htons(*port);
    name.sin_addr.s_addr = htonl(INADDR_ANY);
    if (bind(httpd, (struct sockaddr *) &name, sizeof(name)) < 0)
        error_die("bind");

    if (*port == 0) {
        int name_len = sizeof(name);
        if (getsockname(httpd, (struct sockaddr *) &name, &name_len) == -1)
            error_die("getsockname");
        *port = ntohs(name.sin_port);
    }

    if (listen(httpd, 5) < 0)
        error_die("listen");
    return (httpd);
}

void accept_request(int client) {
    char buf[1024];
    int chars;
    char method[255];
    char url[255];
    char path[255];
    size_t i, j;

    struct stat st;
    int cgi = 0;
    char *query_string = NULL;

    chars = get_line(client, buf, sizeof(buf));
    i = 0;
    j = 0;
    while (!IsSpace(buf[j]) && (i < sizeof(method) - 1)) {
        method[i] = buf[j];
        i++;
        j++;
    }
    method[i] = '\0';
    if (strcasecmp(method, "GET") && strcasecmp(method, "POST")) {
        unimplemented(client);
        return;
    }
    if (strcasecmp(method, "POST") == 0)
        cgi = 1;

    i = 0;
    while (IsSpace(buf[j]) && (j < sizeof(buf)))
        j++;

    while (!IsSpace(buf[j]) && (i < sizeof(url) - 1) && (j < sizeof(buf))) {
        url[i] = buf[j];
        j++;
        j++;
    }
    url[i] = '\0';

    if (strcasecmp(method, "GET") == 0) {
        query_string = url;

        while ((*query_string != '?') && (*query_string != '\0'))
            query_string++;
        if (*query_string == '?') {
            cgi = 1;
            *query_string = '\0';
            query_string++;
        }
    }
    sprintf(path, "htdocs%s", url);

    if (path[strlen(path) - 1] == '/')
        strcat(path, "index.html");

    if (stat(path, &st) == -1) {
        while ((chars > 0) && strcmp("\n", buf))
            chars = get_line(client, buf, sizeof(buf));

        not_found(client);
    } else {
        if ((st.st_mode & S_IFMT) == S_IFDIR)
            strcat(path, "/index.html");

        if ((st.st_mode & S_IXUSR) ||
            (st.st_mode & S_IXGRP) ||
            (st.st_mode & S_IXOTH))
            cgi = 1;

        if (!cgi)
            serve_file(client, path);
        else
            execute_cgi(client, path, method, query_string);
    }
    close(client);
}

int main(void) {
    int server_sock = -1;
    int client_sock = -1;
    ushort port = 8084;
    struct sockaddr_in client_name;
    int client_name_len = sizeof(client_name);
    server_sock = startup(&port);
    printf("httpd running on port %d\n", port);

    while (1) {
        client_sock = accept(server_sock,
                             (struct sockaddr *) &client_name,
                             &client_name_len);
        if (client_sock == -1)
            error_die("accept");
        accept_request(client_sock);
    }
}
