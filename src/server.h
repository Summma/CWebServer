#ifndef SERVER_H_
#define SERVER_H_

#include <sys/socket.h>
#include <regex.h>
#include <poll.h>
#include <netinet/in.h>
#include <stdio.h>

typedef enum FileType {
    TEXT,
    BINARY,
    ERROR,
} FileType;

void die(const char *msg);

ssize_t send_response(int client_fd, FILE *file, const char *content_type, FileType file_type);

ssize_t serve_file(int client_fd, const char *file_path, const char *content_type, FileType file_type);

char* match_one(regex_t *regex, const char *str);

int match_content_type(const char *extension, char content_type[]);

void handle_request(int client_fd);

uint32_t string_to_ip(const char *ip_str);

void initialize_server(struct pollfd *fds, int max_size, struct sockaddr_in *addr, __uint32_t ip, __uint16_t port);

void serve(const int max_clients, char* ip, __uint16_t port);

#endif
