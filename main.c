#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <pthread.h>
#include <string.h>
#include <errno.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

#include "sha256.h"
#include "cmd_gui.h"

struct send_thread_args
{
    int secret_port;
    char *hostname;
};

#define MAXBUFLEN 500

uint32_t gen_port_from_pass(char *passphrase)
{

    uint8_t hash[SHA256_DIGEST_SIZE];

    if (SHA256((const uint8_t *)passphrase, 5, hash) != 0)
    {
        fprintf(stderr, "Hashing failed!\n");
        return 1;
    }

    uint32_t val = (hash[0] << 24) | (hash[1] << 16) | (hash[2] << 8) | hash[3];
    uint32_t secret_port = (val % 16383) + 49152;

    return secret_port;
}

void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET)
    {
        return &(((struct sockaddr_in *)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6 *)sa)->sin6_addr);
}

#include <ifaddrs.h>
#include <net/if.h>

int get_local_ip(char *buf, size_t buflen) {
    struct ifaddrs *ifaddr, *ifa;

    if (getifaddrs(&ifaddr) == -1) {
        perror("getifaddrs");
        return -1;
    }

    for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr == NULL) continue;

        // IPv4 only
        if (ifa->ifa_addr->sa_family != AF_INET) continue;

        // skip loopback
        if (ifa->ifa_flags & IFF_LOOPBACK) continue;

        inet_ntop(AF_INET,
                  &((struct sockaddr_in *)ifa->ifa_addr)->sin_addr,
                  buf, buflen);

        freeifaddrs(ifaddr);
        return 0;
    }

    freeifaddrs(ifaddr);
    return -1; // no suitable interface found
}

void *send_message(void *arg)
{
    struct send_thread_args *send_th_args = (struct send_thread_args *)arg;
    int sockfd;
    struct sockaddr_in recv_addr;
    struct hostent *he;
    int numbytes;
    int broadcast = 1;

    if ((he = gethostbyname(send_th_args->hostname)) == NULL)
    {
        perror("gethostbyname");
        printf("gethostbyname");
        exit(1);
    }

    if ((sockfd = socket(PF_INET, SOCK_DGRAM, 0)) == -1)
    {
        perror("socket");
        printf("socket");
        exit(1);
    }

    if (setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof broadcast) == -1)
    {
        perror("setsockopt (SO_BROADCAST)");
        printf("setsockopt");
        exit(1);
    }

    recv_addr.sin_family = AF_INET;
    recv_addr.sin_port = htons(send_th_args->secret_port);
    recv_addr.sin_addr = *((struct in_addr *)he->h_addr_list[0]);
    memset(recv_addr.sin_zero, '\0', sizeof recv_addr.sin_zero);

    while (1)
    {
        char *msg = malloc(sizeof(char) * MAXBUFLEN);

        read_input(msg, MAXBUFLEN);

        numbytes = sendto(sockfd, msg, strlen(msg), 0, (struct sockaddr *)&recv_addr, sizeof recv_addr);

        if (numbytes == -1)
        {
            perror("sendto");
            exit(1);
        }

        // printf("sent %d bytes to %s\n", numbytes, inet_ntoa(recv_addr.sin_addr));
        free(msg);
    }

    close(sockfd);
    free(send_th_args->hostname);
    free(send_th_args);

    pthread_exit(NULL);
}

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
    char s[INET_ADDRSTRLEN];

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
        if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1)
        {
            perror("listener: socket");
            continue;
        }

        if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1)
        {
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
        // printf("listener: waiting to recvfrom...\n");

        addr_len = sizeof their_addr;
        if ((numbytes = recvfrom(sockfd, buf, MAXBUFLEN - 1, 0, (struct sockaddr *)&their_addr, &addr_len)) == -1)
        {
            perror("recvfrom");
            exit(1);
        }

        // printf("listener: got packet from %s\n", inet_ntop(their_addr.ss_family, get_in_addr((struct sockaddr *)&their_addr), s, sizeof s));
        // printf("listener got packets %d bytes long\n", numbytes);
        buf[numbytes] = '\0';

        char host[INET_ADDRSTRLEN];
        struct in_addr host_ip;
        char separator[3] = "<<";
        
        get_local_ip(host, sizeof host);
        inet_pton(AF_INET, host, &host_ip);

        uint32_t their_addr_int = ntohl(((struct sockaddr_in *)&their_addr)->sin_addr.s_addr);
        uint32_t host_addr_int = ntohl(host_ip.s_addr);
        
        snprintf(separator, sizeof separator, "%s", their_addr_int == host_addr_int ? "<<" : ">>");
 
        pthread_mutex_lock(&ui_mutex);
        print_message(separator, buf);
        pthread_mutex_unlock(&ui_mutex);
    }

    close(sockfd);
    return 0;
}

int main(int argc, char *argv[])
{

    if (argc < 3)
    {
        fprintf(stderr, "No passphrase entered");
    }

    int secret_port = gen_port_from_pass(argv[1]);

    uint8_t status;
    pthread_t send_threadID;
    struct send_thread_args *send_th_args = malloc(sizeof(struct send_thread_args));
    send_th_args->hostname = strdup(argv[2]);
    send_th_args->secret_port = secret_port;

    init_ui();

    status = pthread_create(&send_threadID, NULL, send_message, (void *)send_th_args);
    if (status != 0)
    {
        printf("Error creating thread!\n");
        exit(-1);
    }

    pthread_t recv_threadID;

    status = pthread_create(&recv_threadID, NULL, recieve_message, (void *)&secret_port);
    if (status != 0)
    {
        printf("Error creating thread!\n");
        exit(-1);
    }

    pthread_join(send_threadID, NULL);
    pthread_join(recv_threadID, NULL);

    close_ui();

    return 0;
}