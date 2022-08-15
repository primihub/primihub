import numpy as np
from sklearn import metrics
from os import path
from primihub.FL.model.logistic_regression.vfl.evaluation_lr import classification_eva
from primihub.FL.proxy.proxy import ServerChannelProxy
from primihub.FL.proxy.proxy import ClientChannelProxy
import logging


def get_logger(name):
    LOG_FORMAT = "[%(asctime)s][%(filename)s:%(lineno)d][%(levelname)s] %(message)s"
    DATE_FORMAT = "%m/%d/%Y %H:%M:%S %p"
    logging.basicConfig(level=logging.DEBUG,
                        format=LOG_FORMAT, datefmt=DATE_FORMAT)
    logger = logging.getLogger(name)
    return logger


logger = get_logger("hetero_LR_host")


class Host:
    def __init__(self, x, y, config, proxy_server, proxy_client_guest, proxy_client_arbiter):
        self.x = x
        self.y = y
        self.weights = np.zeros(x.shape[1])
        self.mask = 0
        self.data = {}
        self.config = config
        self.proxy_server = proxy_server
        self.proxy_client_guest = proxy_client_guest
        self.proxy_client_arbiter = proxy_client_arbiter

    @staticmethod
    def compute_z_host(we, x):
        z_host = np.dot(x, we)
        return z_host

    def compute_u_host(self, batch_x, batch_y):
        z_host = Host.compute_z_host(self.weights, batch_x)
        u_host = 0.25 * z_host - batch_y * 0.5
        return u_host

    def compute_encrypted_dJ_host(self, batch_x, encrypted_u):
        encrypted_dJ_host = batch_x.T.dot(
            encrypted_u) + self.config['lambda'] * self.weights
        # encrypted_dJ_host = []
        # for dim in batch_x.T:
        #     multiply_dim_u = opt_paillier_cons_mul(self.data['public_key'], encrypted_u[0], int(dim[0]*(2**20)))
        #     for i in range(1,len(dim)):
        #         u_element = opt_paillier_cons_mul(self.data['public_key'], encrypted_u[i], int(dim[i]*(2**20)))
        #         multiply_dim_u = opt_paillier_add(self.data['public_key'], u_element, multiply_dim_u)
        #     # multiply_dim_u = + self.config['lambda'] * self.weights
        #     encrypted_dJ_host.append(multiply_dim_u)
        return encrypted_dJ_host

    def update_weight(self, dJ_host, batch_x):
        self.weights = self.weights - \
            self.config['lr'] * dJ_host / len(batch_x)

    '''
    host Send part of encrypted gradient and loss  to host
    '''

    def cal_u(self, batch_x, batch_y):
        self.data.update(self.proxy_server.Get("pub", 10000))
        dt = self.data
        assert "public_key" in dt.keys(
        ), "Error: 'public_key' from arbiter not successfully received."
        public_key = dt['public_key']
        z_host = Host.compute_z_host(self.weights, batch_x)
        u_host = self.compute_u_host(batch_x, batch_y)
        encrypted_u_host = []
        for x in u_host:
            encrypted_u_host.append(public_key.encrypt(x))
        encrypted_u_host = np.asarray(encrypted_u_host)
        dt.update({"encrypted_u_host": encrypted_u_host})
        dt.update({"z_host": z_host})
        data_to_guest = {"encrypted_u_host": encrypted_u_host}
        # self.channel.send(data_to_guest)
        self.proxy_client_guest.Remote(data_to_guest, "encrypted_u_host")

    '''
        1.host receive the partial gradient from the guest
        2.add it to own gradient
        3.multiply it by  own X to get the full gradient
        4.plus the mask
        5.send the encrypted loss to the arbiter party to decrypt it
    '''

    def cal_dJ_loss(self, batch_x, batch_y):
        self.data.update(self.proxy_server.Get("u_z", 10000))
        dt = self.data
        assert "encrypted_u_guest" in dt.keys(
        ), "Error: 'encrypt_u_guest' from guest not successfully received."
        # encrypted_u_a = self.channel.recv()
        encrypted_u_guest = dt['encrypted_u_guest']
        encrypted_u = encrypted_u_guest + dt["encrypted_u_host"]
        # encrypted_u = []
        # for x, y in zip(encrypted_u_guest, dt["encrypted_u_host"]):
        #     encrypted_u = np.append(
        #         encrypted_u, opt_paillier_add(
        #             dt['public_key'], x, y))
        encrypted_dJ_host = self.compute_encrypted_dJ_host(
            batch_x, encrypted_u)
        self.mask = np.random.rand(len(encrypted_dJ_host))
        encrypted_masked_dJ_host = encrypted_dJ_host + self.mask

        assert "encrypted_z_guest_square" in dt.keys(
        ), "Error: 'encrypted_z_guest_square' from guest in step 1 not successfully received."

        encrypted_z = 4 * encrypted_u_guest + dt['z_host']  # w*x
        encrypted_loss = np.sum(np.log(2) - 0.5 * batch_y *
                                encrypted_z +
                                0.125 *
                                dt["encrypted_z_guest_square"] +
                                0.125 *
                                dt["z_host"] *
                                (encrypted_z +
                                 4 *
                                 encrypted_u_guest))
        data_to_arbiter = {
            "encrypted_masked_dJ_host": encrypted_masked_dJ_host,
            "encrypted_loss": encrypted_loss}
        self.proxy_client_arbiter.Remote(data_to_arbiter, "host_dJ_loss")

    '''
        host receive arbiter's decrypted gradient minus its own mask
    '''

    def update(self, batch_x):
        self.data.update(self.proxy_server.Get("masked_dJ_host", 10000))
        try:
            dt = self.data
            assert "masked_dJ_host" in dt.keys(
            ), "Error: 'masked_dJ_host' from arbiter  not successfully received."
            masked_dJ_host = dt['masked_dJ_host']
            # proxy_server.StopRecvLoop()
            dJ_host = masked_dJ_host - self.mask
            self.update_weight(dJ_host, batch_x)
        except Exception as e:
            logger.error("guest update weight exception: %s" % e)
        logger.info(f"host weight: {self.weights}")
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

    @staticmethod
    def sigmoid(x):
        a = np.exp(x)
        a /= (1. + a)
        return a

    @staticmethod
    def getKS(y_test, y_pred_prob):
        '''
        calculate value ks, list fpr , list tpr , list thresholds

        y_pred_prob: probability of true
        y_test: real y
        return :fpr, tpr, thresholds, ks.
        ROC curve：abscissa：fpr  ordinate：tpr
        ks curve：abscissa：thresholds ordinate：fpr and tpr two curves
        '''
        fpr, tpr, thresholds = metrics.roc_curve(y_test, y_pred_prob)
        ks = max(tpr - fpr)
        return fpr, tpr, thresholds, ks

    @staticmethod
    def get_threshold(true_y, pred_prob):
        fpr, tpr, thresholds, ks = Host.getKS(true_y, pred_prob)
        max_ks = 0
        # re = Host.config["threshold"]
        re = 0.5
        for i in range(len(thresholds)):
            if tpr[i] - fpr[i] > max_ks:
                max_ks = tpr[i] - fpr[i]
                re = thresholds[i]
        return re

    @staticmethod
    def predict_score_to_output(pred_prob, classes, threshold):
        class_neg, class_pos = classes[0], classes[1]
        if isinstance(threshold, float):
            pred_re = list(
                map((lambda x: class_neg if x < threshold else class_pos), pred_prob))
        else:
            pred_re = list(
                map((lambda x: class_neg if x < 0.5 else class_pos), pred_prob))
        return pred_re

    @staticmethod
    def predict(we, data_instances, true_y, guest_re, proxy_client_arbiter, proxy_server):
        # logger.info("weights.shape", we.shape, "x_test.shape", data_instances.shape)
        pred_prob_en = Host.compute_z_host(we, data_instances) + guest_re
        mask = np.random.rand(len(pred_prob_en))
        pred_prob_en = pred_prob_en + mask
        data_to_arbiter = {"pred_prob_en": pred_prob_en}
        proxy_client_arbiter.Remote(data_to_arbiter, "pred_prob_en")
        pred_prob = proxy_server.Get("pred_prob", 10000)
        pred_prob = pred_prob["pred_prob"]
        pred_prob = pred_prob - mask
        logger.info("pred_prob: {}".format(pred_prob))
        pred_prob = list(map(lambda x: Host.sigmoid(x), pred_prob))
        logger.info("true_y.shape: {}".format(true_y.shape))
        logger.info(true_y)
        logger.info("pred_prob: {}".format(pred_prob))
        threshold = Host.get_threshold(true_y, pred_prob)
        logger.info("threshold: {}".format(threshold))
        predict_result = Host.predict_score_to_output(
            pred_prob, classes=[0, 1], threshold=threshold)
        logger.info("predict_result: {}".format(predict_result))
        evaluation_result = classification_eva.getResult(
            true_y, predict_result, pred_prob)
        with open("result_phe.txt", 'a') as f:
            f.write(f"predict_result:{predict_result}")
            f.write("\n\n=============================================\n\n")
            f.write(f"pred_prob:{pred_prob}")
            f.write("\n\n=============================================\n\n")
            f.write(f"threshold:{threshold}")
            f.write("\n\n=============================================\n\n")
            for i in evaluation_result:
                f.write(i)
                f.write("\n")
            f.write("\n\n=============================================\n\n")
        return predict_result, pred_prob, threshold, evaluation_result


def run_hetero_lr_host(role_node_map, node_addr_map, dataset_filepath, params_map={}):
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

    host_port = node_addr_map[host_nodes[0]].split(":")[1]
    proxy_server = ServerChannelProxy(host_port)
    proxy_server.StartRecvLoop()
    logger.debug("Create server proxy for host, port {}.".format(host_port))

    guest_ip, guest_port = node_addr_map[guest_nodes[0]].split(":")
    proxy_client_guest = ClientChannelProxy(guest_ip, guest_port, "guest")
    logger.debug("Create client proxy to guest,"
                 " ip {}, port {}.".format(guest_ip, guest_port))

    arbiter_ip, arbiter_port = node_addr_map[arbiter_nodes[0]].split(":")
    proxy_client_arbiter = ClientChannelProxy(arbiter_ip, arbiter_port, "host")
    logger.debug("Create client proxy to arbiter,"
                 " ip {}, port {}.".format(arbiter_ip, arbiter_port))

    config = {
        'epochs': 1,
        'lambda': 10,
        'threshold': 0.5,
        'lr': 0.05,
        'batch_size': 500
    }

    # load train data
    x = data_host[1:, 1:-1]
    x = x.astype(float)
    label = data_host[1:, -1]
    label = label.astype(int)
    for i in range(len(label)):
        if label[i] == 2:
            label[i] = 0
        else:
            label[i] = 1
    count = x.shape[0]
    batch_num = count // config['batch_size'] + 1
    proxy_client_arbiter.Remote(batch_num, "batch_num")

    client_host = Host(x, label, config, proxy_server,
                       proxy_client_guest, proxy_client_arbiter)

    batch_gen_host = client_host.batch_generator(
        [x, label], config['batch_size'], False)
    for i in range(config['epochs']):
        logger.info("##### epoch %s ##### " % i)
        for j in range(batch_num):
            logger.info("-----epoch=%s, batch=%s-----" % (i, j))
            batch_host_x, batch_host_y = next(batch_gen_host)
            logger.info("batch_host_x.shape:{}".format(batch_host_x.shape))
            logger.info("batch_host_y.shape:{}".format(batch_host_y.shape))
            client_host.cal_u(batch_host_x, batch_host_y)
            client_host.cal_dJ_loss(batch_host_x, batch_host_y)
            client_host.update(batch_host_x)
            logger.info("batch=%s done" % j)
        logger.info("epoch=%i done" % i)
    logger.info("host training process done.")
    client_host.data.update(
        proxy_server.Get(
            "encrypted_z_guest_test", 10000))
    assert "encrypted_z_guest_test" in client_host.data.keys(
    ), "Error: 'encrypted_z_guest_test' from guest  not successfully received."
    guest_re = client_host.data["encrypted_z_guest_test"]
    # weights = np.concatenate(
    #     [client_host.data["guest_weights"], client_host.weights], axis=0)
    # logger.info("weights.shape", weights.shape)
    # logger.info(weights)
    # load test data
    x = data_test[1:, 3:-1]
    x = x.astype(float)
    logger.info("x.shape: ".format(x.shape))
    logger.info(x[0])
    label = data_test[1:, -1]
    label = label.astype(int)
    for i in range(len(label)):
        if label[i] == 2:
            label[i] = 0
        else:
            label[i] = 1

    client_host.predict(client_host.weights, x, label, guest_re,
                        proxy_client_arbiter, proxy_server)

    proxy_server.StopRecvLoop()
