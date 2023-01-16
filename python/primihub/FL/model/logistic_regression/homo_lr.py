import primihub as ph
import logging
from primihub import dataset, context

from os import path
import json
import os
from phe import paillier
import pickle
from primihub.FL.model.logistic_regression.vfl.evaluation_lr import evaluator

#from primihub.FL.model.logistic_regression.homo_lr_base import LRModel
import numpy as np

class LRModel:
    # l2 regularization by default, alpha is the penalty parameter
    def __init__(self, X, y, category, learning_rate=0.2, alpha=0.0001):
        self.learning_rate = learning_rate
        self.alpha = alpha # regularization parameter
        self.t = 0 # iteration number, used for learning rate decay

        if category == 2:
            self.theta = np.random.uniform(-0.5, 0.5, (X.shape[1] + 1,))
            self.multi_class = False
        else:
            self.one_vs_rest_theta = np.random.uniform(-0.5, 0.5, (category, X.shape[1] + 1))
            self.multi_class = True
        
        # 'optimal' learning rate refer to sklearn SGDClassifier
        def dloss(p, y):
            z = p * y
            if z > 18.0:
                return np.exp(-z) * -y
            if z < -18.0:
                return -y
            return -y / (np.exp(z) + 1.0)

        typw = np.sqrt(1.0 / np.sqrt(alpha))
        # computing eta0, the initial learning rate
        initial_eta0 = typw / max(1.0, dloss(-typw, 1.0))
        # initialize t such that eta at first sample equals eta0
        self.optimal_init = 1.0 / (initial_eta0 * alpha)

        # if encrypted == True:
        #     self.theta = self.utils.encrypt_vector(public_key, self.theta)

    def sigmoid(self, x):
        y = 1.0 / (1.0 + np.exp(-x))
        return y

    def get_theta(self):
        return self.theta

    def set_theta(self, theta):
        if not isinstance(theta, np.ndarray):
            theta = np.array(theta)
        self.theta = theta

    def loss(self, x_b, y):
        temp = x_b.dot(self.theta)
        try:
            return (np.maximum(temp, 0.).sum() - y.dot(temp) +
                    np.log(1 + np.exp(-np.abs(temp))).sum() +
                    0.5 * self.alpha * self.theta.dot(self.theta)) / x_b.shape[0]
        except:
            return float('inf')

    def compute_grad(self, x_b, y):
        out = self.sigmoid(x_b.dot(self.theta))
        return (x_b.T.dot(out - y) + self.alpha * self.theta) / len(x_b)

    def gradient_descent(self, x_b, y):
        grad = self.compute_grad(x_b, y)
        self.theta -= self.learning_rate * grad
        
    def gradient_descent_olr(self, x_b, y):
        """
        optimal learning rate
        """
        grad = self.compute_grad(x_b, y)
        learning_rate = 1.0 / (self.alpha * (self.optimal_init + self.t))
        self.t += 1
        self.theta -= learning_rate * grad
    
    def fit(self, x, y):
        x_b = np.hstack([np.ones((x.shape[0], 1)), x])
        self.gradient_descent_olr(x_b, y)

    def predict_prob(self, x):
        x_b = np.hstack([np.ones((len(x), 1)), x])
        return self.sigmoid(x_b.dot(self.theta))

    def predict(self, x):
        prob = self.predict_prob(x)
        return np.array(prob > 0.5, dtype='int')

    def one_vs_rest(self, X, y, k):
        all_theta = np.zeros((k, X.shape[1]))  # K个分类器的最终权重
        for i in range(1, k + 1):  # 因为y的取值为1，，，，10
            # 将y的值划分为二分类：0和1
            y_i = np.array([1 if label == i else 0 for label in y])
            theta = self.fit(X, y_i)
            # Whether to print the result rather than returning it
            all_theta[i - 1, :] = theta
        return all_theta

    def predict_all(self, X_predict, all_theta):
        y_pre = self.sigmoid(X_predict.dot(all_theta))
        y_argmax = np.argmax(y_pre, axis=1)
        return y_argmax

# def prepare_dummies(self, data, idxs):
#     self.onehot_encoder = HorOneHotEncoder()
#     return self.onehot_encoder.get_cats(data, idxs)
#
# def get_dummies(self, data, idxs):
#     return self.onehot_encoder.transform(data, idxs)
#
# def load_dummies(self, union_cats_len, union_cats_idxs):
#     self.onehot_encoder.cats_len = union_cats_len
#     self.onehot_encoder.cats_idxs = union_cats_idxs
import numpy as np
import pandas as pd
import copy
from primihub.FL.proxy.proxy import ServerChannelProxy
from primihub.FL.proxy.proxy import ClientChannelProxy
from os import path
import logging
from primihub.utils.logger_util import FLFileHandler, FLConsoleHandler, FORMAT

# def get_logger(name):
#     LOG_FORMAT = "[%(asctime)s][%(filename)s:%(lineno)d][%(levelname)s] %(message)s"
#     DATE_FORMAT = "%m/%d/%Y %H:%M:%S %p"
#     logging.basicConfig(level=logging.DEBUG,
#                         format=LOG_FORMAT,
#                         datefmt=DATE_FORMAT)
#     logger = logging.getLogger(name)
#     return logger

# logger = get_logger("Homo-LR-Host")

# dataset.define("breast_0")
# dataset.define("breast_1")
# dataset.define("breast_2")

config = {
    'learning_rate': 2.0,
    'alpha': 0.0001,
    'batch_size': 100,
    'max_iter': 1000,
    'n_iter_no_change': 5,
    'compare_threshold': 1e-6,
    'need_one_vs_rest': False,
    'need_encrypt': 'False',
    'category': 2,
    'feature_names': None,
}

def feature_selection(x, feature_names):
    if feature_names != None:
        return x[feature_names]
    else:
        return x

def read_data(dataset_key):
    x = ph.dataset.read(dataset_key).df_data

    if 'id' in x.columns:
        x.pop('id')

    y = x.pop('y').values
    x = feature_selection(x, config['feature_names']).to_numpy()
    return x, y

class Arbiter(LRModel):
    """
    Tips: Arbiter is a trusted third party !!!
    """

    def __init__(self, proxy_server, proxy_client_host, proxy_client_guest):
        self.need_one_vs_rest = None
        self.public_key = None
        self.private_key = None
        self.need_encrypt = None
        self.theta = None
        self.proxy_server = proxy_server
        self.proxy_client_host = proxy_client_host
        self.proxy_client_guest = proxy_client_guest

    def generate_key(self, length=1024):
        public_key, private_key = paillier.generate_paillier_keypair(
            n_length=length)
        self.public_key = public_key
        self.private_key = private_key

    def broadcast_key(self):
        try:
            self.generate_key()
            # logger.info("start send pub")
            self.proxy_client_host.Remote(self.public_key, "pub")
            # logger.info("send pub to host OK")
        except Exception as e:
            print("Arbiter broadcast key pair error : %s" % e)
            # logger.info("Arbiter broadcast key pair error : %s" % e)

    def predict_prob(self, x):
        if self.need_encrypt:
            global_theta = self.decrypt_vector(self.theta)
            x = np.hstack([np.ones((len(x), 1)), x])
            prob = self.sigmoid(x.dot(global_theta))
            return prob
        else:
            return super().predict_prob(x)

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
            return super().predict(data)
        else:
            return self.predict_one_vs_rest(data)

    def model_aggregate(self, host_parm, guest_param, host_data_weight,
                        guest_data_weight):
        param = []
        weight = []
        if self.need_encrypt == 'YES':
            if self.need_one_vs_rest == False:
                param.append(self.decrypt_vector(host_parm))
                param.append(self.decrypt_vector(guest_parm))
            else:
                param.append(self.decrypt_matrix(host_parm))
                param.append(self.decrypt_matrix(guest_parm))
            weight.append(self.decrypt_vector(host_data_weight)[0])
            weight.append(self.decrypt_vector(guest_data_weight)[0])
        else:
            param.append(host_parm)
            param.append(guest_param)
            weight.append(host_data_weight)
            weight.append(guest_data_weight)

        param = np.array(param)
        weight = np.array(weight)

        if self.need_one_vs_rest == True:
            agg_param = np.zeros_like(np.array(param))
            for id_c, p in enumerate(param):
                w = weight[id_c] / np.sum(weight, axis=0)
                for id_d, d in enumerate(p):
                    d = np.array(d)
                    agg_param[id_c, id_d] += d * w
            self.theta = np.sum(agg_param, axis=0)
        else:
            self.theta = np.average(param, weights=weight/weight.sum(), axis=0)
        return list(self.theta)

    def broadcast_global_model_param(self, host_param, guest_param,
                                     host_data_weight, guest_data_weight):
        self.theta = self.model_aggregate(host_param, guest_param,
                                          host_data_weight, guest_data_weight)
        if self.need_encrypt == 'YES':
            if self.need_one_vs_rest == False:
                self.theta = self.encrypt_vector(self.theta)
            else:
                self.theta = self.encrypt_matrix(self.theta)

        self.proxy_client_host.Remote(self.theta, "global_host_model_param")
        self.proxy_client_guest.Remote(self.theta, "global_guest_model_param")

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


def run_homo_lr_arbiter(role_node_map,
                        node_addr_map,
                        data_key,
                        task_params={},
                        log_handler=None):
    host_nodes = role_node_map["host"]
    guest_nodes = role_node_map["guest"]
    arbiter_nodes = role_node_map["arbiter"]
    eva_type = ph.context.Context.params_map.get("taskType", None)

    if len(host_nodes) != 1:
        log_handler.error("Hetero LR only support one host party, but current "
                          "task have {} host party.".format(len(host_nodes)))
        return

    if len(guest_nodes) != 1:
        log_handler.error("Hetero LR only support one guest party, but current "
                          "task have {} guest party.".format(len(guest_nodes)))
        return

    if len(arbiter_nodes) != 1:
        log_handler.error(
            "Hetero LR only support one arbiter party, but current "
            "task have {} arbiter party.".format(len(arbiter_nodes)))
        return

    # host_info = host_info[0]
    # guest_info = guest_info[0]
    # arbiter_info = arbiter_info[0]
    # arbiter_port = node_addr_map[arbiter_nodes[0]].split(":")[1]
    # arbiter_port = arbiter_info['port']
    arbiter_port = node_addr_map[arbiter_nodes[0]].split(":")[1]
    proxy_server = ServerChannelProxy(arbiter_port)
    proxy_server.StartRecvLoop()
    log_handler.debug(
        "Create server proxy for arbiter, port {}.".format(arbiter_port))

    # host_ip, host_port = node_addr_map[host_nodes[0]].split(":")
    # host_ip, host_port = host_info['ip'], host_info['port']
    host_ip, host_port = node_addr_map[host_nodes[0]].split(":")
    proxy_client_host = ClientChannelProxy(host_ip, host_port, "host")
    log_handler.debug("Create client proxy to host,"
                      " ip {}, port {}.".format(host_ip, host_port))

    # guest_ip, guest_port = node_addr_map[guest_nodes[0]].split(":")
    # guest_ip, guest_port = guest_info['ip'], guest_info['port']
    guest_ip, guest_port = node_addr_map[guest_nodes[0]].split(":")

    proxy_client_guest = ClientChannelProxy(guest_ip, guest_port, "guest")
    log_handler.debug("Create client proxy to guest,"
                      " ip {}, port {}.".format(guest_ip, guest_port))   
        
    x, y= read_data(data_key)

    client_arbiter = Arbiter(proxy_server, proxy_client_host,
                             proxy_client_guest)
    client_arbiter.need_one_vs_rest = config['need_one_vs_rest']
    need_encrypt = proxy_server.Get("need_encrypt")

    if need_encrypt == 'YES':
        client_arbiter.need_encrypt = 'YES'
        client_arbiter.broadcast_key()

    host_data_weight = proxy_server.Get("host_data_weight")
    guest_data_weight = proxy_server.Get("guest_data_weight")

    # data preprocessing
    # minmaxscaler
    host_data_max = np.array(proxy_server.Get("host_data_max"))
    guest_data_max = np.array(proxy_server.Get("guest_data_max"))
    host_data_min = np.array(proxy_server.Get("host_data_min"))
    guest_data_min = np.array(proxy_server.Get("guest_data_min"))

    data_max = np.maximum(host_data_max, guest_data_max)
    data_min = np.minimum(host_data_min, guest_data_min)
 
    x = (x - data_min) / (data_max - data_min)

    data_max = list(data_max) 
    data_min = list(data_min)
    proxy_client_host.Remote(data_max, "data_max")
    proxy_client_guest.Remote(data_max, "data_max")
    proxy_client_host.Remote(data_min, "data_min")
    proxy_client_guest.Remote(data_min, "data_min")

    n_iter_no_change = config['n_iter_no_change']
    compare_threshold = config['compare_threshold']
    count_iter_no_change = 0
    convergence = 'NO'
    
    last_acc = 0

    for i in range(config['max_iter']):
        log_handler.info("-------- start iteration {} --------".format(i+1))
            
        host_param = proxy_server.Get("host_param")
        guest_param = proxy_server.Get("guest_param")
        client_arbiter.broadcast_global_model_param(host_param, guest_param,
                                                    host_data_weight,
                                                    guest_data_weight)
                                                        
        y_hat = client_arbiter.predict_prob(x)
        acc = evaluator.getAccuracy(y, (y_hat >= 0.5).astype('int'))
        auc = evaluator.getAUC(y, y_hat)
        fpr, tpr, thresholds, ks = evaluator.getKS(y, y_hat)

        # only print acc, auc, ks
        # fpr and tpr can be finded in the json file
        log_handler.info("acc={}, auc={}, ks={}".format(acc, auc, ks))
        
        # convergence is checked using acc
        if abs(last_acc - acc) < compare_threshold:
            count_iter_no_change += 1
        else:
            count_iter_no_change = 0
        last_acc = acc

        if count_iter_no_change > n_iter_no_change:
            convergence = 'YES'
            
        proxy_client_host.Remote(convergence, "convergence")
        proxy_client_guest.Remote(convergence, "convergence")

        if convergence == 'YES':
            log_handler.info("-------- end at iteration {} --------".format(i+1))
            break

    indicator_file_path = ph.context.Context.get_indicator_file_path()
    log_handler.info("Current metrics file path is: {}".format(indicator_file_path))
    trainMetrics = {
        "train_acc": acc,
        "train_auc": auc,
        "train_ks": ks,
        "train_fpr": fpr.tolist(),
        "train_tpr": tpr.tolist()
    }
    trainMetricsBuff = json.dumps(trainMetrics)
    with open(indicator_file_path, 'w') as filePath:
        filePath.write(trainMetricsBuff)

    log_handler.info("####### start predict ######")
    log_handler.info("All process done.")
    proxy_server.StopRecvLoop()


class Client(LRModel):

    def __init__(self, X, y, proxy_server, proxy_client_arbiter):
        super().__init__(X, y, category=config['category'], learning_rate=config['learning_rate'], alpha=config['alpha'])
        self.public_key = None
        self.need_encrypt = config['need_encrypt']
        self.need_one_vs_rest = None
        self.flag = True
        self.proxy_server = proxy_server
        self.proxy_client_arbiter = proxy_client_arbiter
    
    def get_theta(self):
        return list(super().get_theta())

    def fit_binary(self, batch_x, batch_y):
        if self.need_one_vs_rest == False:
            theta = self.theta
        else:
            theta = list(self.theta)
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
            super().fit(batch_x, batch_y)
            
    def one_vs_rest(self, X, y, k):
        all_theta = []
        for i in range(0, k):
            y_i = np.array([1 if label == i else 0 for label in y])
            theta = self.fit_binary(X, y_i, self.one_vs_rest_theta[i])
            all_theta.append(theta)
        return all_theta

    def fit(self, X, y):
        if not self.multi_class:
            self.need_one_vs_rest = False
            self.fit_binary(X, y)
        else:
            self.need_one_vs_rest = True
            self.one_vs_rest(X, y, category)

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
        # logger.info("data_size: {}".format(data_size))
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


def run_homo_lr_client(role_node_map,
                       node_addr_map,
                       data_key,
                       client_name,
                       task_params={},
                       log_handler=None):
    client_nodes = role_node_map[client_name]
    arbiter_nodes = role_node_map["arbiter"]

    if len(client_nodes) != 1:
        log_handler.error("Homo LR only support one {0} party, but current "
                          "task have {1} {0} party.".format(client_name, len(client_nodes)))
        return

    if len(arbiter_nodes) != 1:
        log_handler.error("Homo LR only support one arbiter party, but current "
                          "task have {} arbiter party.".format(
                              len(arbiter_nodes)))
        return

    client_port = node_addr_map[client_nodes[0]].split(":")[1]

    arbiter_ip, arbiter_port = node_addr_map[arbiter_nodes[0]].split(":")

    proxy_server = ServerChannelProxy(client_port)
    proxy_server.StartRecvLoop()
    log_handler.debug(
        "Create server proxy for {}, port {}.".format(client_name, client_port))

    proxy_client_arbiter = ClientChannelProxy(arbiter_ip, arbiter_port,
                                              "arbiter")
    log_handler.debug("Create client proxy to arbiter,"
                      " ip {}, port {}.".format(arbiter_ip, arbiter_port))

    x, label = read_data(data_key)

    client = Client(x, label, proxy_server, proxy_client_arbiter)
    proxy_client_arbiter.Remote(client.need_encrypt, "need_encrypt")
    data_weight = config['batch_size']
    
    if client.need_encrypt == 'YES':
        # if task_params['encrypted']:
        client.public_key = proxy_server.Get("pub")
        data_weight = client.encrypt_vector([data_weight])

    proxy_client_arbiter.Remote(data_weight, client_name+"_data_weight")
  
    # data preprocessing
    # minmaxscaler
    data_max = x.max(axis=0)
    data_min = x.min(axis=0)

    proxy_client_arbiter.Remote(list(data_max), client_name+"_data_max")
    proxy_client_arbiter.Remote(list(data_min), client_name+"_data_min")

    data_max = np.array(proxy_server.Get("data_max"))
    data_min = np.array(proxy_server.Get("data_min"))

    x = (x - data_min) / (data_max - data_min)
    
    batch_gen = client.batch_generator([x, label],
                                            config['batch_size'], False)

    for i in range(config['max_iter']):
        log_handler.info("-------- start iteration {} --------".format(i+1))
        
        batch_x, batch_y = next(batch_gen)
        client.fit(batch_x, batch_y)

        proxy_client_arbiter.Remote(client.get_theta(), client_name+"_param")
        client.set_theta(proxy_server.Get(
                        "global_"+client_name+"_model_param"))

        if proxy_server.Get('convergence') == 'YES':
            log_handler.info("-------- end at iteration {} --------".format(i+1))
            break

    log_handler.info("{} training process done.".format(client_name))
    model_file_path = ph.context.Context.get_model_file_path()
    log_handler.info("Current model file path is: {}".format(model_file_path))
    with open(model_file_path, 'wb') as fm:
        pickle.dump(client.get_theta(), fm)

    proxy_server.StopRecvLoop()


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


# def get_logger(name):
#     LOG_FORMAT = "[%(asctime)s][%(filename)s:%(lineno)d][%(levelname)s] %(message)s"
#     DATE_FORMAT = "%m/%d/%Y %H:%M:%S %p"
#     logging.basicConfig(level=logging.DEBUG,
#                         format=LOG_FORMAT,
#                         datefmt=DATE_FORMAT)
#     logger = logging.getLogger(name)
#     return logger

# # arbiter_info, guest_info, host_info, task_type, task_params = load_info()
# task_params = {}
# logger = get_logger("Homo-LR")


def run_party(party_name):
    role_node_map = ph.context.Context.get_role_node_map()
    node_addr_map = ph.context.Context.get_node_addr_map()
    dataset_map = ph.context.Context.dataset_map

    taskId = ph.context.Context.params_map['taskid']
    jobId = ph.context.Context.params_map['jobid']

    console_handler = FLConsoleHandler(jb_id=jobId,
                                       task_id=taskId,
                                       log_level='info',
                                       format=FORMAT)
    fl_console_log = console_handler.set_format()

    fl_console_log.debug("dataset_map {}".format(dataset_map))
    data_key = list(dataset_map.keys())[0]

    fl_console_log.debug("role_nodeid_map {}".format(role_node_map))

    fl_console_log.debug("node_addr_map {}".format(node_addr_map))
    fl_console_log.info("Start homo-LR {} logic.".format(party_name))
    
    if party_name == 'arbiter':
        run_homo_lr_arbiter(role_node_map,
                            node_addr_map,
                            data_key,
                            log_handler=fl_console_log)
    else:
        run_homo_lr_client(role_node_map,
                           node_addr_map,
                           data_key,
                           client_name=party_name,
                           log_handler=fl_console_log)

    fl_console_log.info("Finish homo-LR {} logic.".format(party_name))


@ph.context.function(role='arbiter',
                     protocol='lr',
                     datasets=['homo_lr_data'],
                     port='9010',
                     task_type="lr-train")
def run_arbiter_party():
    run_party('arbiter')


@ph.context.function(role='host',
                     protocol='lr',
                     datasets=['train_homo_lr_host'],
                     port='9020',
                     task_type="lr-train")
def run_host_party():
    run_party('host')


@ph.context.function(role='guest',
                     protocol='lr',
                     datasets=['train_homo_lr_guest'],
                     port='9030',
                     task_type="lr-train")
def run_guest_party():
    run_party('guest')
