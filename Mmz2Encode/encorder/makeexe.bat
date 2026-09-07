rem install pyinstaller
rem python -m pip install -r requirements.txt
rem python -m pip install pyinstaller
rem make exe
rem python -m PyInstaller --noconfirm --clean --windowed --onedir --name MMZEncoder --collect-all tkinterdnd2 mz2200_gui.py
python -m PyInstaller --noconfirm --clean --windowed --onefile --name MMZEncoder  --collect-all tkinterdnd2 mz2200_gui.py
