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

proxy_server_guest = ServerChannelProxy("10094")  # arbiter接收guest消息
proxy_server_host = ServerChannelProxy("10092")  # arbiter接收host消息
proxy_client_guest = ClientChannelProxy(
    "127.0.0.1", "10090")  # arbiter发送消息给guest
proxy_client_host = ClientChannelProxy(
    "127.0.0.1", "10091")  # arbiter发送消息给host

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

    def __init__(self):
        self.need_one_vs_rest = None
        self.public_key = None
        self.private_key = None
        self.need_encrypt = True
        self.epoch = None
        self.weight_host = None
        self.weight_guest = None
        self.theta = None

    def sigmoid(self, x):
        x = np.array(x, dtype=np.float64)
        y = 1.0 / (1.0 + np.exp(-x))
        return y

    def generate_key(self, length=1024):
        public_key, private_key = paillier.generate_paillier_keypair(n_length=length)
        self.public_key = public_key
        self.private_key = private_key
        return public_key, private_key

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

    def aggregate(self, client_param):
        self.global_model_param = self.server_aggregate(client_param)
        return self.global_model_param

    def broadcast_key(self):
        try:
            self.public_key, self.private_key = self.generate_key()
            data_to_host = {"public_key": self.public_key}
            logger.info("start send pub")
            proxy_client_host.Remote(data_to_host, "pub")
            logger.info("send pub to host OK")
        except Exception as e:
            logger.info("Arbiter broadcast key pair error : %s" % e)

    def model_aggregate(self, host_parm, guest_param, host_data_weight, guest_data_weight):
        agg_param = np.zeros(len(host_parm))
        param = []
        weight_all = []
        param.append(self.decrypt_vector(host_parm))
        param.append(guest_param)
        weight_all.append(self.decrypt_vector(host_data_weight)[0])
        weight_all.append(guest_data_weight)

        weight = np.array([weight * 1.0 for weight in weight_all])
        for id_c, p in enumerate(param):
            w = weight[id_c] / np.sum(weight, axis=0)
            for id_d, d in enumerate(p):
                agg_param[id_d] += d * w
        self.theta = agg_param
        return list(self.theta)

    def server_aggregate(self, *client_param):  # plaintext
        sum_weight = np.zeros(client_param[0][0].shape)
        print('sum_weight.shape---->', sum_weight.shape)
        sum_bias = np.float64("0")
        for param in client_param:
            sum_weight = sum_weight + param[0]
            sum_bias = sum_bias + param[1]
        print(f"sum average is {sum_weight}")
        avg_weight = sum_weight / len(client_param)
        avg_bias = sum_bias / len(client_param)
        return avg_weight, avg_bias

    def broadcast_global_model_param(self, host_param, guest_param, host_data_weight, guest_data_weight):
        self.theta = self.model_aggregate(host_param, guest_param, host_data_weight, guest_data_weight)
        # send guest plaintext
        proxy_client_guest.Remote(self.theta, "global_guest_model_param")
        # send host ciphertext
        self.theta = self.encrypt_vector(self.theta)
        proxy_client_host.Remote(self.theta, "global_host_model_param")

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


if __name__ == "__main__":
    conf = {'epoch': 3}
    proxy_server_host.StartRecvLoop()
    need_encrypt = proxy_server_host.Get("need_encrypt")
    client_arbiter = Arbiter()
    if need_encrypt == True:
        public_key, private_key = client_arbiter.generate_key()
        proxy_client_host.Remote(public_key, "public_key")
        logger.info("send public key done!")
    proxy_server_guest.StartRecvLoop()

    batch_num = proxy_server_host.Get("batch_num")
    host_data_weight = proxy_server_host.Get("host_data_weight")
    guest_data_weight = proxy_server_guest.Get("guest_data_weight")

    for i in range(conf['epoch']):
        logger.info("######## epoch %s ######### start " % i)
        for j in range(batch_num):
            host_param = proxy_server_host.Get("host_param")
            guest_param = proxy_server_guest.Get("guest_param")
            client_arbiter.broadcast_global_model_param(host_param, guest_param, host_data_weight, guest_data_weight)
        logger.info("######### epoch %s ######### done " % i)
    logger.info("All process done.")

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

    proxy_server_guest.StopRecvLoop()
    proxy_server_host.StopRecvLoop()
