
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <fcntl.h>

#include "irc.h"

#define LINE_BUFFER_SIZE 4028
#define MAX_MESSAGE_SIZE 2000
#define MAX_HOSTNAME_SIZE 256
#define MAX_CHANNEL_NAME_SIZE 256

int IRC_connectTimeout(int __fd, void *__addr, socklen_t __len, uint64_t timeout);
int IRC_netConnect(const char *addr, uint16_t port);

int IRC_connect(IRC_t *irc) {

    irc->fd = IRC_netConnect(irc->addr, irc->port);

    if (irc->fd < 0)
        return -__LINE__;
    
    irc->fl = fdopen(irc->fd, "r+");

    if (
        fprintf(irc->fl, "NICK %s\r\n", irc->username) < 0 || 
        fprintf(irc->fl, "USER %s %s 127.0.0.1 :%s\r\n", irc->username, irc->username, irc->username) < 0 ||
        fflush(irc->fl) < 0
    ) return -__LINE__;

    return 0;
}

int IRC_join(IRC_t *irc, const char *channel) {
    if (channel[0] == '#')
        channel++;
    
    if (
        fprintf(irc->fl, "JOIN #%s\r\n", channel) < 0 ||
        fflush(irc->fl) < 0
    ) return -__LINE__;
}

int IRC_sendmsg(IRC_t *irc, const char *channel, const char *msg) {
    if (
        fprintf(irc->fl, "PRIVMSG %s :%s\r\n", channel, msg) < 0 ||
        fflush(irc->fl) < 0
    ) return -__LINE__;
}

int IRC_quit(IRC_t *irc) {
    if (
        fprintf(irc->fl, "QUIT\r\n") < 0 ||
        fflush(irc->fl) < 0
    ) return -__LINE__;
}

void IRC_processMessage(IRC_t *irc, char *packet) {
    char sender[MAX_HOSTNAME_SIZE] = "";
    char message[MAX_MESSAGE_SIZE] = "";
    char channel[MAX_CHANNEL_NAME_SIZE] = "";

    sscanf(packet, ":%s PRIVMSG %s :%2000[^\n]", sender, channel, message);

    for (size_t i = 0; sender[i] != '\x00'; i++) {
        if (sender[i] == '!' || sender[i] == '@') {
            sender[i] = '\x00';
            break;
        }
    }

    message[strlen(message)-1] = 0;

    if (irc->onMessage)
        irc->onMessage(irc, sender, (channel[0] != '#') ? sender : channel, message);
}

void IRC_runforever(IRC_t *irc) {
    char buffer[5000] = "";

    while (fgets(buffer, sizeof(buffer)-1, irc->fl)) {
        if (strstr(buffer, "PING :") == buffer) {
            fputs("PONG ", irc->fl);
            fputs(buffer+6, irc->fl);
        } else if (strstr(buffer, " PRIVMSG ")) {
            IRC_processMessage(irc, buffer);
        }
        
        fflush(irc->fl);
        memset(buffer, 0, sizeof(buffer));
    }

    close(irc->fd);
}

int IRC_netConnect(const char *addr, uint16_t port) {
    struct addrinfo hints;
    struct addrinfo *result, *rp;
    int sfd, s;
    size_t len;
    ssize_t nread;
    char portStr[20] = "";
    int tmp = 0;

    sprintf(portStr, "%hu", port);

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = 0;
    hints.ai_protocol = 0;

    s = getaddrinfo(addr, portStr, &hints, &result);
    if (s != 0) {
        return -__LINE__;
    }

    for (rp = result; rp != NULL; rp = rp->ai_next) {
        sfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (sfd == -1)
            continue;

        if ((tmp = IRC_connectTimeout(sfd, rp->ai_addr, rp->ai_addrlen, 1000)) >= 0)
            break;

        close(sfd);
    }

    freeaddrinfo(result);

    if (rp == NULL)
        return tmp;

    return sfd;
}

int IRC_connectTimeout(int __fd, void *__addr, socklen_t __len, uint64_t timeout) {

    struct timeval tv; // timeout
    int res; // connection response
    long arg; // socket args
    fd_set myset;
    int valopt;
    socklen_t lon;

    tv.tv_usec = (timeout%1000);
    tv.tv_sec  = (timeout/1000);

    // Set non-blocking 
    if((arg = fcntl(__fd, F_GETFL, NULL)) < 0)
        return -1;

    arg |= O_NONBLOCK; 
    if(fcntl(__fd, F_SETFL, arg) < 0)
        return -1;

    // Trying to connect with timeout 
    res = connect(__fd, __addr, __len); 
    
    if (res < 0) {
        if (errno == EINPROGRESS) {
            FD_ZERO(&myset); 
            FD_SET(__fd, &myset); 
            res = select(__fd+1, NULL, &myset, NULL, &tv); 
            if (res < 0 && errno != EINTR)
                return -1;
            else if (res > 0) {

                lon = sizeof(int);

                if (getsockopt(__fd, SOL_SOCKET, SO_ERROR, (void*)(&valopt), &lon) < 0)
                    return -__LINE__;
                
                if (valopt) {
                    errno = valopt;
                    return -__LINE__;
                } 
            } else
                return -__LINE__;
        }  else
            return -__LINE__;
    } 

    errno = 0;

    // Set to blocking mode again... 
    if((arg = fcntl(__fd, F_GETFL, NULL)) < 0)
        return -__LINE__;
    
    arg &= (~O_NONBLOCK); 
    if(fcntl(__fd, F_SETFL, arg) < 0) 
        return -__LINE__;

    return 0;
}