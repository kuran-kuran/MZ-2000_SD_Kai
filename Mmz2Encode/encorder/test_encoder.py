import struct
import tempfile
import unittest
from pathlib import Path

import mz2200_encoder as enc
import lze


def frame_with_tiles(spec):
    planes = [bytearray(enc.ROW_BYTES * enc.HEIGHT) for _ in range(3)]
    for tile_x, plane, value in spec:
        start_x = tile_x * enc.TILE_ROW_BYTES
        for row in range(enc.TILE_HEIGHT):
            pos = row * enc.ROW_BYTES + start_x
            planes[plane][pos:pos + 2] = bytes((value, value))
    return enc.Frame(tuple(bytes(plane) for plane in planes))


class EncoderTests(unittest.TestCase):
    def test_lze_round_trip(self):
        samples = [
            b"A",
            bytes(range(256)),
            b"ABCD" * 2048,
            bytes((index * 73 + index // 7) & 0xFF for index in range(8192)),
        ]
        for sample in samples:
            packed = lze.encode(sample)
            self.assertEqual(int.from_bytes(packed[:4], "big"), len(sample))
            self.assertEqual(lze.decode(packed), sample)

    def test_lze_chunk_has_compressed_body_and_c000_load_address(self):
        records = [b"A" * 1000, b"B" * 1000]
        chunks = enc.pack_lze_chunks(records)
        self.assertEqual(len(chunks), 1)
        self.assertEqual(lze.decode(chunks[0]), b"".join(records) + b"\x0b")
        mzt = enc.make_mzt(chunks[0], 0xC000)
        self.assertEqual(struct.unpack_from("<H", mzt, 20)[0], 0xC000)
        self.assertLessEqual(len(mzt) - 128, 8192)

    def test_lze_chunks_also_limit_expanded_size(self):
        records = [bytes(range(256)) * 20, bytes(range(256)) * 16]
        chunks = enc.pack_lze_chunks(records)
        self.assertEqual(len(chunks), 2)
        self.assertTrue(all(len(lze.decode(chunk)) <= 8192 for chunk in chunks))

    def test_png_alpha_is_composited_on_black_and_left_is_bit_zero(self):
        from PIL import Image

        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "alpha.png"
            image = Image.new("RGBA", (enc.WIDTH, enc.HEIGHT), (0, 0, 0, 0))
            image.putpixel((0, 0), (255, 0, 0, 0))  # hidden red must become black
            image.putpixel((1, 0), (0, 0, 255, 255))
            image.save(path)
            frame = enc.load_png(path)
            mono = enc.load_png(path, mono=True)

        self.assertEqual(frame.planes[0][0], 0x02)
        self.assertEqual(frame.planes[1][0], 0x00)
        self.assertEqual(frame.planes[2][0], 0x00)
        self.assertEqual(mono.planes[0][0], 0x02)

    def test_full_frame_always_contains_all_tiles_and_planes(self):
        black = enc.Frame((bytes(16000), bytes(16000), bytes(16000)))
        records = enc.make_records(black)
        self.assertEqual(len(records), 25)
        self.assertEqual(struct.unpack_from("<BHB", records[0]), (7, 0xC000, 40))
        self.assertEqual(len(records[0]), 4 + 40 * 16 * 3)
        self.assertEqual(struct.unpack_from("<H", records[1], 1)[0], 0xC280)

    def test_32x4_full_frame_uses_twenty_tiles_and_address_plus_four(self):
        black = enc.Frame((bytes(16000), bytes(16000), bytes(16000)))
        records = enc.make_records(black, mode32x4=True)
        self.assertEqual(len(records), 50)
        self.assertEqual(struct.unpack_from("<BHB", records[0]), (7, 0xC004, 20))
        self.assertEqual(len(records[0]), 4 + 20 * 16 * 3)
        self.assertEqual(struct.unpack_from("<H", records[1], 1)[0], 0xC144)

    def test_8x8_mz80b_full_frame_is_320x200_mono(self):
        black = enc.Frame((bytes(8000), bytes(8000), bytes(8000)), 320, 200)
        records = enc.make_records(black, mode8x8=True)
        self.assertEqual(len(records), 25)
        self.assertEqual(struct.unpack_from("<BHB", records[0]), (1, 0xE000, 40))
        self.assertEqual(len(records[0]), 4 + 40 * 8)
        self.assertEqual(struct.unpack_from("<H", records[1], 1)[0], 0xE140)

    def test_vram_base_can_be_selected(self):
        black = enc.Frame((bytes(8000), bytes(8000), bytes(8000)), 320, 200)
        record = enc.make_records(black, mode8x8=True, vram_base=0xC000)[0]
        self.assertEqual(struct.unpack_from("<H", record, 1)[0], 0xC000)

    def test_e000_base_rejects_640x200_profile(self):
        black = enc.Frame((bytes(16000), bytes(16000), bytes(16000)))
        with self.assertRaises(enc.EncodeError):
            enc.make_records(black, vram_base=0xE000)

    def test_32x4_e000_uses_320x200_geometry(self):
        black = enc.Frame((bytes(8000), bytes(8000), bytes(8000)), 320, 200)
        records = enc.make_records(
            black, mono=True, mode32x4=True, vram_base=0xE000
        )
        self.assertEqual(len(records), 50)
        self.assertEqual(struct.unpack_from("<BHB", records[0]), (1, 0xE004, 10))
        self.assertEqual(len(records[0]), 4 + 10 * 16)
        self.assertEqual(struct.unpack_from("<H", records[1], 1)[0], 0xE0A4)

    def test_e000_32x4_conversion_uses_mz80b_load_address(self):
        from PIL import Image

        with tempfile.TemporaryDirectory() as directory:
            folder = Path(directory)
            source = folder / "fast80b.png"
            Image.new("RGB", (320, 200), "black").save(source)
            mzt = enc.convert(
                source, folder / "out", mono=True, mode32x4=True,
                vram_base=0xE000,
            )[0].read_bytes()
        self.assertEqual(struct.unpack_from("<H", mzt, 20)[0], 0xA800)
        self.assertEqual(struct.unpack_from("<BHB", mzt, 128), (1, 0xE004, 10))

    def test_8x8_mzt_uses_mz80b_load_addresses(self):
        from PIL import Image

        with tempfile.TemporaryDirectory() as directory:
            folder = Path(directory)
            source = folder / "mz80b.png"
            Image.new("RGB", (320, 200), "black").save(source)
            raw = enc.convert(source, folder / "raw", mode8x8=True)[0].read_bytes()
            packed = enc.convert(
                source, folder / "packed", use_lze=True, mode8x8=True
            )[0].read_bytes()
        self.assertEqual(struct.unpack_from("<H", raw, 20)[0], 0xA800)
        self.assertEqual(struct.unpack_from("<H", packed, 20)[0], 0xCF00)

    def test_8x8_png_requires_320x200_and_uses_left_as_bit_zero(self):
        from PIL import Image

        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "mz80b.png"
            image = Image.new("RGB", (320, 200), "black")
            image.putpixel((0, 0), (255, 255, 255))
            image.save(path)
            frame = enc.load_png(path, mode8x8=True)
        self.assertEqual((frame.width, frame.height), (320, 200))
        self.assertEqual(frame.planes[0][0], 0x01)

    def test_tile_major_plane_order_and_same_mask_run(self):
        old = frame_with_tiles([])
        new = frame_with_tiles([
            (0, 0, 0x01), (0, 1, 0x02),
            (1, 0, 0x04), (1, 1, 0x08),
        ])
        records = enc.make_records(new, old)
        self.assertEqual(len(records), 1)
        self.assertEqual(struct.unpack_from("<BHB", records[0]), (3, 0xC000, 2))
        payload = records[0][4:]
        self.assertEqual(payload[0:16], bytes([0x01, 0x01]) * 8)
        self.assertEqual(payload[16:32], bytes([0x02, 0x02]) * 8)
        self.assertEqual(payload[32:48], bytes([0x04, 0x04]) * 8)
        self.assertEqual(payload[48:64], bytes([0x08, 0x08]) * 8)

    def test_different_masks_create_separate_records(self):
        old = frame_with_tiles([])
        new = frame_with_tiles([(0, 0, 1), (1, 0, 1), (2, 1, 1)])
        records = enc.make_records(new, old)
        self.assertEqual([struct.unpack_from("<BHB", r) for r in records], [
            (1, 0xC000, 2), (2, 0xC004, 1)
        ])

    def test_unchanged_frame_is_one_byte(self):
        black = enc.Frame((bytes(16000), bytes(16000), bytes(16000)))
        self.assertEqual(enc.pack_chunks(enc.make_records(black, black)), [b"\x0b"])

    def test_chunks_are_unpadded_and_terminated(self):
        chunks = enc.pack_chunks([b"x" * 5000, b"y" * 4000])
        self.assertEqual([len(chunk) for chunk in chunks], [5001, 4001])
        self.assertEqual(chunks[0][-1], 0x0A)
        self.assertEqual(chunks[1][-1], 0x0B)

    def test_old_numbered_outputs_are_removed(self):
        with tempfile.TemporaryDirectory() as directory:
            folder = Path(directory)
            (folder / "picture_01.bin").write_bytes(b"old")
            (folder / "picture_12.bin").write_bytes(b"old")
            (folder / "picture_13.mzt").write_bytes(b"old")
            (folder / "picture-14.mzt").write_bytes(b"old")
            (folder / "picture_other.bin").write_bytes(b"keep")
            enc._remove_old_outputs(folder, "picture")
            self.assertFalse((folder / "picture_01.bin").exists())
            self.assertFalse((folder / "picture_12.bin").exists())
            self.assertFalse((folder / "picture_13.mzt").exists())
            self.assertFalse((folder / "picture-14.mzt").exists())
            self.assertTrue((folder / "picture_other.bin").exists())

    def test_mzt_header_fields_and_padding(self):
        payload = b"test\x0b"
        mzt = enc.make_mzt(payload)
        self.assertEqual(len(mzt), 128 + len(payload))
        self.assertEqual(mzt[0], 1)
        self.assertEqual(mzt[1:18], bytes([0x0D]) * 17)
        self.assertEqual(
            struct.unpack_from("<HHH", mzt, 18),
            (len(payload), 0x9800, 0x00B1),
        )
        self.assertEqual(mzt[24:128], bytes(104))
        self.assertEqual(mzt[128:], payload)

    def test_real_full_png_conversion_produces_seven_mzt_files(self):
        from PIL import Image

        with tempfile.TemporaryDirectory() as directory:
            folder = Path(directory)
            source = folder / "picture.png"
            output = folder / "out"
            Image.new("RGB", (enc.WIDTH, enc.HEIGHT), "black").save(source)
            paths = enc.convert(source, output)

            self.assertEqual([path.name for path in paths], [
                f"picture-{number:02d}.mzt" for number in range(1, 8)
            ])
            self.assertTrue(all(path.stat().st_size <= 128 + 8192 for path in paths))
            self.assertTrue(all(path.read_bytes()[128:][-1] == 0x0A for path in paths[:-1]))
            self.assertEqual(paths[-1].read_bytes()[-1], 0x0B)
            for path in paths:
                contents = path.read_bytes()
                self.assertEqual(
                    struct.unpack_from("<H", contents, 18)[0], len(contents) - 128
                )


if __name__ == "__main__":
    unittest.main()
