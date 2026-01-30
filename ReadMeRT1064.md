# RT1064 ⇄ SAMS70 SPI Bridge Overview

This document explains how the RT1064 USB-host firmware transfers HID/CDC
information to the SAMS70 over the SPI bridge and how the SAMS70 consumes it.

## Protocol summary

The RT1064 acts as an SPI **slave** and exposes a small register-like map of
logical endpoints (EN):

- **EN 0**: Hub status bitmap (HID slots + CDC present)
- **EN 1–4**: HID device slots
- **EN 5**: CDC endpoint

Each endpoint is represented by a variable-length block:

- **Header (1 byte)**: DIRTY / TYPE / LEN
- **Payload (LEN bytes)**: up to 63 bytes
- **CRC (2 bytes)**: CRC-16/IBM over header + payload

Only `1 + LEN + 2` bytes are clocked per block.

## Command flow

Every SPI transaction begins with a single command byte:

```
[op << 6] | EN
```

Operations:

- **READ_HEADER (op=0)**: returns only the header.
- **READ_BLOCK (op=1)**: returns header + payload + CRC and clears DIRTY.
- **WRITE_BLOCK (op=2)**: host sends header + payload + CRC for OUT endpoints.

## How SAMS70 reads data

On RT1064-capable boards, the SAMS70 runs a FreeRTOS task that polls the SPI
bridge:

1. **Poll hub status (EN 0):**
   - READ_HEADER to see if DIRTY is set.
   - READ_BLOCK when DIRTY=1, then update local endpoint-active flags.
2. **Poll active endpoints (EN 1–5):**
   - READ_HEADER to check DIRTY.
   - READ_BLOCK for payloads with CRC validation.
   - Store HID IN or CDC IN payloads locally.

## DIRTY/TYPE/LEN fields

- **DIRTY (bit 0):** New data available.
- **TYPE (bit 1):** Distinguishes descriptor vs. data payloads.
- **LEN (bits 7:2):** Payload byte count.

## CRC validation

CRC is computed over the header and payload bytes. The SAMS70 validates CRC
before accepting a block. CRC mismatches are ignored and retried on the next
poll.

## Wire requirements

The SAMS70 must clock exactly `1 + LEN + 2` bytes for each READ_BLOCK or
WRITE_BLOCK. Extra clocks leave data in the RT1064 RX FIFO and can desync the
protocol.

## Explanation: How the RT1064 Transfers Information to the SAMS70

Below is a detailed walkthrough of the data flow between the RT1064 (SPI slave)
and the SAMS70 (SPI master), grounded in the current code and protocol
documentation.

### 1) Protocol overview

The RT1064 SPI bridge publishes USB activity through logical endpoints (EN):

- EN 0: Hub status
- EN 1–4: HID device slots
- EN 5: CDC endpoint

Each endpoint exposes a header with DIRTY/TYPE/LEN, a payload of LEN bytes, and
CRC16 for integrity. The master clocks exactly `1 + LEN + 2` bytes per block.

### 2) SAMS70 polling loop

On RT1064-capable boards, the SAMS70 runs a FreeRTOS task that performs:

1. **Hub poll (EN 0)**
   - READ_HEADER to check DIRTY.
   - READ_BLOCK to fetch the hub bitmap when DIRTY is set.
   - Update local endpoint-active flags (HID slots + CDC).
2. **Endpoint poll (EN 1–5)**
   - READ_HEADER for each active endpoint.
   - READ_BLOCK when DIRTY is set.
   - Validate CRC and store HID IN or CDC IN payloads locally.

### 3) Command byte and read window

Every SPI transaction begins with one command byte:

```
[op << 6] | EN
```

The RT1064 expects:

- READ_HEADER: 1 additional byte to return the header.
- READ_BLOCK: exactly `1 + LEN + 2` bytes to return header/payload/CRC.
- WRITE_BLOCK: exactly `1 + LEN + 2` bytes sent by the master.

### 4) DIRTY, TYPE, and LEN

- DIRTY: new data available and should be read.
- TYPE: descriptor vs. data payload distinction.
- LEN: byte count of valid payload.

### 5) CRC validation

The SAMS70 computes CRC16 (header + payload) and compares it with the trailing
CRC bytes. Blocks failing CRC are ignored and retried on the next poll.

### 6) FIFO discipline

The SAMS70 must not clock extra bytes. Extra clocks leave stale data in the
RT1064 RX FIFO and can desynchronize subsequent transactions.
