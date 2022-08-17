from primihub.FL.model.logistic_regression.homo_lr_base import LRModel
import numpy as np
import pandas as pd
import copy
from primihub.FL.proxy.proxy import ServerChannelProxy
from primihub.FL.proxy.proxy import ClientChannelProxy
from os import path
import logging


path = path.join(path.dirname(__file__), '../../../tests/data/wisconsin.data')


def get_logger(name):
    LOG_FORMAT = "[%(asctime)s][%(filename)s:%(lineno)d][%(levelname)s] %(message)s"
    DATE_FORMAT = "%m/%d/%Y %H:%M:%S %p"
    logging.basicConfig(level=logging.DEBUG,
                        format=LOG_FORMAT, datefmt=DATE_FORMAT)
    logger = logging.getLogger(name)
    return logger


logger = get_logger("Homo-LR-Host")


def data_process():
    X1 = pd.read_csv(path, header=None)
    y1 = X1.iloc[:, -1]
    yy = copy.deepcopy(y1)
    # 处理标签
    for i in range(len(yy.values)):
        if yy[i] == 2:
            yy[i] = 0
        else:
            yy[i] = 1
    X1 = X1.iloc[:, :-1]
    return X1, yy


class Host:
    def __init__(self, X, y, config, proxy_server, proxy_client_arbiter):
        self.X = X
        self.y = y
        self.config = config
        self.model = LRModel(X, y)
        self.public_key = None
        self.need_one_vs_rest = None
        self.need_encrypt = False
        self.lr = self.config['lr']
        self.batch_size = 200
        self.iteration = 0
        self.epoch = 10
        self.flag = True
        self.proxy_server = proxy_server
        self.proxy_client_arbiter = proxy_client_arbiter

    def predict(self, data=None):
        if self.need_one_vs_rest:
            pass
        else:
            pre = self.mode.predict(data)
            return pre

    def fit_binary(self, batch_x, batch_y):
        if self.need_encrypt == True:
            if self.flag == True:
                # Only need to encrypt once
                self.model.theta = self.encrypt_vector(self.model.theta)
                self.flag = False
            # Convert subtraction to addition
            neg_one = self.public_key.encrypt(-1)
            batch_x = np.concatenate((np.ones((batch_x.shape[0], 1)), batch_x), axis=1)
            # use taylor approximation
            batch_encrypted_grad = batch_x.transpose() * (
                    0.25 * batch_x.dot(self.model.theta) + 0.5 * batch_y.transpose() * neg_one)
            encrypted_grad = batch_encrypted_grad.sum(axis=1) / batch_y.shape[0]

            for j in range(len(self.model.theta)):
                self.model.theta[j] -= self.lr * encrypted_grad[j]

            # weight_accumulators = []
            # for j in range(len(self.local_model.encrypt_weights)):
            #     weight_accumulators.append(self.local_model.encrypt_weights[j] - original_w[j])
            # print('host model_param---->', model_param)
            return self.model.theta

        else:  # Plaintext
            self.model.theta = self.model.fit(batch_x, batch_y, eta=self.lr)
            return list(self.model.theta)

    def batch_generator(self, all_data, batch_size, shuffle=True):

        """
        :param all_data : incluing features and label
        :param batch_size: number of samples in one batch
        :param shuffle: Whether to disrupt the order
        :return:iterator to generate every batch of features and labels
        """
        # Each element is a numpy array
        all_data = [np.array(d) for d in all_data]
        data_size = all_data[0].shape[0]
        logger.info("data_size: {}".format(data_size))
        if shuffle:
            p = np.random.permutation(data_size)
            all_data = [d[p] for d in all_data]
        batch_count = 0
        while True:
            # The epoch completes, disrupting the order once
            if batch_count * batch_size + batch_size > data_size:
                batch_count = 0
                if shuffle:
                    p = np.random.permutation(data_size)
                    all_data = [d[p] for d in all_data]
            start = batch_count * batch_size
            end = start + batch_size
            batch_count += 1
            yield [d[start: end] for d in all_data]


    def encrypt_vector(self, x):
        return [self.public_key.encrypt(i) for i in x]


# def encrypt_matrix(self, x):
#     ret = []
#     for r in x:
#         ret.append(self.encrypt_vector(self.public_key, r))
#     return ret


def run_homo_lr_host(role_node_map, node_addr_map, params_map={}):
    host_nodes = role_node_map["host"]
    # guest_nodes = role_node_map["guest"]
    arbiter_nodes = role_node_map["arbiter"]

    if len(host_nodes) != 1:
        logger.error("Hetero LR only support one host party, but current "
                     "task have {} host party.".format(len(host_nodes)))
        return

    if len(arbiter_nodes) != 1:
        logger.error("Hetero LR only support one arbiter party, but current "
                     "task have {} arbiter party.".format(len(host_nodes)))
        return

    host_port = node_addr_map[host_nodes[0]].split(":")[1]
    proxy_server = ServerChannelProxy(host_port)
    proxy_server.StartRecvLoop()
    logger.debug("Create server proxy for host, port {}.".format(host_port))

    arbiter_ip, arbiter_port = node_addr_map[arbiter_nodes[0]].split(":")
    proxy_client_arbiter = ClientChannelProxy(arbiter_ip, arbiter_port, "host")
    logger.debug("Create client proxy to arbiter,"
                 " ip {}, port {}.".format(arbiter_ip, arbiter_port))

    config = {
        'epochs': 1,
        'lr': 0.05,
        'batch_size': 500
    }
    x, label = data_process()
    client_host = Host(x, label, config, proxy_server, proxy_client_arbiter)
    x = LRModel.normalization(x)
    count_train = x.shape[0]
    proxy_client_arbiter.Remote(client_host.need_encrypt, "need_encrypt")
    batch_num_train = count_train // config['batch_size'] + 1
    proxy_client_arbiter.Remote(batch_num_train, "batch_num")
    host_data_weight = config['batch_size']
    if client_host.need_encrypt == True:
        client_host.public_key = proxy_server.Get("pub")
        host_data_weight = client_host.encrypt_vector([host_data_weight])

    proxy_client_arbiter.Remote(host_data_weight, "host_data_weight")

    batch_gen_host = client_host.batch_generator(
        [x, label], config['batch_size'], False)
    for i in range(config['epochs']):
        logger.info("##### epoch %s ##### " % i)
        for j in range(batch_num_train):
            logger.info("-----epoch=%s, batch=%s-----" % (i, j))
            batch_host_x, batch_host_y = next(batch_gen_host)
            logger.info("batch_host_x.shape:{}".format(batch_host_x.shape))
            logger.info("batch_host_y.shape:{}".format(batch_host_y.shape))
            host_param = client_host.fit_binary(batch_host_x, batch_host_y)
            proxy_client_arbiter.Remote(host_param, "host_param")
            client_host.model.theta = proxy_server.Get("global_host_model_param")
            logger.info("batch=%s done" % j)
        logger.info("epoch=%i done" % i)
    logger.info("host training process done.")

    proxy_server.StopRecvLoop()
