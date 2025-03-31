#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <regex.h>
#include <poll.h>
#include "server.h"

void die(const char *msg) {
    perror(msg);
    exit(EXIT_FAILURE);
}

ssize_t send_response(int client_fd, FILE *file, const char *content_type, FileType file_type) {
    fseek(file, 0, SEEK_END);
    size_t file_size = ftell(file);
    rewind(file);

    char header[400];  // Buffer for HTTP response
    snprintf(header, sizeof(header),
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: %s\r\n"
        "Content-Length: %lu\r\n"
        "\r\n",
        content_type, file_size);

    ssize_t rv = send(client_fd, header, strlen(header), 0);

    char buffer[4096];
    size_t bytes_read;
    while ((bytes_read = fread(buffer, 1, sizeof(buffer) - 1, file)) > 0) {
        rv = send(client_fd, buffer, bytes_read, 0);

        if (rv < 0) {
            die("send");
        }
    }

    if (file_type == TEXT)
        buffer[bytes_read] = '\0';

    fclose(file);

    return rv;
}

ssize_t serve_file(int client_fd, const char *file_path, const char *content_type, FileType file_type) {
    FILE *file = fopen(file_path, "rb");

    if (!file) {
        if (strcmp(file_path, "404.html") == 0) {
            die("Please make the 404.html file");
        }

        printf("MAYDAY!\n");
        printf("%s\n", file_path);
        printf("%s\n", content_type);
        return serve_file(client_fd, "404.html", "text/html", TEXT);
    }

    return send_response(client_fd, file, content_type, TEXT);
}

char* match_one(regex_t *regex, const char *str) {
    size_t one_match = 2;
    regmatch_t match[one_match];
    if (regexec(regex, str, one_match, match, 0) == 0) {
        return strndup(str + match[1].rm_so, match[1].rm_eo - match[1].rm_so);
    }
    return NULL;
}

int match_content_type(const char *extension, char content_type[]) {
    if (strcmp(extension, "html") == 0) {
        strcpy(content_type, "text/html");
    }
    else if (strcmp(extension, "css") == 0) {
        strcpy(content_type, "text/css");
    }
    else if (strcmp(extension, "jpeg") == 0) {
        strcpy(content_type, "image/jpeg");
    }
    else if (strcmp(extension, "js") == 0) {
        strcpy(content_type, "application/javascript");
    }
    else if (strcmp(extension, "pdf") == 0) {
        strcpy(content_type, "application/pdf");
        return 2;
    }
    else {
        printf("Unsupported content type: %s\n", extension);
        return 0;
    }
    return 1;
}

void handle_request(int client_fd) {
    char buffer[1024];
    ssize_t bytes_read = recv(client_fd, buffer, sizeof(buffer), 0);

    if (bytes_read < 0) {
        die("recv()");
    }

    // printf("Received message: %.*s\n", (int)bytes_read, buffer);

    regex_t regex;

    // Regex pattern for extracting file path
    int status = regcomp(&regex, "GET /(.+\\..+) HTTP/1\\.1", REG_EXTENDED);

    if (status != 0) {
        die("regcomp()");
    }

    // Getting file path
    char* matched = match_one(&regex, buffer);

    if (matched) {
        // At this point, we know the file path must at least follow this pattern
        // So checking for errors again would be redundant
        int _ = regcomp(&regex, "\\.(.*)", REG_EXTENDED);

        char* extension = match_one(&regex, matched);
        char content_type[100];

        int valid_extension = match_content_type(extension, content_type);

        printf("Server asked for: %s\n", matched);
        if (valid_extension) {
            FileType file_type;
            if (status == 1) file_type = TEXT;
            else file_type = BINARY;

            bytes_read = serve_file(client_fd, matched, content_type, file_type);
        }
        else {
            printf("Unsupported content type: %s\n", extension);
        }
    }
    else {
        bytes_read = serve_file(client_fd, "index.html", "text/html", TEXT);
    }
    if (bytes_read < 0) {
        die("read");
    }
}

void initialize_server(struct pollfd *fds, int max_size, struct sockaddr_in *addr, __uint32_t ip, __uint16_t port) {
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);

    int val = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val));

    addr->sin_family = AF_INET;
    addr->sin_port = htons(port);
    addr->sin_addr.s_addr = htonl(ip);

    int rv = bind(server_fd, (const struct sockaddr *)addr, sizeof(*addr));

    if (rv) {
        die("bind()");
    }

    rv = listen(server_fd, 4096);

    if (rv) {
        die("listen()");
    }

    fds[0].fd = server_fd;
    fds[0].events = POLLIN;

    for (int i = 1; i<=max_size; i++) {
        fds[i].fd = -1;
    }
}

void serve(const int max_clients, __uint32_t ip, __uint16_t port) {
    const int max_size = 20; int client_fd;
    struct sockaddr_in addr;
    int addr_len = sizeof(addr);
    struct pollfd fds[max_size + 1];

    initialize_server(fds, max_size, &addr, 0, 1234);

    int fd = fds[0].fd;

    while (1) {
        int activity = poll(fds, max_size + 1, -1);

        if (activity < 0) {
            perror("poll()");
            continue;
        }

        if (fds[0].revents & POLLIN) {
            client_fd = accept(fd, (struct sockaddr *)&addr, (socklen_t *)&addr_len);

            if (client_fd < 0) {
                perror("accept()");
                continue;
            }

            // Looping through all clients and assigning fd to the first available slot
            for (int i = 1; i <= max_size; i++) {
                if (fds[i].fd == -1) {
                    fds[i].fd = client_fd;
                    fds[i].events = POLLIN;
                    break;
                }
            }
        }

        for (int i = 1; i <= max_size; i++) {
            if (fds[i].fd == -1) continue;

            if (fds[i].revents & POLLIN) {
                handle_request(fds[i].fd);
                close(fds[i].fd);
                fds[i].fd = -1;
            }
        }
    }
}
