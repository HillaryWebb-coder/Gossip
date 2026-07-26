# Gossip

A serverless, passphrase-keyed peer-to-peer messaging tool built on raw UDP. Two nodes share nothing but a passphrase — from it they independently derive the same port, find each other on the network, and exchange messages. No central server. No persistent connections. No directory.

Built from scratch in C as an exploration of protocol design, raw socket programming, and concurrent systems.

---

## How it works

### Dead-drop addressing

The core concept is borrowed from tradecraft. Instead of connecting to a server, both parties agree on a meeting point — a specific UDP port — derived deterministically from a shared passphrase.

```
passphrase → SHA-256 → first 4 bytes → uint32 → map to ephemeral port range (49152–65535)
```

Both nodes run this derivation independently and arrive at the same port. One binds and listens. The other sends to the network's broadcast address on that port. No handshake, no directory, no coordination — the passphrase *is* the address.

### Packet header

Every packet is framed with a binary header:

```
[signature: 2B][version: 1B][sender_id: 4B][timestamp: 4B][ttl: 4B][payload_len: 2B][payload...]
```

The signature validates that a packet is a genuine Gossip packet — stray UDP traffic on the derived port is discarded. The sender ID filters the node's own transmissions, which are visible on the socket because UDP broadcast is used. The timestamp and TTL allow the receiver to discard packets that are stale in transit.

### Terminal UI

A two-panel ncurses interface keeps sent and received messages in a scrolling chat window while input is handled in a fixed bar at the bottom. Concurrent access to the terminal from the send and receive threads is serialised via a mutex.

### Thread model

```
main
├── send_thread    — reads input, builds packet, derives broadcast address, transmits via sendto()
└── recv_thread    — binds derived port, loops on recvfrom(), validates header, prints to chat
```

Both threads share the ncurses UI state, protected by `ui_mutex`.

---

## Project status

| Feature | Status |
|---|---|
| Passphrase → port derivation (SHA-256) | ✅ |
| Raw UDP broadcast send / receive | ✅ |
| Binary packet header framing | ✅ |
| Sender ID stamping and filtering | ✅ |
| Packet TTL — stale packet rejection | ✅ |
| ncurses terminal UI | ✅ |
| Concurrent send + receive threads | ✅ |
| Message fragmentation and reassembly | 🔧 Planned |
| XOR stream cipher | 🔧 Planned |

---

## Build

**Dependencies**

- GCC
- ncurses (`sudo apt install libncurses5-dev`)
- pthreads (glibc, included on Linux)

**Build and run**

```bash
make
```

Or manually:

```bash
mkdir build/
gcc -Wall -I. -o build/server main.c sender.c receiver.c cmd_gui.c sha256.c -lncurses -lpthread
```

---

## Usage

Both nodes must be on the same network and use the same passphrase.

**Node A:**
```bash
./build/server mysecretphrase
```

**Node B:**
```bash
./build/server mysecretphrase
```

Both nodes independently derive the same port from `mysecretphrase`. Packets are sent to the subnet broadcast address so no IP configuration is needed — any node on the network with the correct passphrase will receive them.

---

## File structure

```
.
├── main.c          — entry point, thread orchestration, port derivation
├── main.h          — shared types, GossipHeader_t, constants
├── sender.c        — send thread, broadcast address derivation, packet assembly
├── sender.h
├── receiver.c      — receive thread, header validation, TTL check, message display
├── cmd_gui.c       — ncurses UI (init, print_message, read_input, close)
├── cmd_gui.h       — UI interface and shared ui_mutex declaration
├── sha256.c        — SHA-256 implementation
├── sha256.h        — SHA-256 types and function declarations
└── Makefile
```

---

## Concepts explored

- Raw UDP socket programming (BSD sockets API)
- UDP broadcast — subnet-wide delivery without a target IP
- Deterministic port derivation from a cryptographic hash
- Binary protocol design — custom wire format with versioning
- POSIX threads — concurrent send/receive with mutex synchronisation
- ncurses terminal UI with thread-safe rendering
- Network byte order — correct use of `htons`, `htonl`, `ntohs`, `ntohl`
- Packet validation — signature checks, TTL expiry, sender filtering

---

## Planned

- **Message fragmentation** — split messages exceeding MTU across multiple datagrams, reassemble on receipt
- **XOR stream cipher** — keystream derived from passphrase, applied over payload before transmission

---

## Part of a larger trajectory

Gossip is the protocol foundation for a planned embedded multiplayer system running on [Azrael](https://github.com/HillaryWebb-coder) — a bare-metal STM32F411 handheld console with an ESP32 Wi-Fi co-processor. The same dead-drop protocol, ported to FreeRTOS and lwIP, would allow two Azrael units to exchange game state over a local network with no server infrastructure.