"""LZE compressor compatible with the supplied GORRY/Z80 decoder format."""

from __future__ import annotations

from collections import defaultdict, deque

WINDOW = 8192
MAX_MATCH = 256


class LzeError(ValueError):
    pass


class _TokenWriter:
    def __init__(self) -> None:
        self.output = bytearray()
        self.flags = 0
        self.flag_count = 0
        self.code = bytearray((0,))

    def put(self, bits: int, bit_count: int, payload: bytes) -> None:
        self.flags = (self.flags << bit_count) | bits
        self.flag_count += bit_count
        # This deliberately matches lze.c: a new flag byte is inserted before
        # the token whose control bits cross the current 8-bit boundary.
        if self.flag_count > 8:
            self.flag_count -= 8
            self.code[0] = (self.flags >> self.flag_count) & 0xFF
            self.output.extend(self.code)
            self.code = bytearray((0,))
            self.flags &= 0xFF >> (8 - self.flag_count)
        self.code.extend(payload)

    def finish(self) -> bytes:
        self.put(0b01, 2, b"\x00\x00\x00")
        if self.flag_count:
            self.code[0] = (self.flags << (8 - self.flag_count)) & 0xFF
        if len(self.code) > 1:
            self.output.extend(self.code)
        return bytes(self.output)


def encode(data: bytes) -> bytes:
    """Return a 4-byte big-endian size followed by an LZE stream."""
    size = len(data)
    result = bytearray(size.to_bytes(4, "big"))
    if not data:
        return bytes(result)

    # BZCOMPATIBLE mode stores the first byte without a control flag.
    result.append(data[0])
    writer = _TokenWriter()
    positions: dict[bytes, deque[int]] = defaultdict(deque)

    def add_position(position: int) -> None:
        if position + 1 >= size:
            return
        key = data[position : position + 2]
        queue = positions[key]
        queue.append(position)
        minimum = position - WINDOW
        while queue and queue[0] < minimum:
            queue.popleft()

    add_position(0)
    cursor = 1
    while cursor < size:
        best_length = 0
        best_distance = 0
        if cursor + 1 < size:
            key = data[cursor : cursor + 2]
            candidates = positions.get(key, ())
            maximum = min(MAX_MATCH, size - cursor)
            for candidate in reversed(candidates):
                distance = cursor - candidate
                if distance > WINDOW:
                    break
                length = 2
                while (
                    length < maximum
                    and data[cursor + length] == data[cursor + length - distance]
                ):
                    length += 1
                if length > best_length:
                    best_length, best_distance = length, distance
                    if length == maximum:
                        break

        if best_length < 2:
            token_length = 1
            writer.put(0b1, 1, data[cursor : cursor + 1])
        elif best_length < 6 and best_distance <= 256:
            token_length = best_length
            writer.put(
                best_length - 2,
                4,
                bytes(((256 - best_distance) & 0xFF,)),
            )
        elif best_length > 2:
            token_length = best_length
            position_code = 8192 - best_distance
            if best_length > 9:
                payload = bytes(
                    ((position_code >> 5) & 0xFF, (position_code << 3) & 0xFF,
                     best_length - 1)
                )
            else:
                payload = bytes(
                    ((position_code >> 5) & 0xFF,
                     ((position_code << 3) | (best_length - 2)) & 0xFF)
                )
            writer.put(0b01, 2, payload)
        else:
            token_length = 1
            writer.put(0b1, 1, data[cursor : cursor + 1])

        for position in range(cursor, cursor + token_length):
            add_position(position)
        cursor += token_length

    result.extend(writer.finish())
    return bytes(result)


def decode(encoded: bytes) -> bytes:
    """Reference decoder used to verify streams before giving them to Z80."""
    if len(encoded) < 4:
        raise LzeError("LZE header is incomplete")
    expected_size = int.from_bytes(encoded[:4], "big")
    if expected_size == 0:
        return b""
    if len(encoded) < 5:
        raise LzeError("LZE first byte is missing")

    cursor = 5
    output = bytearray((encoded[4],))
    flags = 0
    flag_count = 0

    def get_byte() -> int:
        nonlocal cursor
        if cursor >= len(encoded):
            raise LzeError("LZE stream ended unexpectedly")
        value = encoded[cursor]
        cursor += 1
        return value

    def get_bit() -> int:
        nonlocal flags, flag_count
        if flag_count == 0:
            flags = get_byte()
            flag_count = 8
        bit = 1 if flags & 0x80 else 0
        flags = (flags << 1) & 0xFF
        flag_count -= 1
        return bit

    while True:
        if get_bit():
            output.append(get_byte())
            continue
        if get_bit():
            high, low = get_byte(), get_byte()
            length_code = low & 7
            position_code = ((high << 8) | low) >> 3
            distance = 8192 - position_code
            if length_code == 0:
                extra = get_byte()
                if extra == 0:
                    break
                length = extra + 1
            else:
                length = length_code + 2
        else:
            length = (get_bit() << 1) | get_bit()
            length += 2
            offset = get_byte()
            distance = 256 - offset if offset else 256

        if distance <= 0 or distance > len(output):
            raise LzeError(f"invalid LZE distance: {distance}")
        for _ in range(length):
            output.append(output[-distance])

    if len(output) != expected_size:
        raise LzeError(
            f"expanded size mismatch: expected {expected_size}, got {len(output)}"
        )
    return bytes(output)
