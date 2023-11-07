import numpy as np
from numpy.random import default_rng
import pandas as pd
import os
import imghdr
import zipfile
import yaml
import json
from sqlalchemy import create_engine
from torchvision.io import read_image
from torch.utils.data import Dataset as TorchDataset
from primihub.utils.logger_util import logger
from primihub.context import Context
from primihub.client.seatunnel_client import SeatunnelClient
from primihub.client.ph_grpc.dataset_stream import DataStreamGrpcClient
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
    elif data_type == 'seatunnel':
        return read_from_seatunnel(data_info, selected_column, id)
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

def read_from_seatunnel(data_info,
                        selected_column,
                        need_drop_col_name):
    print("read_from_seatunnel data info", data_info)
    print("request id: ", Context.request_id())
    print("server_config_file", Context.server_config_file())
    server_conf_file = Context.server_config_file()
    with open(server_conf_file, encoding='utf-8') as fd:
        server_config = yaml.safe_load(fd)
    jvm_config_file = server_config["seatunnel"]["config_file"]
    task_template_file = server_config["seatunnel"]["task_template_file"]
    with open(jvm_config_file, encoding='utf-8') as fd:
        jvm_config = fd.read()
    with open(task_template_file, encoding='utf-8') as fd:
        seatunnel_task_config = json.load(fd)
    client = SeatunnelClient(str.encode(jvm_config))
    dataset_id = data_info["dataset_id"]
    request_id = Context.request_id()
    # config task
    proxy_node = server_config["proxy_server"]
    j_sink = seatunnel_task_config["sink"]
    if data_info["source_type"] in ["dm"]:
        query_sql = f"""SELECT * FROM
                        "{data_info['dbName']}"."{data_info['tableName']}" """
    else:
        query_sql = f"SELECT * FROM {data_info['tableName']}"

    for item in j_sink:
        item["dataSetId"] = dataset_id
        item["traceId"] = request_id
        item["host"] = proxy_node["ip"]
        item["port"] = proxy_node["port"]
    j_source = seatunnel_task_config["source"]
    for item in j_source:
        item["password"] = data_info["password"]
        item["user"] = data_info["username"]
        item["url"] = data_info["dbUrl"]
        item["driver"] = data_info["dbDriver"]
        item["query"] = query_sql

    task_config = json.dumps(seatunnel_task_config)
    print("seatunnel task config ", task_config)
    client.SubmitTask(task_config)
    proxy_info = "{}:{}".format(proxy_node["ip"], proxy_node["port"])
    logger.info(f"proxy_info: {proxy_info}")
    fetch_data_client = DataStreamGrpcClient(proxy_info)
    recv_data = fetch_data_client.featch_data(dataset_id, request_id)
    colums = []
    if selected_column:
        colums = selected_column
    else:
        for key in recv_data.keys():
            colums.append(key)
    df = pd.DataFrame(recv_data, columns=colums)
    if need_drop_col_name in df.columns:
        df.pop(need_drop_col_name)
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
