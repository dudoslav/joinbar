#define _POSIX_C_SOURCE 2

#include <unistd.h>
#include <poll.h>

#include <json-c/json.h>

#define BUFFER_SIZE 1024
#define VERSION_STR "{\"version\":1,\"click_events\":true}\n"
#define PREFIX_STR "[\n"
#define POSTFIX_STR "]\n"
#define SEPARATOR_STR ",\n"

static int run_status(const char *scmd) {
  int pfd[2];

  pipe(pfd);

  if (fork()) {
    /* Parent */
    close(pfd[1]);

    /* Read VERSION, [ and \n */
    char c;
    while (1) {
      read(pfd[0], &c, 1);
      if (c == '\n') break;
    }

    while (1) {
      read(pfd[0], &c, 1);
      if (c == '\n') break;
    }

    return pfd[0];
  } else {
    /* Child */
    close(pfd[0]);
    dup2(pfd[1], 1);
    close(pfd[1]);
    close(2);
    execl("/bin/sh", "sh", "-c", scmd, NULL);

    return -1;
  }
}

static json_object* merge(json_object **jss, size_t njss) {
  json_object *r, *e;
  size_t i, j;

  r = json_object_new_array();

  for (i = 0; i < njss; ++i) {
    if (!jss[i]) continue;

    for (j = 0; j < json_object_array_length(jss[i]); ++j) {
      e = json_object_array_get_idx(jss[i], j);
      json_object_array_add(r, json_object_get(e));
    }
  }

  return r;
}

static void on_status(json_object **jo, int fd) {
  char buf[BUFFER_SIZE] = {0};
  json_object *t;

  read(fd, buf, BUFFER_SIZE);
  t = json_tokener_parse(buf[0] == ',' ? buf + 1 : buf);

  if (json_object_is_type(t, json_type_array)) {
    if (*jo) json_object_put(*jo);
    *jo = t;
  }
}

int main(int argc, char **argv) {
  struct pollfd fds[argc - 1];
  json_object *fjo[argc - 1];
  json_object *rjo = NULL;
  const char *json = NULL;
  int i;
  size_t l;

  write(0, VERSION_STR, sizeof(VERSION_STR));
  write(0, PREFIX_STR, sizeof(PREFIX_STR));

  for (i = 0; i < argc - 1; ++i)
    fjo[i] = NULL;

  for (i = 0; i < argc - 1; ++i) {
    fds[i].fd = run_status(argv[i + 1]);
    fds[i].events = POLLIN;
  }

  while (argc > 1) {
    if (poll(fds, argc - 1, -1) <= 0) continue;

    for (i = 0; i < argc - 1; ++i) {
      if (!(fds[i].revents & POLLIN)) continue;

      on_status(&fjo[i], fds[i].fd);
    }

    rjo = merge(fjo, argc - 1);
    json = json_object_to_json_string_length(rjo, JSON_C_TO_STRING_PLAIN, &l);
    write(0, json, l);
    write(0, SEPARATOR_STR, sizeof(SEPARATOR_STR));
    json_object_put(rjo);
  }

  write(0, POSTFIX_STR, sizeof(POSTFIX_STR));

  return 0;
}
