#define _GNU_SOURCE

#include <sys/unistd.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "main.h"
#include "cmd_gui.h"

void *send_message(void *arg)
{
    int secret_port = *(int *)arg;
    int sockfd;
    struct hostent *he;
    int broadcast_val = 1;
    struct sockaddr_in broadcast_addr;

    

    char hostname[128];

    check_result(gethostname(hostname, sizeof hostname), "Hostname");

    if((he = gethostbyname(hostname)) == NULL)
    {
        herror("gethostbyname");
        exit(1);
    }

    check_result((sockfd = socket(PF_INET, SOCK_DGRAM, 0)), "Socket");

    check_result(setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, &broadcast_val, sizeof broadcast_val), "Setsockopt");

    /**
     * Get the broadcast ip from the host ip and broadcast mask
     * broadcast ip = host ip | ~broadcast mask
     */

    const char *mask_str = "255.255.255.0";
    struct in_addr mask;
    check_result(inet_pton(AF_INET, mask_str, &mask), "Invalid Address");

    struct in_addr broadcast_ip;
    broadcast_ip.s_addr = htonl(
        ntohl(((struct in_addr *)he->h_addr_list[0])->s_addr) | ntohl(~mask.s_addr)
    );

    broadcast_addr.sin_family = AF_INET;
    broadcast_addr.sin_port = htons(secret_port);
    broadcast_addr.sin_addr = broadcast_ip;
    memset(broadcast_addr.sin_zero, '\0', sizeof broadcast_addr.sin_zero);

    char *ba_ip = malloc(sizeof(char) * INET_ADDRSTRLEN);
    inet_ntop(AF_INET, &broadcast_ip, ba_ip, sizeof ba_ip);

    while (1)
    {
        int numbytes;
        char *msg = malloc(sizeof(char) * MAXBUFLEN);

        read_input(msg, MAXBUFLEN);

        check_result((numbytes = sendto(sockfd, msg, strlen(msg), 0, (struct sockaddr *)&broadcast_addr, sizeof broadcast_addr)), "sendto");

        free(msg);
    }

    close(sockfd);

    pthread_exit(NULL);
}