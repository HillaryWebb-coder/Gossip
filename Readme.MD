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

Both nodes run this derivation independently and arrive at the same port. One binds and listens. The other sends. No handshake, no directory, no coordination — the passphrase *is* the address.

### Sender identity

Every packet is prefixed with a 4-byte sender ID generated randomly at startup. The receive thread checks this ID against its own — packets originating from the same node are silently discarded, preventing the UDP socket from echoing your own transmissions back into the chat window.

### Terminal UI

A two-panel ncurses interface keeps sent and received messages in a scrolling chat window while input is handled in a fixed bar at the bottom. Concurrent access to the terminal from the send and receive threads is serialised via a mutex.

### Thread model

```
main
├── send_thread    — reads input, prefixes sender ID, transmits via sendto()
└── recv_thread    — binds derived port, loops on recvfrom(), filters own packets, prints to chat
```

Both threads share the ncurses UI state, protected by `ui_mutex`.

---

## Project status

| Feature | Status |
|---|---|
| Passphrase → port derivation (SHA-256) | ✅ |
| Raw UDP send / receive | ✅ |
| Sender ID stamping and filtering | ✅ |
| ncurses terminal UI | ✅ |
| Concurrent send + receive threads | ✅ |
| Packet TTL and ephemeral message reaping | 🔧 In progress |
| Binary packet framing (header + checksum) | 🔧 Planned |
| Message fragmentation and reassembly | 🔧 Planned |

Ephemerality — the defining property of a true dead-drop — is the next milestone. Each packet will carry a TTL field; a background reaper thread will evict incomplete or uncollected message buffers once their TTL expires.

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
gcc -Wall -I. -o build/server main.c cmd_gui.c sha256.c -lncurses -lpthread
```

> Note: the Makefile does not currently pass `-lpthread`. If the build fails with undefined pthread references, add it to `CFLAGS` or link manually as above.

---

## Usage

Both nodes must be on the same network and use the same passphrase.

**Node A:**
```bash
./build/server mysecretphrase 192.168.0.255
```

**Node B:**
```bash
./build/server mysecretphrase 192.168.0.255
```

Both nodes derive the same port from `mysecretphrase`. Node A listens and sends to Node B's IP; Node B does the same in reverse. Messages appear in the chat window as they arrive.

---

## File structure

```
.
├── main.c          — entry point, thread orchestration, port derivation
├── cmd_gui.c       — ncurses UI (init, print_message, read_input, close)
├── cmd_gui.h       — UI interface and shared ui_mutex declaration
├── sha256.c        — SHA-256 implementation
├── sha256.h        — SHA-256 types and function declarations
└── Makefile
```

---

## Concepts explored

- Raw UDP socket programming (BSD sockets API)
- Deterministic port derivation from a cryptographic hash
- POSIX threads — concurrent send/receive with mutex synchronisation
- ncurses terminal UI with thread-safe rendering
- Packet identity and filtering without a connection handshake
- Protocol design from first principles

---

## Planned

- **TTL reaper thread** — background goroutine evicting expired message buffers
- **Binary packet header** — magic bytes, sequence number, TTL field, checksum
- **Message fragmentation** — split messages exceeding MTU across multiple datagrams, reassemble on receipt
- **XOR stream cipher** — keystream derived from passphrase, applied over payload before transmission

---

## Part of a larger trajectory

Gossip is the protocol foundation for a planned embedded multiplayer system running on [Azrael](https://github.com/HillaryWebb-coder) — a bare-metal STM32F411 handheld console with an ESP32 Wi-Fi co-processor. The same dead-drop protocol, ported to FreeRTOS and lwIP, would allow two Azrael units to exchange game state over a local network with no server infrastructure.