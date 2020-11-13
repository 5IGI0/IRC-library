/**
 * @file irc.h
 * @author 5IGI0 (https://github.com/5IGI0/)
 * @brief Simple library to make IRC bot.
 * @version 1.0
 * @date 2020-11-13
 * 
 */
#ifndef __IRC_H__
#define __IRC_H__

#include <stdio.h>
#include <stdint.h>

typedef struct __IRC{

    char *addr; // address used to connect
    uint16_t port; // port used to connect

    char *username; // username used by the bot

    int fd;   // connection file descriptor
    FILE *fl; // connection file pointer

    // function called on message
    void (*onMessage)(struct __IRC *client, const char *sender, const char *channel, const char *message);

} IRC_t;

/**
 * @brief connect to a server
 * 
 * @param irc IRC bot
 * @return negative integer while error (see errno), 0 on success.
 */
int IRC_connect(IRC_t *irc);

/**
 * @brief join IRC channel
 * 
 * @param irc IRC bot
 * @param channel channel name
 * @return 0 is returned if the **PACKET** is sent, negative integer on failure.
 */
int IRC_join(IRC_t *irc, const char *channel);

/**
 * @brief send a message to a channel/user
 * 
 * @param irc IRC bot
 * @param channel channel/user name (in channel case, you have to add # at start)
 * @param msg message to send
 * @return 0 is returned if the **PACKET** is sent, negative integer on failure.
 */
int IRC_sendmsg(IRC_t *irc, const char *channel, const char *msg);

/**
 * @brief send to the server a quit packet, the server will close the connection and IRC_runforever will stop.
 * 
 * @param irc IRC bot
 * @return 0 is returned if the **PACKET** is sent, negative integer on failure.
 */
int IRC_quit(IRC_t *irc);

/**
 * @brief Packet reading function, it will stop automatically when the connection is closed, and will call the on_message function.
 * 
 * @param irc IRC bot
 */
void IRC_runforever(IRC_t *irc);


#endif