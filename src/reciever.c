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
        if((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1){
            perror("listener: socket");
            continue;
        }
        
        if(bind(sockfd, p->ai_addr, p->ai_addrlen) == -1){
            close(sockfd);
            perror("listener: bind");
            continue;
        }
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
        char buf[MAXBUFLEN + sizeof(GossipHeader_t)];
        check_result((numbytes = recvfrom(sockfd, buf, MAXBUFLEN + sizeof(GossipHeader_t) - 1, 0, (struct sockaddr *)&their_addr, &addr_len)), "recvfrom");
        
        GossipHeader_t *hdr = (GossipHeader_t *)buf;
        if(ntohs(hdr->signature) != GOSSIP_SIGN){
            continue; // Invalid Packet
        }

        uint32_t sent_at = ntohl(hdr->timestamp);
        uint32_t ttl = ntohl(hdr->ttl);
        uint32_t now = (uint32_t)time(NULL);

        if(now - sent_at > ttl) continue; // discard expired messages

        if(ntohl(hdr->sender_id) == my_id) continue; // discard own messages

        char *message = buf + sizeof(GossipHeader_t); // set message pointer to message location in buffer
        message[ntohs(hdr->payload_len)] = '\0';

        char separator[3] = "<<";
 
        pthread_mutex_lock(&ui_mutex);
        print_message(separator, message, sent_at);
        pthread_mutex_unlock(&ui_mutex);
    }

    close(sockfd);
    return 0;
}