from primihub.FL.model.logistic_regression.homo_lr_base import LRModel
import numpy as np
from os import path
import pandas as pd
import copy
from primihub.FL.proxy.proxy import ServerChannelProxy
from primihub.FL.proxy.proxy import ClientChannelProxy
import logging

path = path.join(path.dirname(__file__), '../../../tests/data/wisconsin.data')


def get_logger(name):
    LOG_FORMAT = "[%(asctime)s][%(filename)s:%(lineno)d][%(levelname)s] %(message)s"
    DATE_FORMAT = "%m/%d/%Y %H:%M:%S %p"
    logging.basicConfig(level=logging.DEBUG,
                        format=LOG_FORMAT, datefmt=DATE_FORMAT)
    logger = logging.getLogger(name)
    return logger


logger = get_logger("Homo-LR-Guest")


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


class Guest:
    def __init__(self, X, y, config, proxy_server, proxy_client_arbiter):
        self.X = X
        self.y = y
        self.config = config
        self.model = LRModel(X, y)
        self.need_one_vs_rest = None
        self.need_encrypt = False
        self.batch_size = None
        self.proxy_server = proxy_server
        self.proxy_client_arbiter = proxy_client_arbiter

    def predict(self, data=None):
        if self.need_one_vs_rest:
            pass
        else:
            pre = self.model.predict(data)
        return pre

    def fit_binary(self, X, y):
        # if self.need_encrypt == True:
        #     model_param = Utils.encrypt_vector(self.public_key, self.global_model.theta)
        #     neg_one = self.public_key.encrypt(-1)
        #
        #     for e in range(1):  # 10为本地epoch大小
        #         print("start epoch ", e)
        #         # 每一轮都随机挑选batch_size大小的训练数据进行训练
        #         idx = np.arange(X.shape[0])
        #         batch_idx = np.random.choice(idx, self.batch_size, replace=False)
        #         x = X[batch_idx]
        #         x = np.concatenate((np.ones((x.shape[0], 1)), x), axis=1)
        #         y = y[batch_idx].values.reshape((-1, 1))
        #         # 在加密状态下求取加密梯度
        #         batch_encrypted_grad = x.transpose() * (
        #                 0.25 * x.dot(model_param) + 0.5 * y.transpose() * neg_one)
        #         encrypted_grad = batch_encrypted_grad.sum(axis=1) / y.shape[0]
        #
        #         for j in range(len(model_param)):
        #             model_param[j] -= self.lr * encrypted_grad[j]
        #
        #     # weight_accumulators = []
        #     # for j in range(len(self.local_model.encrypt_weights)):
        #     #     weight_accumulators.append(self.local_model.encrypt_weights[j] - original_w[j])
        #     return model_param
        # plaintext
        self.model.theta = self.model.fit(X, y, eta=self.config['lr'])
        self.model.theta = list(self.model.theta)
        return self.model.theta

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



def run_homo_lr_guest(role_node_map, node_addr_map, params_map={}):
    guest_nodes = role_node_map["guest"]
    arbiter_nodes = role_node_map["arbiter"]

    if len(guest_nodes) != 1:
        logger.error("Hetero LR only support one guest party, but current "
                     "task have {} guest party.".format(len(guest_nodes)))
        return

    if len(arbiter_nodes) != 1:
        logger.error("Hetero LR only support one arbiter party, but current "
                     "task have {} arbiter party.".format(len(arbiter_nodes)))
        return

    guest_port = node_addr_map[guest_nodes[0]].split(":")[1]
    proxy_server = ServerChannelProxy(guest_port)
    proxy_server.StartRecvLoop()
    logger.debug("Create server proxy for guest, port {}.".format(guest_port))

    arbiter_ip, arbiter_port = node_addr_map[arbiter_nodes[0]].split(":")
    proxy_client_arbiter = ClientChannelProxy(
        arbiter_ip, arbiter_port, "arbiter")
    logger.debug("Create client proxy to arbiter,"
                 " ip {}, port {}.".format(arbiter_ip, arbiter_port))

    config = {
        'epochs': 1,
        'lr': 0.05,
        'batch_size': 500
    }


    x, label = data_process()
    x = LRModel.normalization(x)
    count_train = x.shape[0]
    batch_num_train = count_train // config['batch_size'] + 1

    guest_data_weight = config['batch_size']
    proxy_client_arbiter.Remote(guest_data_weight, "guest_data_weight")
    client_guest = Guest(x, label, config, proxy_server,
                          proxy_client_arbiter)

    batch_gen_guest = client_guest.batch_generator(
        [x, label], config['batch_size'], False)

    for i in range(config['epochs']):
        logger.info("##### epoch %s ##### " % i)
        for j in range(batch_num_train):
            logger.info("-----epoch=%s, batch=%s-----" % (i, j))
            batch_x, batch_y = next(batch_gen_guest)
            logger.info("batch_host_x.shape:{}".format(batch_x.shape))
            logger.info("batch_host_y.shape:{}".format(batch_y.shape))
            guest_param = client_guest.fit_binary(batch_x, batch_y)
            proxy_client_arbiter.Remote(guest_param, "guest_param")
            client_guest.model.theta = proxy_server.Get("global_guest_model_param")
            logger.info("batch=%s done" % j)
        logger.info("epoch=%i done" % i)
    logger.info("guest training process done.")


    proxy_server.StopRecvLoop()
