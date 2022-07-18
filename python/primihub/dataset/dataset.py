"""
 Copyright 2022 Primihub

 Licensed under the Apache License, Version 2.0 (the "License");
 you may not use this file except in compliance with the License.
 You may obtain a copy of the License at

      https://www.apache.org/licenses/LICENSE-2.0

 Unless required by applicable law or agreed to in writing, software
 distributed under the License is distributed on an "AS IS" BASIS,
 WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 See the License for the specific language governing permissions and
 limitations under the License.
 """

import abc
from typing import List

import pandas as pd
import pyarrow as pa
from primihub.context import Context
from primihub.context import reg_dataset


class DataDriver(abc.ABC):
    # @abc.abstractmethod
    # def cursor():
    #     raise NotImplementedError
    pass


class FileDriver(DataDriver):
    @abc.abstractmethod
    def read(self, path):
        raise NotImplementedError


class DBDriver(DataDriver):
    @abc.abstractmethod
    def connect(self, **kwargs):
        raise NotImplementedError


class Dataset:
    def __init__(self, df_data: pd.DataFrame, df_meta: pd.DataFrame = None):
        self.df_data: pd.DataFrame = df_data

    def as_arrow(self) -> pa.Table:
        return pa.Table.from_pandas(self.df_data)


class Cursor(abc.ABC):
    pass


class CSVCursor(Cursor):
    def __init__(self, path: str):
        self.path = path
        self.cursor: int = 0;

    # @property
    # def cursor(self):
    #     return self.cursor

    def read(self, names: List = None,
             usecols: List = None,
             skiprows=None,
             nrows=None, **kwargs) -> Dataset:
        if skiprows is not None:
            self.cursor += skiprows

        df = pd.read_csv(self.path, names=names, usecols=usecols, skiprows=self.cursor, nrows=nrows, **kwargs)

        if nrows is not None:
            self.cursor += nrows
            # self.cursor += d/
        return Dataset(df)


class CSVDataDriver(FileDriver):
    """ CSV data driver.
    Usage:
        cursor = CSVDataDriver().read("path/to/file.csv")
        for piece in cursor.read(usecols, skiprows, nrows):
            arrow_table = piece.as_arrow()
    """

    def __init__(self) -> None:
        self.cursor: CSVCursor = None

    def read(self, path) -> CSVCursor:
        self.cursor = CSVCursor(path)
        return self.cursor


class HDFSDataDriver(DataDriver):
    # TODO (chenhongbo)
    pass


__drivers__ = {
    "csv": CSVDataDriver,
}


def driver(name: str) -> DataDriver:
    """Returns a driver object.
    """
    # FIXME (chenhongbo) key error
    return __drivers__[name]


# @reg_dataset
def get(dataset: str):
    Context.datasets.append(dataset)
    print("mock registe dataset:", dataset)


@reg_dataset
def define(dataset: str):
    print("mock registe dataset:", dataset)


def read(dataset_key: str = None,
         names: List = None,
         usecols: List = None,
         skiprows=None,
         nrows=None, **kwargs):
    dataset_path = Context.dataset_map.get(dataset_key, None)
    d = driver(name="csv")
    cursor = d().read(path=dataset_path)
    dataset = cursor.read(names=names, usecols=usecols, skiprows=skiprows, nrows=nrows)
    return dataset
