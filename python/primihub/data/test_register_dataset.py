#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Created on Mon Apr 17 15:14:20 2023

@author: czl
"""
import abc
from typing import List

import pandas as pd
from primihub.utils.logger_util import logger

import grpc
from primihub.client.ph_grpc.src.primihub.protos import service_pb2
from primihub.client.ph_grpc.src.primihub.protos import service_pb2_grpc

def register_dataset(ip, port, use_tls, driver, path, name):
    logger.info(f"Dataset service is ip: {ip}, port: {port}, use_tls: {use_tls}")
    host_port = f"{ip}:{port}"
    use_tls = '0' #No implementation error
    if use_tls == '1': #No implementation error
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

    stub = service_pb2_grpc.DataServiceStub(channel)
    request = service_pb2.NewDatasetRequest()
    request.fid = name
    request.driver = driver
    request.path = path

    response = stub.NewDataset(request)
    if response.ret_code != 0:
        logger.error("Register dataset {} failed.".format(name));
        raise RuntimeError("Register dataset {} failed.".format(name))
    else:
        logger.info("Register dataset {} finish, dataset url is {}.".format(
            name, response.dataset_url))
        

array = np.random.uniform(0, 100, (10,10))
char_list = random.sample('zyxwvutsrqponmlkjihgfedcba', 20)

name = ''
for char in char_list:
    name = name + char

path = "/tmp/{}.csv".format(name)
np.savetxt(path, array, delimiter=",")

service_addr = ph.context.Context.params_map["DatasetServiceAddr"]
register_dataset(service_addr, "csv", path, name)