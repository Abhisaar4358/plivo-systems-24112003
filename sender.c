/* Sender: custom framing plus XOR parity for every two frames. */
#include <arpa/inet.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define PAYLOAD_BYTES 160
#define FRAME_BYTES (4 + PAYLOAD_BYTES)
#define HEADER_BYTES 12
#define WIRE_BYTES (HEADER_BYTES + PAYLOAD_BYTES)
#define BLOCK_SIZE 2
#define MAGIC 0xA7
#define VERSION 1
#define TYPE_DATA 0
#define TYPE_PARITY 1

/*
 * Every relay packet is 172 bytes:
 *   0 magic, 1 version, 2 type, 3 index,
 *   4..7 big-endian block-base sequence, 8 block size, 9..11 reserved,
 *   12..171 data payload or the XOR parity payload.
 */
static void send_wire(int fd, const struct sockaddr_in *relay, uint8_t type,
                      uint8_t index, uint32_t base,
                      const uint8_t body[PAYLOAD_BYTES]) {
    uint8_t wire[WIRE_BYTES] = {0};
    uint32_t base_be = htonl(base);
    wire[0] = MAGIC;
    wire[1] = VERSION;
    wire[2] = type;
    wire[3] = index;
    memcpy(wire + 4, &base_be, sizeof base_be);
    wire[8] = BLOCK_SIZE;
    memcpy(wire + HEADER_BYTES, body, PAYLOAD_BYTES);
    sendto(fd, wire, sizeof wire, 0, (const struct sockaddr *)relay,
           sizeof *relay);
}

int main(void) {
    int in_fd = socket(AF_INET, SOCK_DGRAM, 0);
    struct sockaddr_in in_addr = {0};
    in_addr.sin_family = AF_INET;
    in_addr.sin_port = htons(47010);
    in_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    if (bind(in_fd, (struct sockaddr *)&in_addr, sizeof in_addr) < 0) {
        perror("bind 47010");
        return 1;
    }

    int out_fd = socket(AF_INET, SOCK_DGRAM, 0);
    struct sockaddr_in relay = {0};
    relay.sin_family = AF_INET;
    relay.sin_port = htons(47001);
    relay.sin_addr.s_addr = inet_addr("127.0.0.1");

    uint8_t frame[2048];
    uint8_t parity[PAYLOAD_BYTES] = {0};
    uint32_t current_base = 0;
    for (;;) {
        ssize_t n = recvfrom(in_fd, frame, sizeof frame, 0, NULL, NULL);
        if (n != FRAME_BYTES) continue;

        uint32_t seq_be;
        memcpy(&seq_be, frame, sizeof seq_be);
        uint32_t seq = ntohl(seq_be);
        uint32_t base = seq - (seq % BLOCK_SIZE);
        uint8_t index = (uint8_t)(seq % BLOCK_SIZE);
        const uint8_t *payload = frame + 4;

        if (index == 0 || base != current_base) {
            memset(parity, 0, sizeof parity);
            current_base = base;
        }
        for (size_t i = 0; i < PAYLOAD_BYTES; ++i)
            parity[i] ^= payload[i];

        send_wire(out_fd, &relay, TYPE_DATA, index, base, payload);
        if (index == BLOCK_SIZE - 1)
            send_wire(out_fd, &relay, TYPE_PARITY, BLOCK_SIZE, base, parity);
    }
}
