from primihub.primitive.opt_paillier_c2py_warpper import *
import numpy as np
import time
from phe import paillier
from os import path
from primihub.FL.proxy.proxy import ServerChannelProxy
from primihub.FL.proxy.proxy import ClientChannelProxy

# dir = path.join(path.dirname(__file__), '../../../../tests/data')
# data_guest = np.loadtxt(
#     path.join(dir, "wisconsin_guest.data"), str, delimiter=',')
# data_host = np.loadtxt(
#     path.join(dir, "wisconsin_host.data"), str, delimiter=',')
# data_test = np.loadtxt(
#     path.join(dir, "wisconsin_test.data"), str, delimiter=',')

import logging


def get_logger(name):
    LOG_FORMAT = "[%(asctime)s][%(filename)s:%(lineno)d][%(levelname)s] %(message)s"
    DATE_FORMAT = "%m/%d/%Y %H:%M:%S %p"
    logging.basicConfig(level=logging.DEBUG,
                        format=LOG_FORMAT, datefmt=DATE_FORMAT)
    logger = logging.getLogger(name)
    return logger


logger = get_logger("hetero_LR_guest")


class Guest:
    def __init__(self, x, config, proxy_server, proxy_client_host, proxy_client_arbiter):
        self.x = x
        self.weights = np.zeros(x.shape[1])
        self.mask = 0
        self.data = {}
        self.config = config
        self.proxy_server = proxy_server
        self.proxy_client_arbiter = proxy_client_arbiter
        self.proxy_client_host = proxy_client_host

    def compute_z_guest(self, we, x):
        z_guest = np.dot(x, we)
        return z_guest

    def compute_encrypted_dJ_guest(self, batch_x, encrypted_u):
        encrypted_dJ_guest = batch_x.T.dot(
            encrypted_u) + self.config['lambda'] * self.weights
        return encrypted_dJ_guest

    def update_weight(self, dJ_guest, batch_x):
        self.weights = self.weights - \
            self.config['lr'] * dJ_guest / len(batch_x)

    '''
    guest Send part of encrypted gradient and loss  to host
    '''

    def cal_uz(self, batch_x):
        self.data.update(self.proxy_server.Get("pub", 10000))
        dt = self.data
        assert "public_key" in dt.keys(
        ), "Error: 'public_key' from arbiter not successfully received."
        public_key = dt['public_key']
        z_guest = self.compute_z_guest(self.weights, batch_x)
        u_guest = 0.25 * z_guest
        z_guest_square = z_guest ** 2
        encrypted_u_guest = []
        encrypted_z_guest_square = []
        for x in u_guest:
            encrypted_u_guest.append(public_key.encrypt(x))
        for y in z_guest_square:
            encrypted_z_guest_square.append(public_key.encrypt(y))
        encrypted_u_guest = np.asarray(encrypted_u_guest)
        encrypted_z_guest_square = np.asarray(encrypted_z_guest_square)
        dt.update({"encrypted_u_guest": encrypted_u_guest})
        data_to_host = {"encrypted_u_guest": encrypted_u_guest,
                        "encrypted_z_guest_square": encrypted_z_guest_square}
        self.proxy_client_host.Remote(data_to_host, "u_z")

    '''
    1.guest receive the partial gradient from the host
    2.add it to own gradient
    3.multiply it by  own X to get the full gradient
    4.plus the mask
    5.send the encrypted loss to the arbiter party to decrypt it
    '''

    def cal_dJ(self, batch_x):
        self.data.update(self.proxy_server.Get("encrypted_u_host", 10000))
        dt = self.data
        assert "encrypted_u_host" in dt.keys(
        ), "Error: 'encrypt_u_host' from host not successfully received."
        encrypted_u_host = dt['encrypted_u_host']
        encrypted_u = encrypted_u_host + dt["encrypted_u_guest"]
        # encrypted_u = []
        # for x, y in zip(encrypted_u_host, dt["encrypted_u_guest"]):
        #     encrypted_u = np.append(
        #         encrypted_u, opt_paillier_add(
        #             dt['public_key'], x, y))
        encrypted_dJ_guest = self.compute_encrypted_dJ_guest(
            batch_x, encrypted_u)
        self.mask = np.random.rand(len(encrypted_dJ_guest))
        encrypted_masked_dJ_guest = encrypted_dJ_guest + self.mask
        data_to_arbiter = {
            'encrypted_masked_dJ_guest': encrypted_masked_dJ_guest}
        self.proxy_client_arbiter.Remote(
            data_to_arbiter, "encrypted_masked_dJ_guest")

    '''
    guest receive arbiter's decrypted gradient minus its own mask
    '''

    def update(self, batch_x):
        self.data.update(self.proxy_server.Get("masked_dJ_guest", 10000))
        dt = self.data
        assert "masked_dJ_guest" in dt.keys(
        ), "Error: 'masked_dJ_guest' from arbiter  not successfully received."
        masked_dJ_guest = dt['masked_dJ_guest']
        dJ_guest = masked_dJ_guest - self.mask
        self.update_weight(dJ_guest, batch_x)
        logger.info(f"guest weight: {self.weights}")
        return self.weights

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

    def predict(self, w_test, x_test):
        self.data.update(self.proxy_server.Get("pub", 10000))
        dt = self.data
        assert "public_key" in dt.keys(
        ), "Error: 'public_key' from arbiter not successfully received when testing."
        public_key = dt['public_key']
        z_guest_test = self.compute_z_guest(w_test, x_test)
        encrypted_z_guest_test = []
        for x in z_guest_test:
            encrypted_z_guest_test.append(public_key.encrypt(x))
        encrypted_z_guest_test = np.asarray(encrypted_z_guest_test)
        data_to_host = {"encrypted_z_guest_test": encrypted_z_guest_test}
        self.proxy_client_host.Remote(data_to_host, "encrypted_z_guest_test")


def run_hetero_lr_guest(role_node_map, node_addr_map, params_map={}):
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

    guest_port = node_addr_map[guest_nodes[0]].split(":")[1]
    proxy_server = ServerChannelProxy(guest_port)
    proxy_server.StartRecvLoop()
    logger.debug("Create server proxy for guest, port {}.".format(guest_port))

    host_ip, host_port = node_addr_map[host_nodes[0]].split(":")
    proxy_client_arbiter = ClientChannelProxy(host_ip, host_port)
    logger.debug("Create client proxy to host,"
                 " ip {}, port {}.".format(host_ip, host_port))

    arbiter_ip, arbiter_port = node_addr_map[arbiter_nodes[0]].split(":")
    proxy_client_host = ClientChannelProxy(arbiter_ip, arbiter_port)
    logger.debug("Create client proxy to arbiter,"
                 " ip {}, port {}.".format(arbiter_ip, arbiter_port))

    config = {
        'epochs': 1,
        'lambda': 10,
        'threshold': 0.5,
        'lr': 0.05,
        'batch_size': 200
    }
    
    # TODO: File path shouldn't a fixed path.
    data_guest = np.loadtxt("/tmp/wisconsin_guest.data", str, delimiter=',')

    x = data_guest[1:, :]
    x = x.astype(np.float)
    count = x.shape[0]
    batch_num = count // config['batch_size'] + 1

    client_guest = Guest(x, config, proxy_server,
                         proxy_client_host, proxy_client_arbiter)

    batch_gen_guest = client_guest.batch_generator(
        [x], config['batch_size'], False)

    for i in range(config['epochs']):
        logger.info("##### epoch %s ##### " % i)
        for j in range(batch_num):
            logger.info("-----epoch=%s, batch=%s-----" % (i, j))
            batch_guest_x = next(batch_gen_guest)[0]
            logger.info("batch_guest_x.shape: {}".format(batch_guest_x.shape))
            client_guest.cal_uz(batch_guest_x)
            client_guest.cal_dJ(batch_guest_x)
            client_guest.update(batch_guest_x)
            logger.info("batch=%s done" % j)
        logger.info("epoch=%i done" % i)
    logger.info("guest training process done.")

    # load test data
    x_test = data_test[1:, 1:3]
    x_test = x_test.astype(float)
    logger.info("x_test.shape: {}".format(x_test.shape))
    logger.info(x_test[0])
    client_guest.predict(client_guest.weights, x_test)

    proxy_server.StopRecvLoop()
