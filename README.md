# IRC Library

A simple library to make an IRC bot in C.

example:

```c
#include <string.h>

#include "irc.h"

#define PF "!"

void on_message(IRC_t *irc, const char *sender, const char *channel, const char *message) {
    if (strstr(message, PF"quit") == message) {
        IRC_sendmsg(irc, channel, "bye \\o");
        IRC_quit(irc);
    }
}

int main(void) {

    IRC_t irc = {
        .username = "myBot",
        .addr = "127.0.0.1",
        .port = 6667,
        .onMessage = on_message
    };

    if (IRC_connect(&irc) < 0) {
        perror("IRC connection");
        return -__LINE__;
    }

    IRC_join(&irc, "test");
    IRC_sendmsg(&irc, "#test", "Hello, World!");

    IRC_runforever(&irc);

    return 0;
}
```