/* Receiver: data forwarding plus one-loss-per-block XOR recovery. */
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
#define BLOCK_SLOTS 16
#define MAGIC 0xA7
#define VERSION 1
#define TYPE_DATA 0
#define TYPE_PARITY 1

typedef struct {
    uint32_t base;
    uint8_t initialized;
    uint8_t present[BLOCK_SIZE];
    uint8_t sent[BLOCK_SIZE];
    uint8_t payload[BLOCK_SIZE][PAYLOAD_BYTES];
    uint8_t parity_present;
    uint8_t parity[PAYLOAD_BYTES];
} block_state;

static void reset_block(block_state *block, uint32_t base) {
    memset(block, 0, sizeof *block);
    block->initialized = 1;
    block->base = base;
}

static void send_frame(int fd, const struct sockaddr_in *player, uint32_t seq,
                       const uint8_t payload[PAYLOAD_BYTES]) {
    uint8_t frame[FRAME_BYTES];
    uint32_t seq_be = htonl(seq);
    memcpy(frame, &seq_be, sizeof seq_be);
    memcpy(frame + 4, payload, PAYLOAD_BYTES);
    sendto(fd, frame, sizeof frame, 0, (const struct sockaddr *)player,
           sizeof *player);
}

/* If exactly one member is absent, XOR of parity and the other members is it. */
static void try_recover(block_state *block, int out_fd,
                        const struct sockaddr_in *player) {
    if (!block->parity_present) return;

    int missing = -1;
    for (int i = 0; i < BLOCK_SIZE; ++i) {
        if (!block->present[i]) {
            if (missing != -1) return;  /* Two or more losses cannot be XORed. */
            missing = i;
        }
    }
    if (missing == -1) return;

    memcpy(block->payload[missing], block->parity, PAYLOAD_BYTES);
    for (int i = 0; i < BLOCK_SIZE; ++i) {
        if (i == missing) continue;
        for (size_t j = 0; j < PAYLOAD_BYTES; ++j)
            block->payload[missing][j] ^= block->payload[i][j];
    }
    block->present[missing] = 1;
    if (!block->sent[missing]) {
        send_frame(out_fd, player, block->base + (uint32_t)missing,
                   block->payload[missing]);
        block->sent[missing] = 1;
    }
}

int main(void) {
    int in_fd = socket(AF_INET, SOCK_DGRAM, 0);
    struct sockaddr_in in_addr = {0};
    in_addr.sin_family = AF_INET;
    in_addr.sin_port = htons(47002);
    in_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    if (bind(in_fd, (struct sockaddr *)&in_addr, sizeof in_addr) < 0) {
        perror("bind 47002");
        return 1;
    }

    int out_fd = socket(AF_INET, SOCK_DGRAM, 0);
    struct sockaddr_in player = {0};
    player.sin_family = AF_INET;
    player.sin_port = htons(47020);
    player.sin_addr.s_addr = inet_addr("127.0.0.1");

    block_state blocks[BLOCK_SLOTS] = {0};
    uint8_t wire[2048];
    for (;;) {
        ssize_t n = recvfrom(in_fd, wire, sizeof wire, 0, NULL, NULL);
        if (n != WIRE_BYTES || wire[0] != MAGIC || wire[1] != VERSION ||
            wire[8] != BLOCK_SIZE)
            continue;

        uint8_t type = wire[2];
        uint8_t index = wire[3];
        if ((type == TYPE_DATA && index >= BLOCK_SIZE) ||
            (type == TYPE_PARITY && index != BLOCK_SIZE) ||
            (type != TYPE_DATA && type != TYPE_PARITY))
            continue;

        uint32_t base_be;
        memcpy(&base_be, wire + 4, sizeof base_be);
        uint32_t base = ntohl(base_be);
        if (base % BLOCK_SIZE != 0) continue;

        block_state *block = &blocks[(base / BLOCK_SIZE) % BLOCK_SLOTS];
        if (!block->initialized || block->base != base)
            reset_block(block, base);

        if (type == TYPE_DATA) {
            if (!block->present[index]) {
                memcpy(block->payload[index], wire + HEADER_BYTES, PAYLOAD_BYTES);
                block->present[index] = 1;
                if (!block->sent[index]) {
                    send_frame(out_fd, &player, base + index, block->payload[index]);
                    block->sent[index] = 1;
                }
            }
        } else {
            memcpy(block->parity, wire + HEADER_BYTES, PAYLOAD_BYTES);
            block->parity_present = 1;
        }
        try_recover(block, out_fd, &player);
    }
}
