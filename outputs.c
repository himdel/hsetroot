#include <ctype.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "outputs.h"


OutputInfo
*outputs_list(int *count_out)
{
  OutputInfo *outputs = NULL;
  int noutputs = 0;

  int pipefd[2];
  if (pipe(pipefd) == -1) {
    perror("pipe");
    return NULL;
  }

  pid_t pid = fork();
  if (pid == -1) {
    perror("fork");
    return NULL;
  }

  if (pid == 0) {
    // child writes to pipe
    close(pipefd[0]);
    dup2(pipefd[1], STDOUT_FILENO);

    system("xrandr | grep ' connected' | sed -e 's/^.*\\s\\+\\([0-9]\\+x[0-9]\\+[-+][0-9]\\+[-+][0-9]\\+\\)\\s\\+.*$/\\1/'");

    close(pipefd[1]);
    _exit(0);
  } else {
    // parent parses the numbers
    close(pipefd[1]);

    int state = 0;
    int num = 0;

    char buf;
    while (read(pipefd[0], &buf, 1) > 0) {
      switch (state) {
        case 0: // start of input, after newline
          if (isspace(buf))
            continue;

          state = 1;
          outputs = realloc(outputs, ++noutputs * sizeof(OutputInfo));
          // pass

#define STATE(INIT, FIELD, END) \
        case INIT:\
          if (! isdigit(buf)) {\
            outputs[noutputs - 1].FIELD = num;\
            \
            state = END;\
            num = 0;\
            continue;\
          }\
          \
          num *= 10;\
          num += buf - '0';\
          \
          break;

        STATE(1, w, 2); // reading width
        STATE(2, h, 3); // reading height
        STATE(3, x, 4); // reading left
        STATE(4, y, 0); // reading top
#undef STATE
      }
    }

    wait(NULL);
    close(pipefd[0]);
  }

  *count_out = noutputs;
  return outputs;
}

void
outputs_free(OutputInfo *outputs)
{
  if (outputs)
    free(outputs);
}
