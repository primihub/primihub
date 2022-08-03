#!/usr/bin/env python
# -*- coding: utf-8 -*-
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
import numpy as np
from sklearn.preprocessing import StandardScaler
from sklearn.datasets import load_breast_cancer
from sklearn.model_selection import train_test_split
# from sklearn.datasets import make_hastie_10_2
from vfl.lr_guest import Guest
from vfl.lr_host import Host
from vfl.lr_arbiter import Arbiter


def load_data():
    # 加载数据
    breast = load_breast_cancer()
    # 数据拆分
    X_train, X_test, y_train, y_test = train_test_split(
        breast.data, breast.target, random_state=1)
    # 数据标准化
    std = StandardScaler()
    X_train = std.fit_transform(X_train)
    X_test = std.transform(X_test)
    print("X_train.shape", X_train.shape, "X_test.shape", X_test.shape)
    return X_train, y_train, X_test, y_test


# 将特征分配给guest和host
def vertically_partition_data(X_train, X_test, guest_idx, host_idx):
    """
    Vertically partition feature for party guest and host
    :param X_train: train feature
    :param X_test: test feature
    :param guest_idx: feature index of party guset
    :param host_idx: feature index of party host
    :return: train data for guest, host; test data for guest, host
    """

    X_guest_train = X_train[:, guest_idx]
    X_host_train = X_train[:, host_idx]
    X_host_train = np.c_[np.ones(X_train.shape[0]), X_host_train]
    X_guest_test = X_test[:, guest_idx]
    X_host_test = X_test[:, host_idx]
    X_host_test = np.c_[np.ones(X_test.shape[0]), X_host_test]
    return X_guest_train, X_host_train, X_guest_test, X_host_test


def vertical_logistic_regression(
        X_guest_train,
        X_host_train,
        config,
        batch_num):
    """
    Start the processes of the three clients: guest, host and arbiter.
    :param X_guest_train: features of the guest
    :param X_host_train: features of the guest
    :param config: including hyperparameters
    :param batch_num: number of batches for training
    :return: weights of guest and host,test data of guest and host
    """
    # 各参与方的初始化
    client_guest = Guest(X_guest_train, config)
    client_host = Host(X_host_train, y_train, config)
    client_arbiter = Arbiter(X_guest_train.shape, X_host_train.shape, config)

    # 各参与方之间连接的建立
    client_guest.connect("host", client_host)
    client_guest.connect("arbiter", client_arbiter)
    client_host.connect("guest", client_guest)
    client_host.connect("arbiter", client_arbiter)
    client_arbiter.connect("guest", client_guest)
    client_arbiter.connect("host", client_host)

    batch_gen_guest = batch_generator(
        [X_guest_train, y_train], config['batch_size'], False)
    batch_gen_host = batch_generator(
        [X_host_train, y_train], config['batch_size'], False)
    client_arbiter.send_pub("guest", "host")
    for i in range(config['epochs']):
        print("##### epoch %s ##### " % i)
        for j in range(batch_num):
            print("-----epoch=%s, batch=%s-----" % (i, j))
            batch_host_x, batch_host_y = next(batch_gen_host)
            print("batch_host_x.shape",batch_host_x.shape)
            print("batch_host_y.shape",batch_host_y.shape)
            batch_guest_x, batch_guest_y = next(batch_gen_guest)
            print("batch_guest_x.shape", batch_guest_x.shape)
            print("batch_guest_y.shape", batch_guest_y.shape)
            client_guest.task_1(batch_guest_x, "host")
            client_host.task_1(batch_host_x, batch_host_y, "guest")
            client_guest.task_2(batch_guest_x,"arbiter")
            client_host.task_2(batch_host_x,"arbiter")
            client_arbiter.dec_gradient("guest", "host")
            client_guest.task_3(batch_guest_x)
            client_host.task_3(batch_host_x)
    print("All process done.")
    weights = np.concatenate(
        (client_host.weights, client_guest.weights), axis=0)
    print("weights.shape", weights.shape)
    return weights


def batch_generator(all_data, batch_size, shuffle=True):
    """
    :param all_data : incluing features and label
    :param batch_size: number of samples in one batch
    :param shuffle: Whether to disrupt the order
    :return:iterator to generate every batch of features and labels
    """
    # 每个元素都是numpy数组，方便取值
    all_data = [np.array(d) for d in all_data]
    # 获取样本大小
    data_size = all_data[0].shape[0]
    print("data_size: ", data_size)
    if shuffle:
        # 随机生成打乱的索引
        p = np.random.permutation(data_size)
        # 重新组织数据
        all_data = [d[p] for d in all_data]
    batch_count = 0
    while True:
        # 数据一轮循环(epoch)完成，打乱一次顺序
        if batch_count * batch_size + batch_size > data_size:
            batch_count = 0
            if shuffle:
                p = np.random.permutation(data_size)
                all_data = [d[p] for d in all_data]
        start = batch_count * batch_size
        end = start + batch_size
        batch_count += 1
        yield [d[start: end] for d in all_data]


config = {
    'epochs': 2,
    'lambda': 10,
    'lr': 0.05,
    'batch_size': 50,
    'guest_idx': [
        10,
        11,
        12,
        13,
        14,
        15,
        16,
        17,
        18,
        19,
        20,
        21,
        22,
        23,
        24,
        25,
        26,
        27,
        28,
        29],
    'host_idx': [
        0,
        1,
        2,
        3,
        4,
        5,
        6,
        7,
        8,
        9],}

X_train, y_train, X_test, y_test = load_data()
X_guest_train, X_host_train, X_guest_test, X_host_test = vertically_partition_data(
    X_train, X_test, config['guest_idx'], config['host_idx'])
count = X_train.shape[0]
batch_num = count // config['batch_size'] + 1
weights = vertical_logistic_regression(
    X_guest_train, X_host_train, config, batch_num)  # 分批次训练
X_test_con = np.concatenate((X_host_test, X_guest_test), axis=1)
Host.predict(weights, X_test_con, y_test)
