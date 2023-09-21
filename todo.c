#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#define handle_error(msg, sockfd)                                              \
  do {                                                                         \
    perror(msg);                                                               \
    close(sockfd);                                                             \
    exit(0);                                                                   \
  } while (0)

typedef struct {
  int created_at;
  char *todo;
} Todo;

static const TODO_SIZE = 100;
static Todo todos[TODO_SIZE];
static int todos_len = 0;

char *todo_to_string(Todo *t) {
  char *str = malloc(1000);
  memset(str, '\0', 1000);
  sprintf(str, "{\"created_at\": %d, \"todo\": \"%s\"}", t->created_at,
          t->todo);
  return str;
}

char *todos_to_string() {
  char *str = malloc(10000);
  memset(str, '\0', 10000);
  strcat(str, "[");
  for (int i = 0; i < todos_len; i++) {
    strcat(str, todo_to_string(&todos[i]));
    if (i != todos_len - 1) {
      strcat(str, ",");
    }
  }
  strcat(str, "]");
  printf("todos_to_string: %s\n", str);
  return str;
}

void add_todo(const char *todo) {
  todos_len = todos_len % TODO_SIZE;
  todos[todos_len] = (Todo){0};
  todos[todos_len].created_at = time(NULL);
  todos[todos_len].todo = malloc(strlen(todo));
  memset(todos[todos_len].todo, '\0', strlen(todo));
  strcpy(todos[todos_len].todo, todo);
  todos_len++;
}

typedef struct {
  char *key;
  char *value;
} Header;

void handle_request(char *request, int *client_sock) {}

int main(void) {
  int sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd == -1) {
    handle_error("socket", sockfd);
  }
  struct sockaddr_in addr;
  socklen_t addrlen = sizeof(addr);
  memset(&addr, 0, addrlen);
  addr.sin_family = AF_INET;
  addr.sin_port = htons(8080);
  addr.sin_addr.s_addr = INADDR_ANY;
  if (bind(sockfd, (struct sockaddr *)&addr, addrlen) == -1) {
    handle_error("bind", sockfd);
  };
  printf("Bound to port 8080\n");
  if (listen(sockfd, 10) == -1) {
    handle_error("listen", sockfd);
  };
  printf("Listening on port 8080\n");
  while (1) {
    int client_sock = accept(sockfd, (struct sockaddr *)&addr, &addrlen);
    if (client_sock == -1) {
      handle_error("accept", sockfd);
    };
    printf("Accepted connection\n");
    char *buf = malloc(10000);
    memset(buf, '\0', 10000);
    if (read(client_sock, buf, 10000) == -1) {
      handle_error("read", client_sock);
    }
    if (strncmp(buf, "GET", 3) == 0) {
      printf("GET\n");
      char *msg = malloc(11000);
      memset(msg, '\0', 11000);
      strcat(msg, "HTTP/1.1 200 OK\nContent-Type: application/json\n\n");
      strcat(msg, todos_to_string());
      printf("msg to be sent: %s", msg);
      write(client_sock, msg, strlen(msg));
      free(msg);
    }
    if (strncmp(buf, "POST", 4) == 0) {
      char *body = strstr(buf, "\r\n\r\n");
      body += 4;
      add_todo(body);
      printf("POST body: %s\n", buf);
      char *msg = "HTTP/1.1 200 OK\nContent-Type: application/json\n\nPOST";
      write(client_sock, msg, strlen(msg));
    }
    if (strncmp(buf, "PUT", 3) == 0) {
      printf("PUT\n");
      char *msg = "HTTP/1.1 200 OK\nContent-Type: application/json\n\nPUT";
      write(client_sock, msg, strlen(msg));
    }
    if (strncmp(buf, "DELETE", 6) == 0) {
      printf("DELETE\n");
      char *msg = "HTTP/1.1 200 OK\nContent-Type: application/json\n\nDELETE";
      write(client_sock, msg, strlen(msg));
    }
    free(buf);
    close(client_sock);
  }
  close(sockfd);
  return 0;
}
