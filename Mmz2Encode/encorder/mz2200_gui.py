#!/usr/bin/env python3
"""Graphical batch encoder for MZ-2200 image sequences."""

from __future__ import annotations

import queue
import re
import threading
import tkinter as tk
from pathlib import Path
from tkinter import filedialog, messagebox, ttk
from typing import Callable

from mz2200_encoder import EncodeError, convert_frame, load_png

EMM_VOLUME_SIZE = 320 * 1024
MACHINE_PROFILES = {
    "mz2200": {
        "mode32x4": False,
        "mode8x8": False,
        "vram_base": 0xC000,
    },
    "mz80b": {
        "mode32x4": False,
        "mode8x8": True,
        "vram_base": 0xE000,
    },
}

try:
    from tkinterdnd2 import DND_FILES, TkinterDnD
except ImportError:  # The GUI remains usable through the Browse button.
    DND_FILES = None
    TkinterDnD = None


def _natural_key(path: Path) -> list[tuple[int, object]]:
    parts = re.split(r"([0-9]+)", path.name.casefold())
    return [(1, int(part)) if part.isdigit() else (0, part) for part in parts]


def find_png_files(folder: Path) -> list[Path]:
    if not folder.is_dir():
        raise ValueError("入力フォルダーが存在しません")
    return sorted(
        (path for path in folder.iterdir() if path.is_file() and path.suffix.lower() == ".png"),
        key=_natural_key,
    )


def concatenate_files(sources: list[Path], destination: Path) -> int:
    """Binary-concatenate complete MZT blocks, preserving every MZT header."""
    total = 0
    with destination.open("wb") as output:
        for source in sources:
            with source.open("rb") as input_file:
                while block := input_file.read(1024 * 1024):
                    output.write(block)
                    total += len(block)
    return total


def create_emm_volumes(
    sources: list[Path], output_folder: Path, base_name: str
) -> list[tuple[Path, int]]:
    """Pack whole MZT files into zero-padded 320 KiB EMM volumes."""
    base = Path(normalize_movie_filename(base_name))
    pattern = re.compile(
        rf"^{re.escape(base.stem)}_[0-9]+{re.escape(base.suffix)}$", re.IGNORECASE
    )
    for old_path in output_folder.iterdir():
        if old_path.is_file() and pattern.match(old_path.name):
            old_path.unlink()

    volumes: list[tuple[Path, int]] = []
    current = bytearray()

    def write_volume() -> None:
        number = len(volumes) + 1
        destination = output_folder / f"{base.stem}_{number:02d}{base.suffix}"
        used = len(current)
        destination.write_bytes(current + bytes(EMM_VOLUME_SIZE - used))
        volumes.append((destination, used))

    for source in sources:
        size = source.stat().st_size
        if size > EMM_VOLUME_SIZE:
            raise EncodeError(f"{source.name} がEMM容量320KBを超えています")
        if current and len(current) + size > EMM_VOLUME_SIZE:
            write_volume()
            current.clear()
        current.extend(source.read_bytes())
    if current:
        write_volume()
    return volumes


def emm_concat_path(output_folder: Path, base_name: str) -> Path:
    base = Path(normalize_movie_filename(base_name))
    return output_folder / f"{base.stem}_CONCAT{base.suffix}"


def remove_generated_parts(paths: list[Path]) -> int:
    """Remove intermediate MZT files after the final CONCAT output is complete."""
    removed = 0
    for path in paths:
        if path.exists():
            path.unlink()
            removed += 1
    return removed


def normalize_movie_filename(value: str) -> str:
    """Validate the user-facing joined filename and add .MZT when omitted."""
    name = value.strip()
    if not name:
        raise ValueError("連結ファイル名を入力してください")
    if Path(name).name != name or name in {".", ".."}:
        raise ValueError("連結ファイル名にフォルダーは指定できません")
    if any(character in name for character in '<>:"/\\|?*'):
        raise ValueError("連結ファイル名に使えない文字が含まれています")
    if not Path(name).suffix:
        name += ".MZT"
    return name


def encode_folder(
    input_folder: Path,
    output_folder: Path,
    *,
    mono: bool,
    mode32x4: bool,
    mode8x8: bool,
    vram_base: int,
    use_lze: bool,
    concatenate_name: str,
    keep_parts: bool,
    cancelled: threading.Event,
    report: Callable[[str, object], None],
) -> tuple[int, int, int]:
    files = find_png_files(input_folder)
    if not files:
        raise EncodeError("入力フォルダーにPNGがありません")
    output_folder.mkdir(parents=True, exist_ok=True)
    total_bytes = 0
    total_parts = 0
    completed = 0
    generated: list[Path] = []
    previous_frame = None
    movie_path = output_folder / normalize_movie_filename(concatenate_name)
    if movie_path.exists():
        movie_path.unlink()
    old_emm_concat = emm_concat_path(output_folder, concatenate_name)
    if old_emm_concat.exists():
        old_emm_concat.unlink()
    for target in files:
        if cancelled.is_set():
            break
        report("current", target.name)
        effective_mono = mono or mode8x8
        current_frame = load_png(target, effective_mono, mode8x8, vram_base)
        outputs = convert_frame(
            target,
            current_frame,
            output_folder,
            previous_frame,
            effective_mono,
            mode32x4,
            use_lze,
            mode8x8,
            vram_base,
        )
        previous_frame = current_frame
        frame_bytes = sum(path.stat().st_size for path in outputs)
        total_bytes += frame_bytes
        total_parts += len(outputs)
        generated.extend(outputs)
        completed += 1
        report("log", f"{target.name}  →  {len(outputs)} file(s), {frame_bytes:,} bytes")
        report("progress", (completed, len(files)))
    if not cancelled.is_set():
        movie_bytes = concatenate_files(generated, movie_path)
        report("log", f"{movie_path.name}  →  {movie_bytes:,} bytes（全ファイル連結）")
        volumes = create_emm_volumes(generated, output_folder, concatenate_name)
        for path, used in volumes:
            report(
                "log",
                f"{path.name}  →  {EMM_VOLUME_SIZE:,} bytes "
                f"（MZT {used:,} bytes + 0埋め {EMM_VOLUME_SIZE - used:,} bytes）",
            )
        combined_path = emm_concat_path(output_folder, concatenate_name)
        combined_bytes = concatenate_files(
            [path for path, _used in volumes], combined_path
        )
        report(
            "log",
            f"{combined_path.name}  →  {combined_bytes:,} bytes（EMM全巻連結）",
        )
        if not keep_parts:
            intermediates = [*generated, *(path for path, _used in volumes)]
            removed = remove_generated_parts(intermediates)
            report(
                "log",
                f"中間MZT {removed}ファイルを削除しました"
                f"（{movie_path.name}と{combined_path.name}を保存）",
            )
    return completed, total_parts, total_bytes


class EncoderGui:
    def __init__(self, root: tk.Tk) -> None:
        self.root = root
        self.root.title("MMZ Encoder")
        self.root.geometry("760x600")
        self.root.minsize(680, 520)
        self.events: queue.Queue[tuple[str, object]] = queue.Queue()
        self.cancelled = threading.Event()
        self.worker: threading.Thread | None = None
        self._auto_output: Path | None = None

        self.input_var = tk.StringVar()
        self.output_var = tk.StringVar()
        self.machine_var = tk.StringVar(value="mz2200")
        self.mono_var = tk.BooleanVar(value=False)
        self.lze_var = tk.BooleanVar(value=True)
        self.keep_parts_var = tk.BooleanVar(value=False)
        self.concatenate_name_var = tk.StringVar(value="MOVIE.MZT")
        self.file_count_var = tk.StringVar(value="PNGフォルダーを指定してください")
        self.current_var = tk.StringVar(value="待機中")
        self.status_var = tk.StringVar(value="")
        self.progress_var = tk.DoubleVar(value=0)

        self._build_widgets()
        self._install_drop_target()
        self.root.after(100, self._poll_events)

    def _build_widgets(self) -> None:
        outer = ttk.Frame(self.root, padding=16)
        outer.pack(fill="both", expand=True)
        outer.columnconfigure(1, weight=1)
        outer.rowconfigure(7, weight=1)

        ttk.Label(outer, text="入力PNGフォルダー").grid(row=0, column=0, sticky="w", pady=5)
        input_entry = ttk.Entry(outer, textvariable=self.input_var)
        input_entry.grid(row=0, column=1, sticky="ew", padx=8)
        ttk.Button(outer, text="参照...", command=self._browse_input).grid(row=0, column=2)

        ttk.Label(outer, text="出力フォルダー").grid(row=1, column=0, sticky="w", pady=5)
        ttk.Entry(outer, textvariable=self.output_var).grid(
            row=1, column=1, sticky="ew", padx=8
        )
        ttk.Button(outer, text="参照...", command=self._browse_output).grid(row=1, column=2)

        options = ttk.LabelFrame(outer, text="エンコード設定", padding=12)
        options.grid(row=2, column=0, columnspan=3, sticky="ew", pady=(12, 8))
        ttk.Label(options, text="対応機種").grid(row=0, column=0, sticky="w")
        ttk.Radiobutton(
            options, text="MZ-2000/2200（16×8）",
            variable=self.machine_var, value="mz2200",
            command=self._machine_changed,
        ).grid(
            row=0, column=1, columnspan=2, sticky="w", padx=(12, 6)
        )
        ttk.Radiobutton(
            options, text="MZ-80B（8×8）",
            variable=self.machine_var, value="mz80b",
            command=self._machine_changed,
        ).grid(row=0, column=3, columnspan=2, sticky="w", padx=6)
        ttk.Checkbutton(options, text="モノクロ", variable=self.mono_var).grid(
            row=0, column=5, padx=18
        )
        ttk.Checkbutton(options, text="LZE圧縮", variable=self.lze_var).grid(
            row=1, column=1, columnspan=5, sticky="w", padx=(12, 6), pady=(10, 0)
        )
        ttk.Checkbutton(
            options,
            text="中間ファイルを残す",
            variable=self.keep_parts_var,
        ).grid(row=2, column=1, columnspan=5, sticky="w", padx=(12, 6), pady=(8, 0))
        ttk.Label(options, text="連結ファイル名").grid(
            row=3, column=0, sticky="w", pady=(10, 0)
        )
        ttk.Entry(options, textvariable=self.concatenate_name_var, width=28).grid(
            row=3, column=1, columnspan=3, sticky="w", padx=(12, 6), pady=(10, 0)
        )

        drop_text = "ここへPNGフォルダーをドロップできます"
        if TkinterDnD is None:
            drop_text += "（ドラッグ＆ドロップ機能は未インストール）"
        self.drop_area = ttk.Label(
            outer, text=drop_text, anchor="center", relief="groove", padding=18
        )
        self.drop_area.grid(row=3, column=0, columnspan=3, sticky="ew", pady=8)
        ttk.Label(outer, textvariable=self.file_count_var).grid(
            row=4, column=0, columnspan=3, sticky="w", pady=(4, 10)
        )

        controls = ttk.Frame(outer)
        controls.grid(row=5, column=0, columnspan=3, sticky="ew")
        self.start_button = ttk.Button(controls, text="エンコード開始", command=self._start)
        self.start_button.pack(side="left")
        self.cancel_button = ttk.Button(
            controls, text="中断", command=self._cancel, state="disabled"
        )
        self.cancel_button.pack(side="left", padx=8)
        ttk.Label(controls, textvariable=self.current_var).pack(side="left", padx=12)

        self.progress = ttk.Progressbar(
            outer, variable=self.progress_var, maximum=100, mode="determinate"
        )
        self.progress.grid(row=6, column=0, columnspan=3, sticky="ew", pady=10)

        log_frame = ttk.LabelFrame(outer, text="処理結果", padding=6)
        log_frame.grid(row=7, column=0, columnspan=3, sticky="nsew")
        log_frame.columnconfigure(0, weight=1)
        log_frame.rowconfigure(0, weight=1)
        self.log = tk.Text(log_frame, height=12, wrap="none", state="disabled")
        self.log.grid(row=0, column=0, sticky="nsew")
        scrollbar = ttk.Scrollbar(log_frame, orient="vertical", command=self.log.yview)
        scrollbar.grid(row=0, column=1, sticky="ns")
        self.log.configure(yscrollcommand=scrollbar.set)
        ttk.Label(outer, textvariable=self.status_var).grid(
            row=8, column=0, columnspan=3, sticky="w", pady=(8, 0)
        )

    def _machine_changed(self) -> None:
        """Set the natural mono default when the user explicitly changes machine."""
        self.mono_var.set(self.machine_var.get() == "mz80b")

    def _install_drop_target(self) -> None:
        if DND_FILES is None:
            return
        self.drop_area.drop_target_register(DND_FILES)
        self.drop_area.dnd_bind("<<Drop>>", self._on_drop)

    def _on_drop(self, event: object) -> None:
        data = getattr(event, "data", "")
        paths = self.root.tk.splitlist(data)
        if paths:
            path = Path(paths[0])
            self._set_input(path.parent if path.is_file() else path)

    def _browse_input(self) -> None:
        selected = filedialog.askdirectory(title="PNGフォルダーを選択")
        if selected:
            self._set_input(Path(selected))

    def _browse_output(self) -> None:
        selected = filedialog.askdirectory(title="出力フォルダーを選択")
        if selected:
            self.output_var.set(selected)
            self._auto_output = None

    def _set_input(self, folder: Path) -> None:
        folder = folder.resolve()
        self.input_var.set(str(folder))
        suggested = folder / "encoded"
        if not self.output_var.get() or self._auto_output is not None:
            self.output_var.set(str(suggested))
            self._auto_output = suggested
        try:
            files = find_png_files(folder)
            first = files[0].name if files else "-"
            last = files[-1].name if files else "-"
            self.file_count_var.set(f"{len(files)}枚  （{first} ～ {last}）")
        except ValueError as exc:
            self.file_count_var.set(str(exc))

    def _append_log(self, text: str) -> None:
        self.log.configure(state="normal")
        self.log.insert("end", text + "\n")
        self.log.see("end")
        self.log.configure(state="disabled")

    def _start(self) -> None:
        if self.worker and self.worker.is_alive():
            return
        input_folder = Path(self.input_var.get())
        output_text = self.output_var.get().strip()
        if not output_text:
            messagebox.showerror("入力エラー", "出力フォルダーを指定してください")
            return
        output_folder = Path(output_text)
        try:
            files = find_png_files(input_folder)
        except ValueError as exc:
            messagebox.showerror("入力エラー", str(exc))
            return
        if not files:
            messagebox.showerror("入力エラー", "PNGファイルがありません")
            return
        try:
            concatenate_name = normalize_movie_filename(self.concatenate_name_var.get())
        except ValueError as exc:
            messagebox.showerror("入力エラー", str(exc))
            return

        self.cancelled.clear()
        self.progress_var.set(0)
        self.status_var.set("エンコード中...")
        self.start_button.configure(state="disabled")
        self.cancel_button.configure(state="normal")
        profile = MACHINE_PROFILES[self.machine_var.get()]
        settings = dict(
            mono=self.mono_var.get(),
            mode32x4=profile["mode32x4"],
            mode8x8=profile["mode8x8"],
            vram_base=profile["vram_base"],
            use_lze=self.lze_var.get(),
            concatenate_name=concatenate_name,
            keep_parts=self.keep_parts_var.get(),
        )

        def report(kind: str, value: object) -> None:
            self.events.put((kind, value))

        def run() -> None:
            try:
                result = encode_folder(
                    input_folder,
                    output_folder,
                    cancelled=self.cancelled,
                    report=report,
                    **settings,
                )
                report("done", result)
            except Exception as exc:  # Show conversion and filesystem errors in GUI.
                report("error", str(exc))

        self.worker = threading.Thread(target=run, daemon=True)
        self.worker.start()

    def _cancel(self) -> None:
        self.cancelled.set()
        self.status_var.set("現在のフレーム終了後に中断します...")

    def _poll_events(self) -> None:
        try:
            while True:
                kind, value = self.events.get_nowait()
                if kind == "current":
                    self.current_var.set(str(value))
                elif kind == "log":
                    self._append_log(str(value))
                elif kind == "progress":
                    completed, total = value  # type: ignore[misc]
                    self.progress_var.set(completed * 100 / total)
                elif kind == "done":
                    completed, parts, total_bytes = value  # type: ignore[misc]
                    cancelled = self.cancelled.is_set()
                    label = "中断" if cancelled else "完了"
                    summary = (
                        f"{label}: {completed} frame(s), {parts} file(s), "
                        f"{total_bytes:,} bytes"
                    )
                    self.status_var.set(summary)
                    self.current_var.set("待機中")
                    self._append_log(summary)
                    self.start_button.configure(state="normal")
                    self.cancel_button.configure(state="disabled")
                elif kind == "error":
                    self.status_var.set("エラー")
                    self._append_log(f"ERROR: {value}")
                    self.start_button.configure(state="normal")
                    self.cancel_button.configure(state="disabled")
                    messagebox.showerror("エンコードエラー", str(value))
        except queue.Empty:
            pass
        self.root.after(100, self._poll_events)


def main() -> None:
    root = TkinterDnD.Tk() if TkinterDnD is not None else tk.Tk()
    EncoderGui(root)
    root.mainloop()


if __name__ == "__main__":
    main()
