# RT1064–SAMS70 USB HID/CDC SPI Bridge
Unified HID + CDC SPI Communication Layer

This repository implements a SPI communication bridge between:

- **NXP i.MX RT1064** (USB Host + SPI Slave)
- **Microchip SAMS70** (SPI Master)

The RT1064 runs the full USB Host stack (HID + CDC ACM + Hub), while the SAMS70 retrieves USB events, descriptors, and device reports using a **minimal, DIRTY-driven SPI protocol**.

This architecture allows the SAMS70 to treat all USB devices uniformly—HID joysticks and CDC virtual serial devices—without requiring any USB stack on the SAMS70.

---

## System Overview

```
 USB Devices (HID, CDC)
        |
   [Optional Hub]
        |
   +------------------+
   |   RT1064 MCU     |
   | USB Host + SPI   |
   |  Enumerator       |
   +------------------+
        |
 EN-Based SPI Protocol
        |
   +------------------+
   |   SAMS70 MCU     |
   |  Application     |
   +------------------+
```

---

## Logical Endpoint Model (EN)

The RT1064 exposes up to **6 logical endpoints**:

| EN | Description                         |
|----|-------------------------------------|
| 0  | Hub status (or first device if no hub) |
| 1  | HID Device 1                        |
| 2  | HID Device 2                        |
| 3  | HID Device 3                        |
| 4  | HID Device 4                        |
| 5  | CDC Device (dedicated port)         |

Each EN holds:

- 1 header byte
- 1–63 bytes of payload
- 2-byte CRC16

The CDC endpoint (EN5) mirrors the USB device's CDC ACM bulk pipes. Bytes received from the host PC on BULK OUT are
published to the SPI master as TYPE=0 payloads, while bytes the master writes to EN5 are transmitted to the PC over the
CDC BULK IN pipe. The hub bitmap includes an extra slot for this channel and stays asserted so the SAMS70 always knows
the CDC lane is available.

TYPE bit:

- `TYPE = 0` → Data report (HID IN or CDC IN)
- `TYPE = 1` → Descriptor (HID or CDC)

---

## SPI Command Format

Each SPI transaction begins with a **command byte** encoded as `(op << 6) | EN`:

| Bits | Meaning                                             |
|------|-----------------------------------------------------|
| 7..6 | Operation (READ_HEADER, READ_BLOCK, WRITE_BLOCK)    |
| 5..0 | EN index (0–5)                                      |

Operations:

| op  | Meaning       | Payload transferred                 |
|-----|---------------|-------------------------------------|
| `00`| READ_HEADER   | Host clocks one extra byte; slave returns header only (DIRTY/TYPE/LEN). DIRTY is **not** cleared. |
| `01`| READ_BLOCK    | Host clocks `1 + LEN + 2` bytes; slave returns header, LEN payload bytes, CRC16 and then clears DIRTY. |
| `10`| WRITE_BLOCK   | Host sends header + payload + CRC16; slave validates CRC/LEN, stores OUT data, and marks DIRTY. |
| `11`| Reserved      | —                                   |

---

## Register Block Format

```
Byte 0      Header (DIRTY, TYPE, LEN[7:2])
Bytes 1–63  Payload (HID or CDC data)
Byte 64–65  CRC16-CCITT/IBM (poly 0x1021, init 0xFFFF, little-endian)
```

Header field breakdown:

- **DIRTY (bit 0):** Set when new data is available. Cleared after a successful `READ_BLOCK` or explicit clear for OUT buffers.
- **TYPE (bit 1):** `0` for data payloads (IN reports or CDC data), `1` for descriptors/notifications.
- **LEN (bits 7..2):** Number of valid payload bytes (0–63).

---

## Variable-Length SPI Transfers (LEN-Dependent)

Although each logical EN block is stored internally as **66 bytes**, the SPI bridge *does not* transfer all 66 bytes.

### READ_BLOCK
The SPI master (SAMS70) must clock exactly:

```
1 (header) + LEN (payload) + 2 (CRC)
```

Example:
If LEN=8 → total wire bytes = 11.

### WRITE_BLOCK  
The master must transmit:

```
1 + LEN + 2 bytes
```

Sending more causes stray bytes in RT1064’s SPI RX FIFO.

> **Tip:** Extra clocks after either READ or WRITE are treated as padding by the RT1064 but will remain queued in its RX FIFO. The bridge resets the FIFO at the start of each command, so the master should always send the exact `(1 + LEN + 2)` byte count for deterministic behavior.

---

## DIRTY Semantics

DIRTY=1 when:

- HID IN report arrives  
- CDC IN bulk data arrives  
- Device connect/disconnect  
- Descriptor updated  
- CDC line coding changed  
- OUT block processed  

DIRTY clears only after a complete READ_BLOCK.

For OUT buffers (HID or CDC), the RT1064 clears DIRTY only after the USB host consumes the payload and the firmware calls the corresponding `SPI_BridgeClear*` helper. The SPI master should treat DIRTY as read-only.

---

## RT1064 FIFO Behavior

The RT1064 SPI slave hardware uses a small receive FIFO.

### FIFO rules:

1. **Any extra bytes clocked by the master are pushed into the FIFO.**
2. These bytes do *not* belong to any EN block.
3. On the next transaction, stale bytes may be output unless cleared.
4. The firmware manually resets the FIFO at the start of each transfer.

### Implication:
**The SAMS70 must send the exact number of bytes required.**

More clocks = protocol desynchronization.

**Hub status payload:** `payload[0..3]` mirror HID slots EN1–EN4; `payload[4]` is always `1` to indicate the CDC logical endpoint (EN5) is present. A zeroed descriptor (TYPE=1, LEN=0) signals removal of a previously mapped device.

---

## Validation Rules (SAMS70 Side)

The SAMS70 should validate every incoming block:

### 1. **Bad LEN**
If LEN > 63 → ignore header and re-read.

### 2. **Missing DIRTY**
If DIRTY=0 → do not perform a READ_BLOCK.

### 3. **CRC Fail**
If CRC mismatch:
- ignore payload  
- retry on next poll  

### 4. **Desync Recovery**
If RT1064 returns an unexpected header:

- Flush SPI RX FIFO  
- Perform a dummy READ_HEADER  
- Re-poll EN until stable  
- Never rely on stale LEN  

### 5. **Zero-Length Descriptor**
LEN=0 & TYPE=1 usually indicates:
- device removal  
- EN mapping change  

---

## S70 Task Loop

```c
void DeviceTask(uint8_t EN)
{
    while (true)
    {
        uint8_t header = SpiReadHeader(EN);
        bool dirty = header & 0x01;
        uint8_t len = header >> 2;

        if (dirty && len > 0)
        {
            uint8_t buf[66];
            SpiReadBlock(EN, buf, 1 + len + 2);

            if (CheckCrc(buf))
            {
                if ((buf[0] & 0x02) == 0)
                    ProcessData(EN, &buf[1], len);
                else
                    ProcessDescriptor(EN, &buf[1], len);
            }
        }

        vTaskDelay(pdMS_TO_TICKS(1));
    }
}
```

---

## Performance Summary

- SPI clock: **5 MHz**
- Header read: **~3 µs**
- 63-byte block read: **~105 µs**
- Sweep of EN0–EN5: **< 0.6 ms**

---

