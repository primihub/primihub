"""
 Copyright 2022 PrimiHub

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
import pickle
from primihub.context import Context
from primihub.context import reg_dataset
from primihub.utils.logger_util import logger

import grpc
from primihub.client.ph_grpc.src.primihub.protos import service_pb2
from primihub.client.ph_grpc.src.primihub.protos import service_pb2_grpc

import zipfile
import os
from torchvision.io import read_image
from torch.utils.data import Dataset as TorchDataset

def register_dataset(service_addr, driver, path, name):
    logger.info("Dataset service is {}.".format(service_addr))
    # ip:port:use_tls:role
    server_info = service_addr.split(":")
    if len(server_info) < 3:
        err_msg = "Register dataset read ca failed. {}".format(str(e))
        logger.error(err_msg)
        raise RuntimeError(err_msg)
    host_port = f"{server_info[0]}:{server_info[1]}"
    use_tls = server_info[2]
    if use_tls == '1':
        try:
            root_ca_path = Context.get_root_ca_path()
            with open(root_ca_path, 'rb') as f:
                root_ca = f.read()
            key_path = Context.get_key_path()
            with open(key_path, 'rb') as f:
                private_key = f.read()
            cert_path = Context.get_cert_path()
            with open(cert_path, 'rb') as f:
                cert = f.read()
        except Exception as e:
            err_msg = "Register dataset read ca failed. {}".format(str(e))
            logger.error(err_msg)
            raise RuntimeError(err_msg)

        creds = grpc.ssl_channel_credentials(root_ca, private_key, cert)
        channel = grpc.secure_channel(host_port, creds)
    else:
        channel = grpc.insecure_channel(host_port)

    stub = service_pb2_grpc.DataSetServiceStub(channel)

    reg_req_data = {
        "op_type": service_pb2.NewDatasetRequest.REGISTER,
        "meta_info": {
          "id": name,
          "driver": driver,
          "access_info": path,
          "visibility": service_pb2.MetaInfo.PUBLIC
        }
    }
    request = service_pb2.NewDatasetRequest(**reg_req_data)
    response = stub.NewDataset(request)
    if response.ret_code != 0:
        logger.error("Register dataset {} failed.".format(name));
        raise RuntimeError("Register dataset {} failed.".format(name))
    else:
        logger.info("Register dataset {} finish, dataset url is {}.".format(
            name, response.dataset_url))


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


class ImageCursor(TorchDataset):
    def __init__(self, annotations_file: str, img_dir: str,
                 transform=None, target_transform=None):
        self.img_labels = pd.read_csv(annotations_file)
        if zipfile.is_zipfile(img_dir):
            with zipfile.ZipFile(img_dir, 'r') as zip_ref:
                zip_ref.extractall(os.path.dirname(img_dir))
                self.img_dir = img_dir.strip('.zip')
        else:
            self.img_dir = img_dir
        self.transform = transform
        self.target_transform = target_transform

    def __len__(self):
        return len(self.img_labels)

    def __getitem__(self, idx):
        img_path = os.path.join(self.img_dir, self.img_labels.loc[idx, 'file_name'])
        image = read_image(img_path)
        label = self.img_labels.loc[idx, 'y']
        if self.transform:
            image = self.transform(image)
        if self.target_transform:
            label = self.target_transform(label)
        return image, label


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


class ImageDataDriver(FileDriver):
    """ Image data driver.
    Usage:
        cursor = ImageDataDriver().read("path/to/file")
    """

    def __init__(self) -> None:
        self.cursor: ImageCursor = None

    def read(self, annotations_file, img_dir,
             transform, target_transform) -> ImageCursor:
        self.cursor = ImageCursor(annotations_file, img_dir,
                                  transform, target_transform)
        return self.cursor


class HDFSDataDriver(DataDriver):
    # TODO (chenhongbo)
    pass


__drivers__ = {
    "csv": CSVDataDriver,
    "image": ImageDataDriver,
}


def driver(name: str) -> DataDriver:
    """Returns a driver object.
    """
    # FIXME (chenhongbo) key error
    return __drivers__[name]


# @reg_dataset
def get(dataset: str):
    Context.datasets.append(dataset)
    print("mock register dataset:", dataset)


@reg_dataset
def define(dataset: str):
    print("mock register dataset:", dataset)


def read(dataset_key: str = None,
         names: List = None,
         usecols: List = None,
         skiprows=None,
         nrows=None,
         transform=None,
         target_transform=None,
         **kwargs):
    dataset_info = eval(Context.dataset_map.get(dataset_key, None))
    file_type = dataset_info['type']

    if file_type == "csv":
        dataset_path = dataset_info['file_path']
        d = driver(name="csv")
        cursor = d().read(path=dataset_path)
        dataset = cursor.read(names=names, usecols=usecols,
                              skiprows=skiprows, nrows=nrows)
    elif file_type == "image":
        annotations_file = dataset_info['annotations_file']
        img_dir = dataset_info['image_dir']
        d = driver(name="image")
        dataset = d().read(annotations_file, img_dir,
                           transform, target_transform)
    else:
        logger.error(f"Not supported file type: {file_type}")
        raise NotImplementedError()

    return dataset


class DatasetRef:
    """Dataset reference.
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
    dataset_client = DatasetClientFactory.create("flight", primihub_cli.node, primihub_cli.cert)
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
    dataset_client = DatasetClientFactory.create("flight", primihub_cli.node, primihub_cli.cert)
    # get dataset use dataset id.
    df_data = dataset_client.do_get(dataset_ref.dataset_name)
    return df_data
