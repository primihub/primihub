import numpy as np
from numpy.random import default_rng
import pandas as pd
import os
import imghdr
import zipfile
from sqlalchemy import create_engine
from torchvision.io import read_image
from torch.utils.data import Dataset as TorchDataset
from primihub.utils.logger_util import logger
import pyarrow.parquet as pq
import pyarrow as pa

def read_data(data_info,
              selected_column=None,
              droped_column=None,
              transform=None,
              target_transform=None):
    data_type = data_info['type'].lower()
    if data_type == 'csv':
        return read_csv(data_info['data_path'],
                        selected_column,
                        droped_column)
    elif data_type == 'image':
        return TorchImageDataset(data_info['image_dir'],
                                 data_info['annotations_file'],
                                 transform,
                                 target_transform)
    elif data_type == 'mysql':
        return read_mysql(data_info['username'],
                          data_info['password'],
                          data_info['host'],
                          data_info['port'],
                          data_info['dbName'],
                          data_info['tableName'],
                          selected_column,
                          droped_column)
    elif data_type == 'parquet':
        return read_parquet(data_info['data_path'],
                            selected_column,
                            droped_column)
    else:
        error_msg = f'Unsupported data type: {data_type}'
        logger.error(error_msg)
        raise RuntimeError(error_msg)


def read_csv(data_path, selected_column=None, droped_column=None):
    data = pd.read_csv(data_path)
    if selected_column:
        data = data[selected_column]
    if droped_column in data.columns:
        data.pop(droped_column)
    return data

def read_parquet(data_path, selected_column=None, droped_column=None):
    parquet_file = pq.ParquetFile(data_path)
    data = parquet_file.read().to_pandas()
    if selected_column:
        data = data[selected_column]
    if droped_column in data.columns:
        data.pop(droped_column)
    return data

def read_mysql(user,
               password,
               host,
               port,
               database,
               table_name,
               selected_column=None,
               droped_column=None):
    engine_str = f"mysql+mysqlconnector://{user}:{password}@{host}:{port}/{database}"
    engine = create_engine(engine_str)
    with engine.connect() as conn:
        df = pd.read_sql_table(table_name, conn, columns=selected_column)
        if droped_column in df.columns:
            df.pop(droped_column)
        return df


class TorchImageDataset(TorchDataset):

    def __init__(self,
                 img_dir,
                 annotations_file=None,
                 transform=None,
                 target_transform=None):
        if zipfile.is_zipfile(img_dir):
            with zipfile.ZipFile(img_dir, 'r') as zip_ref:
                zip_ref.extractall(os.path.dirname(img_dir))
                self.img_dir = os.path.join(os.path.dirname(img_dir),
                                             zip_ref.namelist()[0])
        else:
            self.img_dir = img_dir

        self.transform = transform
        self.target_transform = target_transform

        img_type = ['jpeg', 'png']
        if annotations_file:
            self.img_labels = pd.read_csv(annotations_file)
        else:
            file_name = [
                f for f in os.listdir(self.img_dir)
                if imghdr.what(os.path.join(self.img_dir, f)) in img_type
            ]
            self.img_labels = pd.DataFrame(file_name,
                                           columns=['file_name'])

    def __len__(self):
        return len(self.img_labels)

    def __getitem__(self, idx):
        img_path = os.path.join(self.img_dir,
                                self.img_labels.loc[idx, 'file_name'])
        image = read_image(img_path)
        if self.transform:
            image = self.transform(image)

        if 'y' in self.img_labels.columns:
            label = self.img_labels.loc[idx, 'y']
            if self.target_transform:
                label = self.target_transform(label)
            return image, label
        else:
            return image


class DataLoader:

    def __init__(self, dataset, label=None, batch_size=1, shuffle=True, seed=None):
        self.dataset = dataset
        self.label = label
        self.batch_size = batch_size
        self.shuffle = shuffle
        self.n_samples = self.__len__()
        self.indices = np.arange(self.n_samples)
        self.start = 0
        if seed:
            np.random.seed(seed)

    def __len__(self):
        return len(self.dataset)

    def __getitem__(self, idx):
        return self.dataset[idx]

    def __iter__(self):
        return self

    def __next__(self):
        if self.start == 0:
            if self.shuffle:
                np.random.shuffle(self.indices)

        start = self.start
        end = start + self.batch_size
        if end > self.n_samples:
            end = self.n_samples
        self.start += self.batch_size

        if start < self.n_samples:
            batch_idx = self.indices[start:end]
            if self.label is not None:
                return self.dataset[batch_idx], self.label[batch_idx]
            else:
                return self.dataset[batch_idx]
        else:
            self.start = 0
            raise StopIteration


class DPDataLoader(DataLoader):

    def __init__(self, dataset, label=None, batch_size=1):
        self.dataset = dataset
        self.label = label
        self.batch_size = batch_size
        self.rng = default_rng()
        self.n_samples = self.__len__()
        self.max_iter = self.n_samples // self.batch_size
        self.num_iter = 0

    def __next__(self):
        self.num_iter += 1
        if self.num_iter <= self.max_iter:
            batch_idx = self.rng.choice(self.n_samples,
                                        self.batch_size,
                                        replace=False)
            if self.label is not None:
                return self.dataset[batch_idx], self.label[batch_idx]
            else:
                return self.dataset[batch_idx]
        else:
            self.num_iter = 0
            raise StopIteration