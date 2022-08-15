from primihub.FL.model.logistic_regression.homo_lr_base import LRModel
import numpy as np
import pandas as pd
import copy
from primihub.FL.proxy.proxy import ServerChannelProxy
from primihub.FL.proxy.proxy import ClientChannelProxy
from os import path
import logging

proxy_server_arbiter = ServerChannelProxy("10091")  # host接收来自arbiter消息
proxy_server_guest = ServerChannelProxy("10095")  # host接收来自guest消息
proxy_client_arbiter = ClientChannelProxy(
    "127.0.0.1", "10092")  # host发送消息给arbiter
proxy_client_guest = ClientChannelProxy("127.0.0.1", "10093")  # host发送消息给guest

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
    def __init__(self, X, y):
        self.X = X
        self.y = y
        self.model = LRModel(X, y)
        self.public_key = None
        self.iter = None
        self.need_one_vs_rest = None
        self.need_encrypt = True
        self.lr = 0.01
        self.batch_size = 200
        self.iteration = 0
        self.epoch = 10
        self.flag = True

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
            # idx = np.arange(X.shape[0])
            # batch_idx = np.random.choice(idx, self.batch_size, replace=False)
            # X = np.array(X)
            # y = np.array(y)
            # data_x = X[batch_idx]
            batch_x = np.concatenate((np.ones((batch_x.shape[0], 1)), batch_x), axis=1)
            # data_y = y[batch_idx].reshape((-1, 1))
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
            self.model.theta = self.model.fit(batch_x, batch_y)
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


    def encrypt_vector(self, x):
        return [self.public_key.encrypt(i) for i in x]


# def encrypt_matrix(self, x):
#     ret = []
#     for r in x:
#         ret.append(self.encrypt_vector(self.public_key, r))
#     return ret


if __name__ == "__main__":
    conf = {'iter': 2,
            'lr': 0.01,
            'batch_size': 200,
            'epoch': 3}
    # load train data
    X, label = data_process()
    X = LRModel.normalization(X)
    count = X.shape[0]
    batch_num = count // conf['batch_size'] + 1
    proxy_client_arbiter.Remote(batch_num, "batch_num")

    client_host = Host(X, label)
    client_host.iter = conf['iter']
    client_host.lr = conf['lr']
    client_host.batch_size = conf['batch_size']
    batch_gen_host = client_host.batch_generator([X, label], conf['batch_size'])

    proxy_client_arbiter.Remote(client_host.need_encrypt, "need_encrypt")
    proxy_server_arbiter.StartRecvLoop()
    client_host.public_key = proxy_server_arbiter.Get("public_key")

    # Send host_data_weight to arbiter
    host_data_weight = conf['batch_size']
    host_data_weight = client_host.encrypt_vector([host_data_weight])
    proxy_client_arbiter.Remote(host_data_weight, "host_data_weight")

    # 数据处理
    for i in range(conf['epoch']):
        logger.info("######### epoch %s ######### start " % i)
        for j in range(batch_num):
            batch_x, batch_y = next(batch_gen_host)
            logger.info("batch_host_x.shape:{}".format(batch_x.shape))
            logger.info("batch_host_y.shape:{}".format(batch_y.shape))
            host_param = client_host.fit_binary(batch_x, batch_y)
            proxy_client_arbiter.Remote(host_param, "host_param")
            client_host.model.theta = proxy_server_arbiter.Get("global_host_model_param")
        logger.info("######### epoch %s ######### done " % i)
    logger.info("host training process done!")

    proxy_server_arbiter.StopRecvLoop()
