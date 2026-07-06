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
#include "sender.h"
#include "reciever.h"



#define MAXBUFLEN 500

uint32_t gen_port_from_pass(char *passphrase);
void *get_in_addr(struct sockaddr *sa);

int main(int argc, char *argv[])
{

    if (argc < 2)
    {
        fprintf(stderr, "No passphrase entered\n");
        exit(1);
    }

    int secret_port = gen_port_from_pass(argv[1]);

    uint8_t status;
    pthread_t send_threadID;

    init_ui();

    status = pthread_create(&send_threadID, NULL, send_message, (void *)&secret_port);
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

    // pthread_join(send_threadID, NULL);
    pthread_join(recv_threadID, NULL);

    close_ui();

    return 0;
}

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

void check_result(uint8_t res_errno, char *error_str){
    if(res_errno == -1){
        perror(error_str);
        exit(1);
    }
}