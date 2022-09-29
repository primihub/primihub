import primihub as ph
import logging
from primihub import dataset, context
# from homo_lr_host import run_homo_lr_host
# from homo_lr_guest import run_homo_lr_guest
# from homo_lr_arbiter import run_homo_lr_arbiter
from os import path
import json
import os
from phe import paillier
import pickle
from primihub.FL.model.logistic_regression.vfl.evaluation_lr import evaluator

from primihub.FL.model.logistic_regression.homo_lr_base import LRModel
import numpy as np
import pandas as pd
import copy
from primihub.FL.proxy.proxy import ServerChannelProxy
from primihub.FL.proxy.proxy import ClientChannelProxy
from os import path
import logging
from sklearn.datasets import load_iris


def get_logger(name):
    LOG_FORMAT = "[%(asctime)s][%(filename)s:%(lineno)d][%(levelname)s] %(message)s"
    DATE_FORMAT = "%m/%d/%Y %H:%M:%S %p"
    logging.basicConfig(level=logging.DEBUG,
                        format=LOG_FORMAT,
                        datefmt=DATE_FORMAT)
    logger = logging.getLogger(name)
    return logger


logger = get_logger("Homo-LR-Host")


dataset.define("breast_0")
dataset.define("breast_1")
dataset.define("breast_2")


class Arbiter:
    """
    Tips: Arbiter is a trusted third party !!!
    """

    def __init__(self, proxy_server, proxy_client_host, proxy_client_guest):
        self.need_one_vs_rest = None
        self.public_key = None
        self.private_key = None
        self.need_encrypt = None
        self.epoch = None
        self.weight_host = None
        self.weight_guest = None
        self.theta = None
        self.proxy_server = proxy_server
        self.proxy_client_host = proxy_client_host
        self.proxy_client_guest = proxy_client_guest

    def sigmoid(self, x):
        x = np.array(x, dtype=np.float64)
        y = 1.0 / (1.0 + np.exp(-x))
        return y

    def generate_key(self, length=1024):
        public_key, private_key = paillier.generate_paillier_keypair(
            n_length=length)
        self.public_key = public_key
        self.private_key = private_key

    def broadcast_key(self):
        try:
            self.generate_key()
            logger.info("start send pub")
            self.proxy_client_host.Remote(self.public_key, "pub")
            logger.info("send pub to host OK")
        except Exception as e:
            logger.info("Arbiter broadcast key pair error : %s" % e)

    def predict_prob(self, data):
        if self.need_encrypt:
            global_theta = self.decrypt_vector(self.theta)
            data = np.hstack([np.ones((len(data), 1)), data])
            prob = self.sigmoid(data.dot(global_theta))
            return prob
        else:
            data = np.hstack([np.ones((len(data), 1)), data])
            prob = self.sigmoid(data.dot(self.theta))
            return prob

    def predict_binary(self, prob):
        return np.array(prob > 0.5, dtype='int')

    def predict_one_vs_rest(self, data):
        data = np.hstack([np.ones((len(data), 1)), data])
        if self.need_encrypt == 'YES':
            global_theta = self.decrypt_matrix(self.theta)
            global_theta = np.array(global_theta)
        else:
            global_theta = np.array(self.theta)
        pre = self.sigmoid(data.dot(global_theta.T))
        y_argmax = np.argmax(pre, axis=1)
        return y_argmax

    def predict(self, data, category):
        if category == 2:
            return self.predict_binary(self.predict_prob(data))
        else:
            return self.predict_one_vs_rest(data)

    def evaluation(self, y, y_hat):
        return evaluator.getAccuracy(y, y_hat)

    def model_aggregate(self, host_parm, guest_param, host_data_weight,
                        guest_data_weight):
        param = []
        weight_all = []
        if self.need_encrypt == 'YES':
            if self.need_one_vs_rest == False:
                param.append(self.decrypt_vector(host_parm))
            else:
                param.append(self.decrypt_matrix(host_parm))
            weight_all.append(self.decrypt_vector(host_data_weight)[0])
        else:
            param.append(host_parm)
            weight_all.append(host_data_weight)
        param.append(guest_param)
        weight_all.append(guest_data_weight)
        weight = np.array([weight * 1.0 for weight in weight_all])
        if self.need_one_vs_rest == True:
            agg_param = np.zeros_like(np.array(param))
            for id_c, p in enumerate(param):
                w = weight[id_c] / np.sum(weight, axis=0)
                for id_d, d in enumerate(p):
                    d = np.array(d)
                    agg_param[id_c, id_d] += d * w
            self.theta = np.sum(agg_param, axis=0)
            return list(self.theta)
        else:
            agg_param = np.zeros(len(host_parm))
            for id_c, p in enumerate(param):
                w = weight[id_c] / np.sum(weight, axis=0)
                for id_d, d in enumerate(p):
                    agg_param[id_d] += d * w
            self.theta = agg_param
            return list(self.theta)

    def broadcast_global_model_param(self, host_param, guest_param,
                                     host_data_weight, guest_data_weight):
        self.theta = self.model_aggregate(host_param, guest_param,
                                          host_data_weight, guest_data_weight)
        # send guest plaintext
        self.proxy_client_guest.Remote(self.theta, "global_guest_model_param")
        # send host ciphertext
        if self.need_encrypt == 'YES':
            if self.need_one_vs_rest == False:
                self.theta = self.encrypt_vector(self.theta)
            else:
                self.theta = self.encrypt_matrix(self.theta)

            self.proxy_client_host.Remote(
                self.theta, "global_host_model_param")
        else:
            self.proxy_client_host.Remote(
                self.theta, "global_host_model_param")

    def decrypt_vector(self, x):
        return [self.private_key.decrypt(i) for i in x]

    def decrypt_matrix(self, x):
        ret = []
        for r in x:
            ret.append(self.decrypt_vector(r))
        return ret

    def encrypt_vector(self, x):
        return [self.public_key.encrypt(i) for i in x]

    def encrypt_matrix(self, x):
        ret = []
        for r in x:
            ret.append(self.encrypt_vector(r))
        return ret


def run_homo_lr_arbiter(role_node_map, node_addr_map, host_info, task_params={}):
    host_nodes = role_node_map["host"]
    guest_nodes = role_node_map["guest"]
    arbiter_nodes = role_node_map["arbiter"]
    eva_type = ph.context.Context.params_map.get("taskType", None)

    if len(host_nodes) != 1:
        logger.error("Hetero LR only support one host party, but current "
                     "task have {} host party.".format(len(host_info)))
        return

    if len(guest_nodes) != 1:
        logger.error("Hetero LR only support one guest party, but current "
                     "task have {} guest party.".format(len(guest_info)))
        return

    if len(arbiter_nodes) != 1:
        logger.error("Hetero LR only support one arbiter party, but current "
                     "task have {} arbiter party.".format(len(arbiter_info)))
        return

    # host_info = host_info[0]
    # guest_info = guest_info[0]
    # arbiter_info = arbiter_info[0]
    # arbiter_port = node_addr_map[arbiter_nodes[0]].split(":")[1]
    # arbiter_port = arbiter_info['port']
    arbiter_port = node_addr_map[arbiter_nodes[0]].split(":")[1]
    proxy_server = ServerChannelProxy(arbiter_port)
    proxy_server.StartRecvLoop()
    logger.debug(
        "Create server proxy for arbiter, port {}.".format(arbiter_port))

    # host_ip, host_port = node_addr_map[host_nodes[0]].split(":")
    # host_ip, host_port = host_info['ip'], host_info['port']
    host_ip, host_port = node_addr_map[host_nodes[0]].split(":")
    proxy_client_host = ClientChannelProxy(host_ip, host_port, "host")
    logger.debug("Create client proxy to host,"
                 " ip {}, port {}.".format(host_ip, host_port))

    # guest_ip, guest_port = node_addr_map[guest_nodes[0]].split(":")
    # guest_ip, guest_port = guest_info['ip'], guest_info['port']
    guest_ip, guest_port = node_addr_map[guest_nodes[0]].split(":")

    proxy_client_guest = ClientChannelProxy(guest_ip, guest_port, "guest")
    logger.debug("Create client proxy to guest,"
                 " ip {}, port {}.".format(guest_ip, guest_port))

    config = {
        'epochs': 1,
        'batch_size': 100,
        'need_one_vs_rest': False,
        'category': 2
    }
    client_arbiter = Arbiter(proxy_server, proxy_client_host,
                             proxy_client_guest)
    client_arbiter.need_one_vs_rest = config['need_one_vs_rest']
    need_encrypt = proxy_server.Get("need_encrypt")

    if need_encrypt == 'YES':
        client_arbiter.need_encrypt = 'YES'
        client_arbiter.broadcast_key()

    batch_num = proxy_server.Get("batch_num")
    host_data_weight = proxy_server.Get("host_data_weight")
    guest_data_weight = proxy_server.Get("guest_data_weight")

    for i in range(config['epochs']):
        logger.info("##### epoch %s ##### " % i)
        for j in range(batch_num):
            logger.info("-----epoch={}, batch={}-----".format(i, j))
            host_param = proxy_server.Get("host_param")
            guest_param = proxy_server.Get("guest_param")
            client_arbiter.broadcast_global_model_param(host_param, guest_param,
                                                        host_data_weight,
                                                        guest_data_weight)
            logger.info("batch={} done".format(j))
        logger.info("epoch={} done".format(i))

    logger.info("####### start predict ######")
    # X, y = iris_data()
    # X, y = data_binary(dataset_filepath)
    data = pd.read_csv(guest_info['dataset'], header=0)
    label = data.pop('y').values
    x = data.copy().values
    # X = LRModel.normalization(X)
    pre = client_arbiter.predict(x, config['category'])
    logger.info('Classification result is:')
    # acc = np.mean(y == pre)
    acc = client_arbiter.evaluation(label, pre)
    predict_file_path = ph.context.Context.get_predict_file_path()
    with open(predict_file_path, 'wb') as evf:
        pickle.dump({'acc': acc}, evf)
    logger.info('acc is: %s' % acc)
    logger.info("All process done.")
    proxy_server.StopRecvLoop()


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
            encrypted_grad = batch_encrypted_grad.sum(
                axis=1) / batch_y.shape[0]
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


def run_homo_lr_host(role_node_map, node_addr_map, task_params={}):
    host_nodes = role_node_map["host"]
    arbiter_nodes = role_node_map["arbiter"]
    # eva_type = ph.context.Context.params_map.get("taskType", None)

    if len(host_nodes) != 1:
        logger.error("Homo LR only support one host party, but current "
                     "task have {} host party.".format(len(host_nodes)))
        return

    if len(arbiter_nodes) != 1:
        logger.error("Homo LR only support one arbiter party, but current "
                     "task have {} arbiter party.".format(len(arbiter_nodes)))
        return

    # host_info = host_info[0]
    host_port = node_addr_map[host_nodes[0]].split(":")[1]

    # arbiter_info = arbiter_info[0]
    arbiter_ip, arbiter_port = node_addr_map[arbiter_nodes[0]].split(":")

    # host_port = node_addr_map[host_nodes[0]].split(":")[1]
    # host_port = host_info['port']
    proxy_server = ServerChannelProxy(host_port)
    proxy_server.StartRecvLoop()
    logger.debug("Create server proxy for host, port {}.".format(host_port))

    # arbiter_ip, arbiter_port = node_addr_map[arbiter_nodes[0]].split(":")
    # arbiter_ip, arbiter_port = arbiter_info['ip'], arbiter_info['port']
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

    data = ph.dataset.read(dataset_key="breast_1").df_data

    # label = data.pop('y').values
    label = data.iloc[:, -1].values
    # x = data.copy().values
    x = data.iloc[:, 0:-1].values

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

            proxy_client_arbiter.Remote(host_param, "host_param")
            client_host.model.theta = proxy_server.Get(
                "global_host_model_param")
            logger.info("batch=%s done" % j)
        logger.info("epoch=%i done" % i)
    logger.info("host training process done.")
    model_file_path = ph.context.Context.get_model_file_path()
    logger.info("Current model file path is:",
                model_file_path, ph.context.Context.params_map)
    with open(model_file_path, 'wb') as fm:
        pickle.dump(client_host.model.theta, fm)

    proxy_server.StopRecvLoop()


class Guest:

    def __init__(self, X, y, config, proxy_server, proxy_client_arbiter):
        self.X = X
        self.y = y
        self.config = config
        self.lr = self.config['lr']
        self.model = LRModel(X, y, self.config['category'])
        self.need_one_vs_rest = None
        self.need_encrypt = False
        self.batch_size = self.config['batch_size']
        self.proxy_server = proxy_server
        self.proxy_client_arbiter = proxy_client_arbiter

    def predict(self, data=None):
        if self.need_one_vs_rest:
            pass
        else:
            pre = self.model.predict(data)
        return pre

    def fit_binary(self, X, y, theta=None):
        if self.need_one_vs_rest == False:
            theta = self.model.theta
        self.model.theta = self.model.fit(X, y, theta, eta=self.lr)
        self.model.theta = list(self.model.theta)
        return self.model.theta

    def one_vs_rest(self, X, y, k):
        all_theta = []
        for i in range(0, k):
            y_i = np.array([1 if label == i else 0 for label in y])
            theta = self.fit_binary(X, y_i, self.model.one_vs_rest_theta[i])
            all_theta.append(list(theta))
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


def run_homo_lr_guest(role_node_map, node_addr_map, task_params={}):
    guest_nodes = role_node_map["guest"]
    arbiter_nodes = role_node_map["arbiter"]

    if len(guest_nodes) != 1:
        logger.error("Homo LR only support one guest party, but current "
                     "task have {} guest party.".format(len(guest_nodes)))
        return

    if len(arbiter_nodes) != 1:
        logger.error("Homo LR only support one arbiter party, but current "
                     "task have {} arbiter party.".format(len(arbiter_nodes)))
        return

    # guest_info = guest_info[0]
    # arbiter_info = arbiter_info[0]

    # # guest_port = node_addr_map[guest_nodes[0]].split(":")[1]
    # guest_port = guest_info['port']
    guest_port = node_addr_map[guest_nodes[0]].split(":")[1]
    proxy_server = ServerChannelProxy(guest_port)
    proxy_server.StartRecvLoop()
    logger.debug("Create server proxy for guest, port {}.".format(guest_port))

    # arbiter_ip, arbiter_port = node_addr_map[arbiter_nodes[0]].split(":")
    # arbiter_ip, arbiter_port = arbiter_info['ip'], arbiter_info['port']
    arbiter_ip, arbiter_port = node_addr_map[arbiter_nodes[0]].split(":")

    proxy_client_arbiter = ClientChannelProxy(arbiter_ip, arbiter_port,
                                              "arbiter")
    logger.debug("Create client proxy to arbiter,"
                 " ip {}, port {}.".format(arbiter_ip, arbiter_port))

    config = {'epochs': 1, 'lr': 0.05, 'batch_size': 100, 'category': 2}

    # x, label = data_binary(dataset_filepath)
    # data = pd.read_csv(guest_info['dataset'], header=0)
    # x, label = data_iris()
    # data = pd.read_csv(guest_info['dataset'], header=0)
    data = ph.dataset.read(dataset_key="breast_2").df_data

    # label = data.pop('y').values
    label = data.iloc[:, -1].values
    # x = data.copy().values
    x = data.iloc[:, 0:-1].values

    count_train = x.shape[0]
    batch_num_train = (count_train - 1) // config['batch_size'] + 1

    guest_data_weight = config['batch_size']
    proxy_client_arbiter.Remote(guest_data_weight, "guest_data_weight")
    client_guest = Guest(x, label, config, proxy_server, proxy_client_arbiter)

    batch_gen_guest = client_guest.batch_generator([x, label],
                                                   config['batch_size'], False)
    # batch_gen_host = client_guest.iterate_minibatches(x, label, config['batch_size'], False)

    for i in range(config['epochs']):
        logger.info("##### epoch %s ##### " % i)
        for j in range(batch_num_train):
            logger.info("-----epoch=%s, batch=%s-----" % (i, j))
            batch_x, batch_y = next(batch_gen_guest)
            logger.info("batch_host_x.shape:{}".format(batch_x.shape))
            logger.info("batch_host_y.shape:{}".format(batch_y.shape))
            guest_param = client_guest.fit(
                batch_x, batch_y, config['category'])
            proxy_client_arbiter.Remote(guest_param, "guest_param")
            client_guest.model.theta = proxy_server.Get(
                "global_guest_model_param")
            logger.info("batch=%s done" % j)
        logger.info("epoch=%i done" % i)
    logger.info("guest training process done.")

    proxy_server.StopRecvLoop()

# path = path.join(path.dirname(__file__), '../../../tests/data/wisconsin.data')


def load_info():
    # basedir = os.path.abspath(os.path.dirname(__file__))
    # config_f = open(os.path.join(basedir, 'homo_lr_config.json'), 'r')
    config_f = open(
        './python/primihub/FL/model/logistic_regression/homo_lr_config.json',
        'r')
    lr_config = json.load(config_f)
    print(lr_config)
    task_type = lr_config['task_type']
    task_params = lr_config['task_params']
    node_info = lr_config['node_info']
    arbiter_info = {}
    guest_info = {}
    host_info = {}
    for tmp_node, tmp_val in node_info.items():

        if tmp_node == 'Arbiter':
            arbiter_info = tmp_val

        elif tmp_node == 'Guest':
            guest_info = tmp_val

        elif tmp_node == 'Host':
            host_info = tmp_val

    return arbiter_info, guest_info, host_info, task_type, task_params


def get_logger(name):
    LOG_FORMAT = "[%(asctime)s][%(filename)s:%(lineno)d][%(levelname)s] %(message)s"
    DATE_FORMAT = "%m/%d/%Y %H:%M:%S %p"
    logging.basicConfig(level=logging.DEBUG,
                        format=LOG_FORMAT,
                        datefmt=DATE_FORMAT)
    logger = logging.getLogger(name)
    return logger


arbiter_info, guest_info, host_info, task_type, task_params = load_info()

logger = get_logger(task_type)


# @ph.context.function(role='host', protocol='lr', datasets=['breast_1'], port='8020', task_type="regression")
# def run_host_party():
#     logger.info("Start homo-LR host logic.")

#     run_homo_lr_host(host_info, arbiter_info, task_params)

#     logger.info("Finish homo-LR host logic.")


# # @ph.context.function(
# #     role=guest_info[0]['role'],
# #     protocol=task_type,
# #     #  datasets=host_info[0]['dataset'],
# #     datasets=["guest_dataset"],
# #     port=str(guest_info[0]['port']))
# @ph.context.function(role='guest', protocol='lr', datasets=['breast_0'], port='8010', task_type="regression")
# def run_guest_party():
#     logger.info("Start homo-LR guest logic.")

#     run_homo_lr_guest(guest_info, arbiter_info, task_params)

#     logger.info("Finish homo-LR guest logic.")


# @ph.context.function(role='arbiter', protocol='lr', datasets=['breast_2'], port='8030', task_type="regression")
# def run_arbiter_party():

#     run_homo_lr_arbiter(arbiter_info, guest_info, host_info, task_params)

#     logger.info("Finish homo-LR arbiter logic.")

# role_nodeid_map = ph.context.Context.role_nodeid_map
# node_addr_map = ph.context.Context.node_addr_map


@ph.context.function(role='arbiter', protocol='lr', datasets=['breast_0'], port='8010', task_type="regression")
def run_arbiter_party():
    role_node_map = ph.context.Context.get_role_node_map()
    node_addr_map = ph.context.Context.get_node_addr_map()
    logger.debug(
        "role_nodeid_map {}".format(role_node_map))

    logger.debug(
        "node_addr_map {}".format(node_addr_map))

    run_homo_lr_arbiter(role_node_map, node_addr_map, host_info, task_params)

    logger.info("Finish homo-LR arbiter logic.")


@ph.context.function(role='host', protocol='lr', datasets=['breast_1'], port='8020', task_type="regression")
def run_host_party():
    role_node_map = ph.context.Context.get_role_node_map()
    node_addr_map = ph.context.Context.get_node_addr_map()
    logger.debug(
        "role_nodeid_map {}".format(role_node_map))

    logger.debug(
        "node_addr_map {}".format(node_addr_map))
    logger.info("Start homo-LR host logic.")

    run_homo_lr_host(role_node_map, node_addr_map, task_params)

    logger.info("Finish homo-LR host logic.")


@ph.context.function(role='guest', protocol='lr', datasets=['breast_2'], port='8030', task_type="regression")
def run_guest_party():
    role_node_map = ph.context.Context.get_role_node_map()
    node_addr_map = ph.context.Context.get_node_addr_map()
    logger.debug(
        "role_nodeid_map {}".format(role_node_map))

    logger.debug(
        "node_addr_map {}".format(node_addr_map))
    logger.info("Start homo-LR guest logic.")

    run_homo_lr_guest(role_node_map, node_addr_map, task_params)

    logger.info("Finish homo-LR guest logic.")
