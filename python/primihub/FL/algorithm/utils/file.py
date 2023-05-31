from pathlib import Path
import os


def check_directory_exist(file_path):
    directory = os.path.dirname(file_path)
    Path(directory).mkdir(parents=True, exist_ok=True)