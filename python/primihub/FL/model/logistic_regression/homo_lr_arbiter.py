import copy
from os import path
from primihub.FL.model.logistic_regression.homo_lr_base import LRModel
import numpy as np
from phe import paillier
from primihub.FL.model.evaluation.evaluation import Classification_eva
import pandas as pd
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


logger = get_logger("Homo_LR_Arbiter")


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
        public_key, private_key = paillier.generate_paillier_keypair(n_length=length)
        self.public_key = public_key
        self.private_key = private_key

    def broadcast_key(self):
        try:
            self.generate_key()
            self.need_encrypt = True
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

    def predict(self, prob):
        return np.array(prob > 0.5, dtype='int')

    def model_aggregate(self, host_parm, guest_param, host_data_weight, guest_data_weight):
        agg_param = np.zeros(len(host_parm))
        param = []
        weight_all = []
        if self.need_encrypt == True:
            param.append(self.decrypt_vector(host_parm))
            weight_all.append(self.decrypt_vector(host_data_weight)[0])
        else:
            param.append(host_parm)
            weight_all.append(host_data_weight)
        param.append(guest_param)
        weight_all.append(guest_data_weight)
        for i in weight_all:
            print(i)
        weight = np.array([weight * 1.0 for weight in weight_all])
        for id_c, p in enumerate(param):
            w = weight[id_c] / np.sum(weight, axis=0)
            for id_d, d in enumerate(p):
                agg_param[id_d] += d * w
        self.theta = agg_param
        return list(self.theta)

    def broadcast_global_model_param(self, host_param, guest_param, host_data_weight, guest_data_weight):
        self.theta = self.model_aggregate(host_param, guest_param, host_data_weight, guest_data_weight)
        # send guest plaintext
        self.proxy_client_guest.Remote(self.theta, "global_guest_model_param")
        # send host ciphertext
        if self.need_encrypt == True:
            self.theta = self.encrypt_vector(self.theta)
            self.proxy_client_host.Remote(self.theta, "global_host_model_param")
        else:
            self.proxy_client_host.Remote(self.theta, "global_host_model_param")

    def evaluation(self, y_true, y_pre_prob):
        res = Classification_eva.get_result(y_true, y_pre_prob)
        return res

    def decrypt_vector(self, x):
        return [self.private_key.decrypt(i) for i in x]

    # def decrypt_matrix(self, x):
    #     ret = []
    #     for r in x:
    #         ret.append(self.decrypt_vector(self.private_key, r))
    #     return ret

    def encrypt_vector(self, x):
        return [self.public_key.encrypt(i) for i in x]

    # def encrypt_matrix(self, x):
    #     ret = []
    #     for r in x:
    #         ret.append(self.encrypt_vector(self.public_key, r))
    #     return ret


def run_homo_lr_arbiter(role_node_map, node_addr_map, params_map={}):
    host_nodes = role_node_map["host"]
    guest_nodes = role_node_map["guest"]
    arbiter_nodes = role_node_map["arbiter"]

    if len(host_nodes) != 1:
        logger.error("Hetero LR only support one host party, but current "
                     "task have {} host party.".format(len(host_nodes)))
        return

    if len(guest_nodes) != 1:
        logger.error("Hetero LR only support one guest party, but current "
                     "task have {} guest party.".format(len(host_nodes)))
        return

    if len(arbiter_nodes) != 1:
        logger.error("Hetero LR only support one arbiter party, but current "
                     "task have {} arbiter party.".format(len(host_nodes)))
        return

    arbiter_port = node_addr_map[arbiter_nodes[0]].split(":")[1]
    proxy_server = ServerChannelProxy(arbiter_port)
    proxy_server.StartRecvLoop()
    logger.debug("Create server proxy for arbiter, port {}.".format(arbiter_port))

    host_ip, host_port = node_addr_map[host_nodes[0]].split(":")
    proxy_client_host = ClientChannelProxy(host_ip, host_port, "host")
    logger.debug("Create client proxy to host,"
                 " ip {}, port {}.".format(host_ip, host_port))

    guest_ip, guest_port = node_addr_map[guest_nodes[0]].split(":")
    proxy_client_guest = ClientChannelProxy(guest_ip, guest_port, "guest")
    logger.debug("Create client proxy to guest,"
                 " ip {}, port {}.".format(guest_ip, guest_port))

    config = {
        'epochs': 1,
        'batch_size': 500
    }
    client_arbiter = Arbiter(proxy_server, proxy_client_host, proxy_client_guest)
    need_encrypt = proxy_server.Get("need_encrypt")

    if need_encrypt == True:
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
            client_arbiter.broadcast_global_model_param(host_param, guest_param, host_data_weight, guest_data_weight)
            logger.info("batch={} done".format(j))
        logger.info("epoch={} done".format(i))

    logger.info("####### start predict ######")
    X, y = data_process()
    X = LRModel.normalization(X)
    y = list(y)
    prob = client_arbiter.predict_prob(X)
    logger.info('Classification probability is:')
    logger.info(prob)
    predict = list(client_arbiter.predict(prob))
    logger.info('Classification result is:')
    logger.info(predict)
    count = 0
    for i in range(len(y)):
        if y[i] == predict[i]:
            count += 1
    logger.info('acc is: %s' % (count / (len(y))))
    logger.info("All process done.")
    proxy_server.StopRecvLoop()
