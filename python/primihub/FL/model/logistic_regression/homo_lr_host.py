from primihub.FL.model.logistic_regression.homo_lr_base import LRModel
import numpy as np
import pandas as pd
import copy
from primihub.FL.proxy.proxy import ServerChannelProxy
from primihub.FL.proxy.proxy import ClientChannelProxy
from os import path
import logging
from sklearn.datasets import load_iris

# path = path.join(path.dirname(__file__), '../../../tests/data/wisconsin.data')


def get_logger(name):
    LOG_FORMAT = "[%(asctime)s][%(filename)s:%(lineno)d][%(levelname)s] %(message)s"
    DATE_FORMAT = "%m/%d/%Y %H:%M:%S %p"
    logging.basicConfig(level=logging.DEBUG,
                        format=LOG_FORMAT,
                        datefmt=DATE_FORMAT)
    logger = logging.getLogger(name)
    return logger


logger = get_logger("Homo-LR-Host")


def data_binary(path):
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


def data_iris():
    iris = load_iris()
    X = iris.data
    y = iris.target
    return X, y


class Host:

    def __init__(self, X, y, config, proxy_server, proxy_client_arbiter):
        self.X = X
        self.y = y
        self.config = config
        self.model = LRModel(X, y, self.config['category'])
        self.public_key = None
        self.need_encrypt = self.config['need_encrypt']
        self.lr = self.config['lr']
        self.need_one_vs_rest = None
        self.batch_size = self.config['batch_size']
        self.flag = True
        self.proxy_server = proxy_server
        self.proxy_client_arbiter = proxy_client_arbiter

    def predict(self, data=None):
        pass

    def fit_binary(self, batch_x, batch_y, theta=None):
        if self.need_one_vs_rest == False:
            theta = self.model.theta
        else:
            theta = list(theta)
        if self.need_encrypt == 'YES':
            if self.flag == True:
                # Only need to encrypt once
                theta = self.encrypt_vector(theta)
                self.flag = False
            # Convert subtraction to addition
            neg_one = self.public_key.encrypt(-1)
            batch_x = np.concatenate((np.ones((batch_x.shape[0], 1)), batch_x),
                                     axis=1)
            # use taylor approximation
            batch_encrypted_grad = batch_x.transpose() * (
                0.25 * batch_x.dot(theta) + 0.5 * batch_y.transpose() * neg_one)
            encrypted_grad = batch_encrypted_grad.sum(axis=1) / batch_y.shape[0]
            for j in range(len(theta)):
                theta[j] -= self.lr * encrypted_grad[j]
            return theta

        else:  # Plaintext
            self.model.theta = self.model.fit(batch_x,
                                              batch_y,
                                              theta,
                                              eta=self.lr)
            return list(self.model.theta)

    def one_vs_rest(self, X, y, k):
        all_theta = []
        for i in range(0, k):
            y_i = np.array([1 if label == i else 0 for label in y])
            theta = self.fit_binary(X, y_i, self.model.one_vs_rest_theta[i])
            all_theta.append(theta)
        return all_theta

    def fit(self, X, y, category):
        if category == 2:
            self.need_one_vs_rest = False
            return self.fit_binary(X, y)
        else:
            self.need_one_vs_rest = True
            return self.one_vs_rest(X, y, category)

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
            yield [d[start:end] for d in all_data]

    def encrypt_vector(self, x):
        return [self.public_key.encrypt(i) for i in x]


def run_homo_lr_host(host_info, arbiter_info=None, task_params={}):
    # host_nodes = role_node_map["host"]
    # arbiter_nodes = role_node_map["arbiter"]

    if len(host_info) != 1:
        logger.error("Hetero LR only support one host party, but current "
                     "task have {} host party.".format(len(host_info)))
        return

    if len(arbiter_info) != 1:
        logger.error("Hetero LR only support one arbiter party, but current "
                     "task have {} arbiter party.".format(len(arbiter_info)))
        return

    host_info = host_info[0]
    arbiter_info = arbiter_info[0]

    # host_port = node_addr_map[host_nodes[0]].split(":")[1]
    host_port = host_info['port']
    proxy_server = ServerChannelProxy(host_port)
    proxy_server.StartRecvLoop()
    logger.debug("Create server proxy for host, port {}.".format(host_port))

    # arbiter_ip, arbiter_port = node_addr_map[arbiter_nodes[0]].split(":")
    arbiter_ip, arbiter_port = arbiter_info['ip'], arbiter_info['port']
    proxy_client_arbiter = ClientChannelProxy(arbiter_ip, arbiter_port,
                                              "arbiter")
    logger.debug("Create client proxy to arbiter,"
                 " ip {}, port {}.".format(arbiter_ip, arbiter_port))

    config = {
        'epochs': 1,
        'lr': 0.05,
        'batch_size': 100,
        'need_encrypt': 'False',
        'category': 2
    }
    # x, label = data_binary(dataset_filepath)
    print("********",)
    data = pd.read_csv(host_info['dataset'], header=0)
    label = data.pop('y').values
    x = data.copy().values

    # x, label = data_iris()
    client_host = Host(x, label, config, proxy_server, proxy_client_arbiter)
    x = LRModel.normalization(x)
    count_train = x.shape[0]
    proxy_client_arbiter.Remote(client_host.need_encrypt, "need_encrypt")
    batch_num_train = (count_train - 1) // config['batch_size'] + 1
    proxy_client_arbiter.Remote(batch_num_train, "batch_num")
    host_data_weight = config['batch_size']
    client_host.need_encrypt = task_params['encrypted']
    if client_host.need_encrypt == 'YES':
        # if task_params['encrypted']:
        client_host.public_key = proxy_server.Get("pub")
        host_data_weight = client_host.encrypt_vector([host_data_weight])

    proxy_client_arbiter.Remote(host_data_weight, "host_data_weight")

    batch_gen_host = client_host.batch_generator([x, label],
                                                 config['batch_size'], False)
    for i in range(config['epochs']):
        logger.info("##### epoch %s ##### " % i)
        for j in range(batch_num_train):
            logger.info("-----epoch=%s, batch=%s-----" % (i, j))
            batch_host_x, batch_host_y = next(batch_gen_host)
            logger.info("batch_host_x.shape:{}".format(batch_host_x.shape))
            logger.info("batch_host_y.shape:{}".format(batch_host_y.shape))
            host_param = client_host.fit(batch_host_x, batch_host_y,
                                         config['category'])
            print("host_param=======: ", host_param)

            proxy_client_arbiter.Remote(host_param, "host_param")
            client_host.model.theta = proxy_server.Get(
                "global_host_model_param")
            logger.info("batch=%s done" % j)
        logger.info("epoch=%i done" % i)
    logger.info("host training process done.")

    proxy_server.StopRecvLoop()
