#ifndef __MAIN_H_
#define __MAIN_H_

#include <stdint.h>

#define MAXBUFLEN 256
extern uint32_t my_id;

#define GOSSIP_SIGN 0x2759
#define GOSSIP_VERSION 0x01

/** Packet Header Typedef */
typedef struct {
    uint16_t signature; // Identifier
    uint8_t version;
    uint32_t sender_id;
    uint32_t timestamp;
    uint32_t ttl;
    uint16_t payload_len;
} __attribute__((packed)) GossipHeader_t;


void check_result(uint8_t res_errno, char *error_str);

#endif