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
from primihub.utils.logger_util import logger

import grpc
from primihub.client.ph_grpc.src.primihub.protos import service_pb2
from primihub.client.ph_grpc.src.primihub.protos import service_pb2_grpc


def register_dataset(service_addr, driver, path, name):
    host_port, use_tls = service_addr.rsplit(":", 1)

    if use_tls == '1':
        with open("data/cert/ca.crt", 'rb') as f:
            root_ca = f.read()

        with open('data/cert/client.key', 'rb') as f:
            private_key = f.read()

        with open('data//cert/client.crt', 'rb') as f:
            cert = f.read()

        creds = grpc.ssl_channel_credentials(root_ca, private_key, cert)
        channel = grpc.secure_channel(host_port, creds)
    else:
        channel = grpc.insecure_channel(service_addr)

    stub = service_pb2_grpc.DataServiceStub(channel)
    request = service_pb2.NewDatasetRequest()
    request.fid = name
    request.driver = driver
    request.path = path

    response = stub.NewDataset(request)
    if response.ret_code != 0:
        raise RuntimeError("Register dataset {} failed.".format(name))


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
        self.cursor: int = 0

    # @property
    # def cursor(self):
    #     return self.cursor

    def read(self, names: List = None,
             usecols: List = None,
             skiprows=None,
             nrows=None, **kwargs) -> Dataset:
        if skiprows is not None:
            self.cursor += skiprows

        df = pd.read_csv(self.path, names=names, usecols=usecols,
                         skiprows=self.cursor, nrows=nrows, **kwargs)

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
    dataset = cursor.read(names=names, usecols=usecols,
                          skiprows=skiprows, nrows=nrows)
    return dataset


class DatasetRef:
    """Dataset refrence.
    """

    def __init__(self) -> None:
        self.dataset_id = "NOT_ASSIGNED"
        self.dataset_status = "UNKNOWN"  # UPLOADING, DOWNLOADING, UNKNOWN
        self.type = "LOCAL"  # LOCAL, REMOTE
        self.dataset_url = "NOT_ASSIGNED"
        self.dataset_name = "NOT_ASSIGNED"
        self.dataset: Dataset = None

    def from_meta(self, meta):
        try:
            print(meta)
            self.dataset_id = meta["id"]
            self.type = "REMOTE"
            self.dataset_url = meta["data_url"]
            self.dataset_name = meta["description"]
        except KeyError as e:
            print(e)

    def __repr__(self) -> str:
        return f"name: [ {self.dataset_name} ], id:[ {self.dataset_id} ], url:[ {self.dataset_url} ]"


def put(df_data: pd.DataFrame, dataset_key: str = None) -> DatasetRef:
    """ Put dataset using primihub client (singltone) Dataset client.
        Default is flight client.
    """
    from primihub.client import primihub_cli
    from primihub.dataset.dataset_cli import DatasetClientFactory
    # FIXME use primihub cli client
    # dataset_client = DatasetClientFactory.create("flight", "192.168.99.23:50050", "")
    dataset_client = DatasetClientFactory.create(
        "flight", primihub_cli.node, primihub_cli.cert)
    metas = dataset_client.do_put(df_data, dataset_key)
    dataset_ref = DatasetRef()
    dataset_ref.from_meta(metas[0])
    return dataset_ref


def get(dataset_ref: DatasetRef) -> pd.DataFrame:
    """ Get dataset using primihub client (singltone) Dataset client.
        Default is flight client.
        @return pandas DataFrame object
    """
    from primihub.client import primihub_cli
    from primihub.dataset.dataset_cli import DatasetClientFactory
    # FIXME use primihub cli client
    # dataset_client = DatasetClientFactory.create("flight", "192.168.99.23:50050", "")
    dataset_client = DatasetClientFactory.create(
        "flight", primihub_cli.node, primihub_cli.cert)
    # get dataset use dataset id.
    df_data = dataset_client.do_get(dataset_ref.dataset_name)
    return df_data
