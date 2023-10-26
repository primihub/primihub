from pathlib import Path
import os
import json
import pickle
import pandas
from primihub.utils.logger_util import logger


def check_directory_exist(file_path):
    directory = os.path.dirname(file_path)
    Path(directory).mkdir(parents=True, exist_ok=True)
    logger.info(f"path: {file_path}")


def save_json_file(data, path):
    check_directory_exist(path)
    with open(path, 'w') as file:
        json.dump(data, file)


def save_pickle_file(data, path):
    check_directory_exist(path)
    with open(path, 'wb') as file:
        pickle.dump(data, file)


def load_pickle_file(path):
    with open(path, 'rb') as file:
        data = pickle.load(file)
    return data


def save_csv_file(data, path):
    check_directory_exist(path)
    if not isinstance(data, pandas.DataFrame):
        raise TypeError("Data must be a pandas DataFrame")
    data.to_csv(path, index=False)