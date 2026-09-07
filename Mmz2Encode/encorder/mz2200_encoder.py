#!/usr/bin/env python3
"""Convert one PNG, or the difference between two PNGs, for MZ video VRAM."""

from __future__ import annotations

import argparse
import re
import struct
import sys
from dataclasses import dataclass
from pathlib import Path
from typing import Sequence

import lze

WIDTH, HEIGHT = 640, 200
ROW_BYTES = WIDTH // 8
VRAM_BASE = 0xC000
TILE_WIDTH, TILE_HEIGHT = 16, 8
TILE_ROW_BYTES = TILE_WIDTH // 8
TILE_BYTES = TILE_ROW_BYTES * TILE_HEIGHT
TILES_X, TILES_Y = WIDTH // TILE_WIDTH, HEIGHT // TILE_HEIGHT
FAST_TILE_WIDTH, FAST_TILE_HEIGHT = 32, 4
FAST_ADDRESS_BIAS = 4
MZ80B_WIDTH, MZ80B_HEIGHT = 320, 200
MZ80B_TILE_WIDTH, MZ80B_TILE_HEIGHT = 8, 8
MZ80B_VRAM_BASE = 0xE000
MZ80B_RAW_ADDRESS = 0xA800
MZ80B_LZE_ADDRESS = 0xCF00
CHUNK_SIZE = 8192
THRESHOLD = 128
CMD_CONTINUE, CMD_END_FRAME = 0x0A, 0x0B
MZT_HEADER_SIZE = 128
MZT_MODE = 1
MZT_READ_ADDRESS = 0x9800
MZT_RUN_ADDRESS = 0x00B1
THRESHOLD_TABLE = bytes(0 if value < THRESHOLD else 255 for value in range(256))
BIT_REVERSE_TABLE = bytes(
    int(f"{value:08b}"[::-1], 2) for value in range(256)
)


class EncodeError(Exception):
    pass


@dataclass(frozen=True)
class Frame:
    """MZ-order B, R, G planes with dimensions stored alongside the data."""

    planes: tuple[bytes, bytes, bytes]
    width: int = WIDTH
    height: int = HEIGHT


def load_png(
    path: Path,
    mono: bool = False,
    mode8x8: bool = False,
    vram_base: int | None = None,
) -> Frame:
    try:
        from PIL import Image, ImageChops
    except ImportError as exc:
        raise EncodeError(
            "PNGの読み込みにはPillowが必要です: python -m pip install Pillow"
        ) from exc

    if path.suffix.lower() != ".png":
        raise EncodeError(f"{path}: PNGファイルを指定してください")
    try:
        with Image.open(path) as source:
            rgba = source.convert("RGBA")
    except OSError as exc:
        raise EncodeError(f"{path}: PNGを読み込めません: {exc}") from exc
    if vram_base is None:
        vram_base = MZ80B_VRAM_BASE if mode8x8 else VRAM_BASE
    width = MZ80B_WIDTH if vram_base == MZ80B_VRAM_BASE else WIDTH
    height = MZ80B_HEIGHT if vram_base == MZ80B_VRAM_BASE else HEIGHT
    if rgba.size != (width, height):
        raise EncodeError(
            f"{path}: 画像サイズは{width}x{height}必須です"
            f"（実際は{rgba.width}x{rgba.height}）"
        )

    # Hidden RGB values under alpha=0 must not create pixels or differences.
    black = Image.new("RGBA", rgba.size, (0, 0, 0, 255))
    rgb = Image.alpha_composite(black, rgba).convert("RGB")
    row_bytes = width // 8
    red, green, blue = rgb.split()

    def pack_plane(band: object) -> bytes:
        # Pillow packs mode 1 with the left pixel in bit 7. MZ data uses bit 0.
        binary = band.point(THRESHOLD_TABLE).convert("1")  # type: ignore[attr-defined]
        packed = binary.tobytes()
        if len(packed) != row_bytes * height:
            raise EncodeError("画像の1ビット変換サイズが不正です")
        return packed.translate(BIT_REVERSE_TABLE)

    if mono:
        visible = ImageChops.lighter(ImageChops.lighter(red, green), blue)
        zero = bytes(row_bytes * height)
        planes = (pack_plane(visible), zero, zero)
    else:
        planes = (pack_plane(blue), pack_plane(red), pack_plane(green))
    return Frame(planes, width, height)


def tile_data(
    plane: bytes,
    tile_x: int,
    tile_y: int,
    tile_width: int = TILE_WIDTH,
    tile_height: int = TILE_HEIGHT,
    row_bytes: int = ROW_BYTES,
) -> bytes:
    """Return row-major bytes for one 16-byte tile."""
    tile_row_bytes = tile_width // 8
    start_y = tile_y * tile_height
    start_x = tile_x * tile_row_bytes
    return b"".join(
        plane[(start_y + row) * row_bytes + start_x:
              (start_y + row) * row_bytes + start_x + tile_row_bytes]
        for row in range(tile_height)
    )


def _tile_planes(
    frame: Frame,
    tile_x: int,
    tile_y: int,
    tile_width: int,
    tile_height: int,
) -> tuple[bytes, bytes, bytes]:
    values = [
        tile_data(
            plane, tile_x, tile_y, tile_width, tile_height, frame.width // 8
        )
        for plane in frame.planes
    ]
    return values[0], values[1], values[2]


def _change_mask(
    current: tuple[bytes, bytes, bytes],
    previous: tuple[bytes, bytes, bytes] | None,
    mono: bool,
) -> int:
    if previous is None:
        return 0x01 if mono else 0x07
    plane_count = 1 if mono else 3
    return sum(
        1 << plane
        for plane in range(plane_count)
        if current[plane] != previous[plane]
    )


def make_records(
    frame: Frame,
    previous: Frame | None = None,
    mono: bool = False,
    mode32x4: bool = False,
    mode8x8: bool = False,
    vram_base: int | None = None,
) -> list[bytes]:
    """Build row-major records, combining adjacent tiles with identical masks."""
    records: list[bytes] = []
    if mode32x4 and mode8x8:
        raise EncodeError("32x4と8x8は同時に指定できません")
    if mode8x8:
        tile_width, tile_height = MZ80B_TILE_WIDTH, MZ80B_TILE_HEIGHT
        mono = True
    else:
        tile_width = FAST_TILE_WIDTH if mode32x4 else TILE_WIDTH
        tile_height = FAST_TILE_HEIGHT if mode32x4 else TILE_HEIGHT
    tiles_x = frame.width // tile_width
    tiles_y = frame.height // tile_height
    row_bytes = frame.width // 8
    tile_row_bytes = tile_width // 8
    address_bias = FAST_ADDRESS_BIAS if mode32x4 else 0
    if vram_base is None:
        vram_base = MZ80B_VRAM_BASE if mode8x8 else VRAM_BASE
    if vram_base not in (0xC000, 0xE000):
        raise EncodeError("VRAMベースアドレスはC000hまたはE000hです")
    if vram_base + (frame.width // 8) * frame.height > 0x10000:
        raise EncodeError(
            f"{frame.width}x{frame.height}はVRAMベース{vram_base:04X}hに収まりません"
        )
    for tile_y in range(tiles_y):
        def tiles_for(source: Frame) -> list[tuple[bytes, bytes, bytes]]:
            if mono:
                return [
                    (
                        tile_data(
                            source.planes[0], tile_x, tile_y, tile_width,
                            tile_height, row_bytes,
                        ),
                        b"",
                        b"",
                    )
                    for tile_x in range(tiles_x)
                ]
            return [
                _tile_planes(source, tile_x, tile_y, tile_width, tile_height)
                for tile_x in range(tiles_x)
            ]

        current_tiles = tiles_for(frame)
        previous_tiles = tiles_for(previous) if previous else None
        masks = [
            _change_mask(
                current_tiles[tile_x],
                previous_tiles[tile_x] if previous_tiles else None,
                mono,
            )
            for tile_x in range(tiles_x)
        ]
        tile_x = 0
        while tile_x < tiles_x:
            mask = masks[tile_x]
            if mask == 0:
                tile_x += 1
                continue

            next_x = tile_x + 1
            while next_x < tiles_x and masks[next_x] == mask:
                next_x += 1
            run = current_tiles[tile_x:next_x]

            address = (
                vram_base
                + tile_y * tile_height * row_bytes
                + tile_x * tile_row_bytes
                + address_bias
            )
            payload = bytearray()
            # Finish one tile's B/R/G data before moving to the tile at its right.
            for tile in run:
                for plane in range(3):
                    if mask & (1 << plane):
                        payload.extend(tile[plane])
            records.append(struct.pack("<BHB", mask, address, len(run)) + payload)
            tile_x = next_x
    return records


def pack_chunks(records: Sequence[bytes]) -> list[bytes]:
    """Split whole records into unpadded physical files of at most 8192 bytes."""
    chunks = [bytearray()]
    for record in records:
        if len(record) + 1 > CHUNK_SIZE:
            raise EncodeError("1レコードが8KBに収まりません")
        if len(chunks[-1]) + len(record) + 1 > CHUNK_SIZE:
            chunks[-1].append(CMD_CONTINUE)
            chunks.append(bytearray())
        chunks[-1].extend(record)
    chunks[-1].append(CMD_END_FRAME)
    return [bytes(chunk) for chunk in chunks]


def pack_lze_chunks(records: Sequence[bytes]) -> list[bytes]:
    """Pack whole records so every compressed MZT body is at most 8192 bytes."""
    # First split solely by expanded size. The old implementation compressed
    # every growing candidate twice, which made a long image sequence very slow.
    groups: list[list[bytes]] = [[]]
    for record in records:
        if len(record) + 1 > CHUNK_SIZE:
            raise EncodeError("1レコードのLZE圧縮結果が8KBに収まりません")
        if sum(map(len, groups[-1])) + len(record) + 1 > CHUNK_SIZE:
            groups.append([])
        groups[-1].append(record)

    # Incompressible input can grow slightly. Split only those rare groups,
    # then compress each final group once for output.
    checked: list[list[bytes]] = []

    def add_checked(group: list[bytes]) -> None:
        body = b"".join(group)
        pause_size = len(lze.encode(body + bytes((CMD_CONTINUE,))))
        end_size = len(lze.encode(body + bytes((CMD_END_FRAME,))))
        if max(pause_size, end_size) <= CHUNK_SIZE:
            checked.append(group)
            return
        if len(group) <= 1:
            raise EncodeError("1レコードのLZE圧縮結果が8KBに収まりません")
        middle = len(group) // 2
        add_checked(group[:middle])
        add_checked(group[middle:])

    for group in groups:
        add_checked(group)

    chunks = []
    for index, group in enumerate(checked):
        terminator = CMD_END_FRAME if index == len(checked) - 1 else CMD_CONTINUE
        chunks.append(lze.encode(b"".join(group) + bytes((terminator,))))
    return chunks


def make_mzt(payload: bytes, read_address: int = MZT_READ_ADDRESS) -> bytes:
    """Prepend the fixed 128-byte MZT header to one encoded data chunk."""
    if len(payload) > CHUNK_SIZE:
        raise EncodeError("MZTデータ本体が8KBを超えています")
    header = bytearray(MZT_HEADER_SIZE)
    header[0] = MZT_MODE
    header[1:18] = bytes([0x0D]) * 17
    struct.pack_into(
        "<HHH", header, 18, len(payload), read_address, MZT_RUN_ADDRESS
    )
    return bytes(header) + payload


def _remove_old_outputs(output_dir: Path, stem: str) -> None:
    pattern = re.compile(rf"^{re.escape(stem)}[-_][0-9]+\.(?:bin|mzt)$", re.IGNORECASE)
    for path in output_dir.iterdir():
        if path.is_file() and pattern.match(path.name):
            path.unlink()


def convert_frame(
    target_path: Path,
    target: Frame,
    output_dir: Path,
    previous: Frame | None = None,
    mono: bool = False,
    mode32x4: bool = False,
    use_lze: bool = False,
    mode8x8: bool = False,
    vram_base: int | None = None,
) -> list[Path]:
    """Encode already loaded frames, avoiding repeated PNG decoding in batches."""
    target_path = target_path.resolve()
    output_dir = output_dir.resolve()
    output_dir.mkdir(parents=True, exist_ok=True)
    mono = mono or mode8x8
    if vram_base is None:
        vram_base = MZ80B_VRAM_BASE if mode8x8 else VRAM_BASE

    records = make_records(
        target, previous, mono, mode32x4, mode8x8, vram_base
    )
    chunks = pack_lze_chunks(records) if use_lze else pack_chunks(records)
    if vram_base == MZ80B_VRAM_BASE:
        read_address = MZ80B_LZE_ADDRESS if use_lze else MZ80B_RAW_ADDRESS
    else:
        read_address = 0xC000 if use_lze else MZT_READ_ADDRESS

    _remove_old_outputs(output_dir, target_path.stem)
    output_paths = []
    for number, chunk in enumerate(chunks, start=1):
        path = output_dir / f"{target_path.stem}-{number:02d}.mzt"
        path.write_bytes(make_mzt(chunk, read_address))
        output_paths.append(path)
    return output_paths


def convert(
    target_path: Path,
    output_dir: Path | None = None,
    previous_path: Path | None = None,
    mono: bool = False,
    mode32x4: bool = False,
    use_lze: bool = False,
    mode8x8: bool = False,
    vram_base: int | None = None,
) -> list[Path]:
    target_path = target_path.resolve()
    previous_path = previous_path.resolve() if previous_path else None
    output_dir = (output_dir or target_path.parent).resolve()

    mono = mono or mode8x8
    if vram_base is None:
        vram_base = MZ80B_VRAM_BASE if mode8x8 else VRAM_BASE
    target = load_png(target_path, mono, mode8x8, vram_base)
    previous = (
        load_png(previous_path, mono, mode8x8, vram_base)
        if previous_path else None
    )
    return convert_frame(
        target_path, target, output_dir, previous, mono, mode32x4, use_lze,
        mode8x8, vram_base,
    )


def main(argv: Sequence[str] | None = None) -> int:
    parser = argparse.ArgumentParser(
        description="PNGをMZ-2200/MZ-80B用タイル形式へ変換します"
    )
    parser.add_argument(
        "images", nargs="+", type=Path, metavar="PNG",
        help="1枚なら全画面、2枚なら 比較元.png 出力対象.png",
    )
    parser.add_argument(
        "-8x8", dest="mode8x8", action="store_true",
        help="MZ-80B用320x200・モノクロ8x8タイル形式で変換",
    )
    parser.add_argument("-o", type=Path, metavar="FOLDER", help="出力フォルダー")
    parser.add_argument("-mono", action="store_true", help="モノクロ1プレーンで変換")
    parser.add_argument(
        "-32x4", dest="mode32x4", action="store_true",
        help="PUSH描画用32x4タイル形式で変換",
    )
    parser.add_argument("-lze", action="store_true", help="MMZデータ本体をLZE圧縮")
    parser.add_argument(
        "-base", choices=("C000", "E000"), metavar="{C000,E000}",
        help="VRAMベースアドレス（省略時は機種プロファイルに従う）",
    )
    args = parser.parse_args(argv)
    if len(args.images) not in (1, 2):
        parser.error("PNGは1枚または2枚指定してください")
    if args.mode32x4 and args.mode8x8:
        parser.error("-32x4と-8x8は同時に指定できません")

    previous = args.images[0] if len(args.images) == 2 else None
    target = args.images[-1]
    try:
        outputs = convert(
            target, args.o, previous, args.mono, args.mode32x4, args.lze,
            args.mode8x8, int(args.base, 16) if args.base else None,
        )
    except (OSError, EncodeError) as exc:
        print(f"error: {exc}", file=sys.stderr)
        return 1

    total = sum(path.stat().st_size for path in outputs)
    for path in outputs:
        print(f"{path} ({path.stat().st_size} bytes)")
    print(f"完了: {len(outputs)} file(s), {total} bytes")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
