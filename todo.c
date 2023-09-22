#include <netinet/in.h>
#include <stdbool.h>
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
  int id;
  char *todo;
  bool completed;
} Todo;

#define TODO_SIZE 100

static Todo todos[TODO_SIZE];
static int todos_len = 0;

char *todo_to_string(Todo *t) {
  char *str = malloc(1000);
  memset(str, '\0', 1000);
  sprintf(str, " \
      <div style=\"display: flex; width: 100%%; align-items: center; gap: 5px\"> \
        <input type=\"checkbox\" %s style=\"padding: 10px; height: 20px; width: 20px;\" hx-trigger=\"click\" hx-put=\"/%d\" hx-target=\"#items\" > \
        <button style=\"display: flex; width: 20px; height: 20px; align-items: center; justify-content: center;\" hx-trigger=\"click\" hx-delete=\"/%d\" hx-target=\"#items\">x</button> \
        <p style=\"padding: 10px; flex-grow: 1; margin: 0px; border: 1px solid black;\" type=\"text\">%s</p> \
      </div>",
          t->completed ? "checked" : "", t->id, t->id, t->todo);
  return str;
}

char *todos_to_string() {
  char *str = malloc(10000);
  memset(str, '\0', 10000);
  for (int i = 0; i < todos_len; i++) {
    if (todos[i].todo == NULL) {
      continue;
    }
    char *temp = todo_to_string(&todos[i]);
    strcat(str, temp);
    free(temp);
  }
  return str;
}

void add_todo(const char *todo) {
  todos_len = todos_len % TODO_SIZE;
  todos[todos_len] = (Todo){0};
  todos[todos_len].id = time(NULL);
  todos[todos_len].completed = false;
  todos[todos_len].todo = malloc(strlen(todo));
  memset(todos[todos_len].todo, '\0', strlen(todo));
  strcpy(todos[todos_len].todo, todo);
  todos_len++;
}

void delete_todo(int id) {
  for (int i = 0; i < todos_len; i++) {
    if (todos[i].id == id) {
      todos[i] = (Todo){0};
      return;
    }
  }
}

void toggle_complete_todo(int id) {
  for (int i = 0; i < todos_len; i++) {
    if (todos[i].id == id) {
      todos[i].completed = !todos[i].completed;
      return;
    }
  }
}

typedef struct {
  char *key;
  char *value;
} Header;

void handle_get(int *client_sock, char *buf) {
  char *url = strstr(buf, "/");
  url++;
  if (strncmp(url, "all", 2) == 0) {
    char *msg = malloc(128 * 1024);
    memset(msg, '\0', 128 * 1024);
    char *temp = todos_to_string();
    sprintf(msg, "HTTP/1.1 200 OK\nContent-Type: text/html\n\n%s", temp);
    write(*client_sock, msg, strlen(msg));
    free(temp);
    free(msg);
    return;
  }
  FILE *fp = fopen("index.html", "r");
  char *msg = malloc(128 * 1024);
  memset(msg, '\0', 128 * 1024);
  strcat(msg, "HTTP/1.1 200 OK\nContent-Type: text/html\n\n");
  char *line = NULL;
  size_t len = 0;
  while (getline(&line, &len, fp) != -1) {
    strcat(msg, line);
  }
  fclose(fp);
  write(*client_sock, msg, strlen(msg));
  free(msg);
}

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
      handle_get(&client_sock, buf);
    }
    if (strncmp(buf, "POST", 4) == 0) {
      char *body = strstr(buf, "\r\n\r\n");
      body += 9;
      add_todo(body);
      char *msg = malloc(512 * 1024);
      memset(msg, '\0', 512 * 1024);
      char *temp = todos_to_string();
      sprintf(msg, "HTTP/1.1 200 OK\nContent-Type: text/html\n\n%s", temp);
      write(client_sock, msg, strlen(msg));
      free(msg);
      free(temp);
    }
    if (strncmp(buf, "PUT", 3) == 0) {
      printf("PUT\n");
      char *temp = strstr(buf, "/");
      temp++;
      int id = 0;
      while (*temp != ' ') {
        id = id * 10 + *temp - '0';
        temp++;
      }
      toggle_complete_todo(id);
      char *msg = malloc(512 * 1024);
      memset(msg, '\0', 512 * 1024);
      char *todos = todos_to_string();
      sprintf(msg, "HTTP/1.1 200 OK\nContent-Type: text/html\n\n%s", todos);
      write(client_sock, msg, strlen(msg));
      free(msg);
      free(todos);
    }
    if (strncmp(buf, "DELETE", 6) == 0) {
      printf("DELETE\n");
      char *temp = strstr(buf, "/");
      temp++;
      int id = 0;
      while (*temp != ' ') {
        id = id * 10 + *temp - '0';
        temp++;
      }
      delete_todo(id);
      char *msg = malloc(512 * 1024);
      memset(msg, '\0', 512 * 1024);
      char *todos = todos_to_string();
      sprintf(msg, "HTTP/1.1 200 OK\nContent-Type: text/html\n\n%s", todos);
      write(client_sock, msg, strlen(msg));
      free(msg);
      free(todos);
    }
    free(buf);
    close(client_sock);
    printf("Waiting for next connection...\n");
  }
  close(sockfd);
  return 0;
}
