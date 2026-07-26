#define _GNU_SOURCE

#include <sys/unistd.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <ifaddrs.h>
#include <linux/if.h>

#include "main.h"
#include "cmd_gui.h"
#include "sender.h"

struct in_addr get_local_addr(void){
    struct ifaddrs *ifaddr, *ifa;
    struct in_addr result = {0};

    getifaddrs(&ifaddr);
    for(ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next){
        if(ifa->ifa_addr == NULL) continue;
        if(ifa->ifa_addr->sa_family != AF_INET) continue;
        if(ifa->ifa_flags & IFF_LOOPBACK) continue;

        result = ((struct sockaddr_in *)ifa->ifa_addr)->sin_addr;
        break;
    }

    freeifaddrs(ifaddr);
    return result;
}

void *send_message(void *arg)
{
    int secret_port = *(int *)arg;
    int sockfd;
    int broadcast_val = 1;
    struct sockaddr_in broadcast_addr;

    check_result((sockfd = socket(PF_INET, SOCK_DGRAM, 0)), "Socket");

    check_result(setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, &broadcast_val, sizeof broadcast_val), "Setsockopt");

    /**
     * Get the broadcast ip from the host ip and broadcast mask
     * broadcast ip = host ip | ~broadcast mask
     */

    const char *mask_str = "255.255.255.0";
    struct in_addr mask;
    check_result(inet_pton(AF_INET, mask_str, &mask), "Invalid Address");

    struct in_addr local_addr = get_local_addr();
    struct in_addr broadcast_ip;
    broadcast_ip.s_addr = local_addr.s_addr | ~mask.s_addr;

    broadcast_addr.sin_family = AF_INET;
    broadcast_addr.sin_port = htons(secret_port);
    broadcast_addr.sin_addr = broadcast_ip;
    memset(broadcast_addr.sin_zero, '\0', sizeof broadcast_addr.sin_zero);

    // char ba_ip[INET_ADDRSTRLEN];
    // inet_ntop(AF_INET, &broadcast_ip, ba_ip, sizeof(ba_ip));
    // printf("Broadcasting to: %s\n", ba_ip);

    /** Generate Packet Header */

    GossipHeader_t hdr = {0};
    hdr.signature = htons(GOSSIP_SIGN);
    hdr.version = GOSSIP_VERSION;
    hdr.sender_id = htonl(my_id);
    hdr.ttl = htonl(30);
    
    char packet[sizeof(GossipHeader_t) + MAXBUFLEN];
    
    while (1)
    {
        int numbytes;
        char *msg = malloc(sizeof(char) * MAXBUFLEN);
        
        read_input(msg, MAXBUFLEN);
        hdr.timestamp = htonl((uint32_t)time(NULL));
        hdr.payload_len = htons((uint16_t)(strlen(msg)));

        memcpy(packet, &hdr, sizeof(GossipHeader_t));
        memcpy(packet + sizeof(GossipHeader_t), msg, strlen(msg));

        check_result((numbytes = sendto(sockfd, packet, strlen(msg) + sizeof(GossipHeader_t), 0, (struct sockaddr *)&broadcast_addr, sizeof broadcast_addr)), "sendto");

        free(msg);
    }

    close(sockfd);

    pthread_exit(NULL);
}