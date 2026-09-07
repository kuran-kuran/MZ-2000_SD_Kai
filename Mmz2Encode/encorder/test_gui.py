import tempfile
import unittest
from pathlib import Path

from mz2200_gui import (
    EMM_VOLUME_SIZE,
    MACHINE_PROFILES,
    concatenate_files,
    create_emm_volumes,
    emm_concat_path,
    find_png_files,
    normalize_movie_filename,
    remove_generated_parts,
)


class GuiHelperTests(unittest.TestCase):
    def test_machine_profiles_fix_tile_size_and_vram_base(self):
        self.assertEqual(
            MACHINE_PROFILES["mz2200"],
            {"mode32x4": False, "mode8x8": False, "vram_base": 0xC000},
        )
        self.assertEqual(
            MACHINE_PROFILES["mz80b"],
            {"mode32x4": False, "mode8x8": True, "vram_base": 0xE000},
        )

    def test_remove_generated_parts_only_removes_given_files(self):
        with tempfile.TemporaryDirectory() as directory:
            folder = Path(directory)
            parts = [folder / "0001-01.mzt", folder / "0002-01.mzt"]
            combined = folder / "MOVIE.MZT"
            for path in [*parts, combined]:
                path.write_bytes(b"data")
            self.assertEqual(remove_generated_parts(parts), 2)
            self.assertTrue(combined.exists())
            self.assertTrue(all(not path.exists() for path in parts))

    def test_emm_volumes_keep_mzt_files_whole_and_pad_every_volume(self):
        with tempfile.TemporaryDirectory() as directory:
            folder = Path(directory)
            sources = []
            for number, data in enumerate((b"A" * 200_000, b"B" * 150_000, b"C" * 10)):
                path = folder / f"source-{number}.mzt"
                path.write_bytes(data)
                sources.append(path)

            volumes = create_emm_volumes(sources, folder, "MOVIE.MZT")

            self.assertEqual([path.name for path, _ in volumes], ["MOVIE_01.MZT", "MOVIE_02.MZT"])
            self.assertEqual([used for _, used in volumes], [200_000, 150_010])
            first = volumes[0][0].read_bytes()
            second = volumes[1][0].read_bytes()
            self.assertEqual(len(first), EMM_VOLUME_SIZE)
            self.assertEqual(len(second), EMM_VOLUME_SIZE)
            self.assertEqual(first[:200_000], b"A" * 200_000)
            self.assertEqual(second[:150_010], b"B" * 150_000 + b"C" * 10)
            self.assertEqual(first[200_000:], bytes(EMM_VOLUME_SIZE - 200_000))
            self.assertEqual(second[150_010:], bytes(EMM_VOLUME_SIZE - 150_010))

            combined = emm_concat_path(folder, "MOVIE.MZT")
            concatenate_files([path for path, _ in volumes], combined)
            self.assertEqual(combined.name, "MOVIE_CONCAT.MZT")
            self.assertEqual(combined.read_bytes(), first + second)
            self.assertEqual(combined.stat().st_size, EMM_VOLUME_SIZE * 2)

    def test_movie_filename_adds_default_extension(self):
        self.assertEqual(normalize_movie_filename(" DEMO "), "DEMO.MZT")
        self.assertEqual(normalize_movie_filename("DEMO.BIN"), "DEMO.BIN")

    def test_movie_filename_rejects_paths(self):
        for name in ("", "sub/MOVIE.MZT", "bad?.mzt"):
            with self.subTest(name=name), self.assertRaises(ValueError):
                normalize_movie_filename(name)

    def test_binary_concatenation_keeps_source_order(self):
        with tempfile.TemporaryDirectory() as directory:
            folder = Path(directory)
            first, second = folder / "a.mzt", folder / "b.mzt"
            first.write_bytes(b"header-a-data-a")
            second.write_bytes(b"header-b-data-b")
            destination = folder / "MOVIE.MZT"
            size = concatenate_files([first, second], destination)
            self.assertEqual(destination.read_bytes(), first.read_bytes() + second.read_bytes())
            self.assertEqual(size, destination.stat().st_size)

    def test_png_files_are_naturally_sorted_and_case_insensitive(self):
        with tempfile.TemporaryDirectory() as directory:
            folder = Path(directory)
            for name in ("10.png", "2.PNG", "001.png", "memo.txt"):
                (folder / name).write_bytes(b"")
            self.assertEqual(
                [path.name for path in find_png_files(folder)],
                ["001.png", "2.PNG", "10.png"],
            )


if __name__ == "__main__":
    unittest.main()
