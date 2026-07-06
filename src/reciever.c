#define _GNU_SOURCE

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/unistd.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "main.h"
#include "cmd_gui.h"

void *recieve_message(void *arg)
{
    int secret_port = *(int *)arg;
    char secret_port_str[8];
    snprintf(secret_port_str, sizeof secret_port_str, "%d", secret_port);


    int sockfd;
    struct addrinfo hints, *servinfo, *p;
    int rv;
    int numbytes;
    struct sockaddr_storage their_addr;
    char buf[MAXBUFLEN];
    socklen_t addr_len;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE;

    if ((rv = getaddrinfo(NULL, secret_port_str, &hints, &servinfo)) != 0)
    {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        exit(1);
    }

    for (p = servinfo; p != NULL; p = p->ai_next)
    {
        check_result((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)), "listener: socket");

        check_result(bind(sockfd, p->ai_addr, p->ai_addrlen), "listener: bind");

        break;
    }

    if (p == NULL)
    {
        fprintf(stderr, "listener: failed to bind socket");
        exit(1);
    }

    freeaddrinfo(servinfo);
    while (1)
    {

        addr_len = sizeof their_addr;
        check_result((numbytes = recvfrom(sockfd, buf, MAXBUFLEN - 1, 0, (struct sockaddr *)&their_addr, &addr_len)), "recvfrom");

        buf[numbytes] = '\0';

        char hostname[128];
        struct hostent *he;
        char separator[3] = "<<";
        
        gethostname(hostname, sizeof hostname);
        if((he = gethostbyname(hostname)) == NULL){
            herror("gethostbyname");
        }

        uint32_t their_addr_int = ntohl(((struct sockaddr_in *)&their_addr)->sin_addr.s_addr);
        uint32_t host_addr_int = ntohl(((struct in_addr *)he->h_addr_list[0])->s_addr);
        
        snprintf(separator, sizeof separator, "%s", their_addr_int == host_addr_int ? ">>" : "<<");
 
        pthread_mutex_lock(&ui_mutex);
        print_message(separator, buf);
        pthread_mutex_unlock(&ui_mutex);
    }

    close(sockfd);
    return 0;
}