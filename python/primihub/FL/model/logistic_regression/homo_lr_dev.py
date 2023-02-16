import primihub as ph
import logging
from primihub import dataset, context

from os import path
import json
import os
from phe import paillier
import pickle
from primihub.FL.model.logistic_regression.vfl.evaluation_lr import evaluator


class GrpcServer:

    def __init__(self, local_ip, local_port, remote_ip, remote_port,
                 context) -> None:
        send_session = context.Node(remote_ip, int(remote_port), False)
        recv_session = context.Node(local_ip, int(local_port), False)

        self.send_channel = context.get_link_conext().getChannel(send_session)
        self.recv_channel = context.get_link_conext().getChannel(recv_session)

    def send(self, key, val):
        self.send_channel.send(key, pickle.dumps(val))

    def recv(self, key):
        recv_val = self.recv_channel.recv(key)
        return pickle.loads(recv_val)


#from primihub.FL.model.logistic_regression.homo_lr_base import LRModel
class LRModel:

    # l2 regularization by default, alpha is the penalty parameter
    def __init__(self, x, y, category, learning_rate=0.2, alpha=0.0001):
        self.learning_rate = learning_rate
        self.alpha = alpha # regularization parameter
        self.t = 0 # iteration number, used for learning rate decay

        if category == 2:
            self.theta = np.zeros(x.shape[1] + 1)
            self.multi_class = False
        else:
            self.one_vs_rest_theta = np.random.uniform(-0.5, 0.5, (category, x.shape[1] + 1))
            self.multi_class = True
        
        # 'optimal' learning rate refer to sklearn SGDClassifier
        def dloss(p, y):
            z = p * y
            if z > 18.0:
                return np.exp(-z) * -y
            if z < -18.0:
                return -y
            return -y / (np.exp(z) + 1.0)
        
        if self.learning_rate == 'optimal':
            typw = np.sqrt(1.0 / np.sqrt(alpha))
            # computing eta0, the initial learning rate
            initial_eta0 = typw / max(1.0, dloss(-typw, 1.0))
            # initialize t such that eta at first sample equals eta0
            self.optimal_init = 1.0 / (initial_eta0 * alpha)

    def sigmoid(self, x):
        return 1.0 / (1.0 + np.exp(-x))

    def get_theta(self):
        return self.theta

    def set_theta(self, theta):
        if not isinstance(theta, np.ndarray):
            theta = np.array(theta)
        self.theta = theta

    def loss(self, x, y):
        temp = x.dot(self.theta[1:]) + self.theta[0]
        try:
            return (np.maximum(temp, 0.).sum() - y.dot(temp) +
                    np.log(1 + np.exp(-np.abs(temp))).sum() +
                    0.5 * self.alpha * self.theta.dot(self.theta)) / x.shape[0]
        except:
            return float('inf')

    def compute_grad(self, x, y):
        temp = self.predict_prob(x) - y
        return (np.concatenate((temp.sum(keepdims=True), x.T.dot(temp)))
                + self.alpha * self.theta) / x.shape[0]

    def gradient_descent(self, x, y):
        grad = self.compute_grad(x, y)
        self.theta -= self.learning_rate * grad
        
    def gradient_descent_olr(self, x, y):
        # 'optimal' learning rate: 1.0 / (alpha * (t0 + t))
        grad = self.compute_grad(x, y)
        learning_rate = 1.0 / (self.alpha * (self.optimal_init + self.t))
        self.t += 1 
        self.theta -= learning_rate * grad
    
    def fit(self, x, y):
        if self.learning_rate == 'optimal':
            self.gradient_descent_olr(x, y)
        else:
            self.gradient_descent(x, y)

    def predict_prob(self, x):
        return self.sigmoid(x.dot(self.theta[1:]) + self.theta[0])

    def predict(self, x):
        prob = self.predict_prob(x)
        return np.array(prob > 0.5, dtype='int')


import numpy as np
import pandas as pd
from primihub.utils.logger_util import FLFileHandler, FLConsoleHandler, FORMAT
import dp_accounting


'''
# Plaintext
config = {
    'mode': 'Plaintext', 
    'learning_rate': 'optimal',
    'alpha': 0.0001,
    'batch_size': 100,
    'max_iter': 200,
    'n_iter_no_change': 5,
    'compare_threshold': 1e-6,
    'category': 2,
    'feature_names': None,
}
'''

'''
# DPSGD
config = {
    'mode': 'DPSGD',
    'delta': 1e-3,
    'noise_multiplier': 2.0,
    'l2_norm_clip': 1.0,
    'secure_mode': True,
    'learning_rate': 'optimal',
    'alpha': 0.0001,
    'batch_size': 50,
    'max_iter': 100,
    'category': 2,
    'feature_names': None,
}
'''

#'''
# Paillier
config = {
    'mode': 'Paillier',
    'n_length': 1024,
    'learning_rate': 'optimal',
    'alpha': 0.01,
    'batch_size': 100,
    'max_iter': 50,
    'n_iter_no_change': 5,
    'compare_threshold': 1e-6,
    'category': 2,
    'feature_names': None,
}
#'''


def feature_selection(x, feature_names):
    if feature_names != None:
        return x[feature_names]
    return x


def read_data(dataset_key, feature_names):
    x = ph.dataset.read(dataset_key).df_data

    if 'id' in x.columns:
        x.pop('id')

    y = x.pop('y').values
    x = feature_selection(x, feature_names).to_numpy()
    return x, y


def compute_epsilon(steps, num_train_examples, config):
    """Computes epsilon value for given hyperparameters."""
    if config['noise_multiplier'] == 0.0:
        return float('inf')
    orders = [1 + x / 10. for x in range(1, 100)] + list(range(12, 64))
    accountant = dp_accounting.rdp.RdpAccountant(orders)

    sampling_probability = config['batch_size'] / num_train_examples
    event = dp_accounting.SelfComposedDpEvent(
        dp_accounting.PoissonSampledDpEvent(
            sampling_probability,
            dp_accounting.GaussianDpEvent(config['noise_multiplier'])), steps)

    accountant.compose(event)
    
    assert config['delta'] < 1. / num_train_examples
    return accountant.get_epsilon(target_delta=config['delta'])


class LRModel_DPSGD(LRModel):

    def __init__(self, x, y, category, learning_rate=0.2, alpha=0.0001,
                 noise_multiplier=1.0, l2_norm_clip=1.0, secure_mode=True,
                 *args):
        super().__init__(x, y, category, learning_rate, alpha, *args)
        self.noise_multiplier = noise_multiplier
        self.l2_norm_clip = l2_norm_clip
        self.secure_mode = secure_mode

    def set_noise_multiplier(self, noise_multiplier):
        self.noise_multiplier = noise_multiplier

    def set_l2_norm_clip(self, l2_norm_clip):
        self.l2_norm_clip = l2_norm_clip
    
    def compute_grad(self, x, y): 
        temp = np.expand_dims(self.predict_prob(x) - y, axis=1)
        batch_grad = np.hstack([temp, x * temp])

        batch_grad_l2_norm = np.sqrt((batch_grad ** 2).sum(axis=1))
        clip = np.maximum(1., batch_grad_l2_norm / self.l2_norm_clip)

        grad = (batch_grad / np.expand_dims(clip, axis=1)).sum(axis=0)

        if self.secure_mode:
            noise = np.zeros(grad.shape)
            n = 2
            for _ in range(2 * n):
                noise += np.random.normal(0, self.l2_norm_clip * self.noise_multiplier, grad.shape)
            noise /= np.sqrt(2 * n)
        else:
            noise = np.random.normal(0, self.l2_norm_clip * self.noise_multiplier, grad.shape)

        grad += noise
        return grad / x.shape[0]


class LRModel_Paillier(LRModel):

    def __init__(self, x, y, category, learning_rate=0.2, alpha=0.0001,
                 n_length=1024, *args):
        super().__init__(x, y, category, learning_rate, alpha, *args)
        self.public_key = None
        self.private_key = None

    def decrypt_scalar(self, cipher_scalar):
        return self.private_key.decrypt(cipher_scalar)

    def decrypt_vector(self, cipher_vector):
        return [self.private_key.decrypt(i) for i in cipher_vector]

    def decrypt_matrix(self, cipher_matrix):
        return [[self.private_key.decrypt(i) for i in cv] for cv in cipher_matrix]
    
    def encrypt_scalar(self, plain_scalar):
        return self.public_key.encrypt(plain_scalar)

    def encrypt_vector(self, plain_vector):
        return [self.public_key.encrypt(i) for i in plain_vector]

    def encrypt_matrix(self, plain_matrix):
        return [[self.private_key.encrypt(i) for i in pv] for pv in plain_matrix]
    
    def compute_grad(self, x, y):
        # Taylor first order expansion: sigmoid(x) = 0.5 + 0.25 * (x.dot(w) + b)
        temp = 0.5 + 0.25 * (x.dot(self.theta[1:]) + self.theta[0]) - y
        return (np.concatenate((temp.sum(keepdims=True), x.T.dot(temp)))
                + self.alpha * self.theta) / x.shape[0]

    def loss(self, x, y):
        # Taylor first order expansion: L(x) = ln2 + (0.5 - y) * (x.dot(w) + b)
        return (np.log(2) + (0.5 - y).dot(x.dot(self.theta[1:] + self.theta[0]))) / x.shape[0]


class Arbiter(LRModel):
    """
    Tips: Arbiter is a trusted third party !!!
    """

    def __init__(self, host_channel, guest_channel, log_handler):
        self.theta = None
        self.host_channel = host_channel
        self.guest_channel = guest_channel
        self.log_handler = log_handler
        self.metrics = {}
        self.loss_value = None

    def model_aggregate(self, host_param, guest_param, param_weights):
        param = np.vstack([host_param, guest_param])
        self.set_theta(np.average(param, weights=param_weights, axis=0))

    def broadcast_global_model_param(self):
        self.host_channel.send("global_param", self.theta)
        self.guest_channel.send("global_param", self.theta)

    def loss_and_acc(self, losses, accs, weights):
        return np.average(losses, weights=weights), np.average(accs, weights=weights)

    def metrics_log(self, weights):
        host_loss = self.host_channel.recv("host_loss")
        guest_loss = self.guest_channel.recv("guest_loss")
        losses = [host_loss, guest_loss]
        host_acc = self.host_channel.recv("host_acc")
        guest_acc = self.guest_channel.recv("guest_acc")
        accs = [host_acc, guest_acc]
        loss, acc = self.loss_and_acc(losses, accs, weights)
        self.log_handler.info("loss={}, acc={}".format(loss, acc))

        self.loss_value = loss
        self.metrics = {
            "train_loss": loss,
            "train_acc": acc,
        }

    def get_metrics(self):
        return self.metrics

    def get_loss(self):
        return self.loss_value


class Arbiter_Paillier(LRModel_Paillier, Arbiter):
    
    def __init__(self, host_channel, guest_channel, log_handler):
        Arbiter.__init__(self, host_channel, guest_channel, log_handler)
        self.public_key, self.private_key = paillier.generate_paillier_keypair(n_length=config['n_length']) 
        self.broadcast_public_key()

    def broadcast_public_key(self):
        self.host_channel.send("public_key", self.public_key)
        self.guest_channel.send("public_key", self.public_key)
        
    def model_aggregate(self, host_param, guest_param, param_weights):
        host_param = self.decrypt_vector(host_param)
        guest_param = self.decrypt_vector(guest_param)
                
        Arbiter.model_aggregate(self, host_param, guest_param, param_weights)

    def broadcast_global_model_param(self):
        global_theta = self.encrypt_vector(self.theta)
        self.host_channel.send("global_param", global_theta)
        self.guest_channel.send("global_param", global_theta)

    def broadcast_plaintext_global_model_param(self):
        Arbiter.broadcast_global_model_param(self)

    def loss_log(self, weights):
        host_loss = self.host_channel.recv("host_loss")
        guest_loss = self.guest_channel.recv("guest_loss")
        losses = self.decrypt_vector([host_loss, guest_loss])
        loss = np.average(losses, weights=weights)
        self.log_handler.info("loss={}".format(loss))

        self.loss_value = loss


def run_homo_lr_arbiter(config,
                        role_node_map,
                        node_addr_map,
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
    
    arbiter_ip, arbiter_port = node_addr_map[arbiter_nodes[0]].split(":")
    host_ip, host_port = node_addr_map[host_nodes[0]].split(":")
    guest_ip, guest_port = node_addr_map[guest_nodes[0]].split(":")

    host_channel = GrpcServer(local_ip=arbiter_ip,
                              local_port=arbiter_port,
                              remote_ip=host_ip,
                              remote_port=host_port,
                              context=ph.context.Context)

    guest_channel = GrpcServer(local_ip=arbiter_ip,
                               local_port=arbiter_port,
                               remote_ip=guest_ip,
                               remote_port=guest_port,
                               context=ph.context.Context)

    log_handler.info("Create channel between arbiter and host, "+
                     "locoal ip {}, local port {}, ".format(arbiter_ip, arbiter_port)+
                     "remote ip {}, remote port {}.".format(host_ip, host_port)) 

    log_handler.info("Create channel between arbiter and guest, "+
                     "locoal ip {}, local port {}, ".format(arbiter_ip, arbiter_port)+
                     "remote ip {}, remote port {}.".format(guest_ip, guest_port)) 
    
    if config['mode'] == 'Plaintext':
        check_convergence = True
        arbiter = Arbiter(host_channel, guest_channel, log_handler)
    elif config['mode'] == 'DPSGD':
        # Due to added noise, don't check convergence in DPSGD mode
        check_convergence = False
        arbiter = Arbiter(host_channel, guest_channel, log_handler)
    elif config['mode'] == 'Paillier':
        check_convergence = True
        arbiter = Arbiter_Paillier(host_channel, guest_channel, log_handler)
    else:
        log_handler.info('Mode {} is not supported yet'.format(config['mode']))

    host_param_weight = host_channel.recv("host_param_weight")
    guest_param_weight = guest_channel.recv("guest_param_weight")
    param_weights = [host_param_weight, guest_param_weight]

    host_num_train_examples = host_channel.recv("host_num_train_examples")
    guest_num_train_examples = guest_channel.recv("guest_num_train_examples")
    num_train_examples_weights = [host_num_train_examples, guest_num_train_examples]

    # data preprocessing
    # minmaxscaler
    host_data_max = host_channel.recv("host_data_max")
    guest_data_max = guest_channel.recv("guest_data_max")
    host_data_min = host_channel.recv("host_data_min")
    guest_data_min = guest_channel.recv("guest_data_min")

    data_max = np.maximum(host_data_max, guest_data_max)
    data_min = np.minimum(host_data_min, guest_data_min)

    host_channel.send("data_max", data_max)
    guest_channel.send("data_max", data_max)
    host_channel.send("data_min", data_min)
    guest_channel.send("data_min", data_min)

    if check_convergence:
        n_iter_no_change = config['n_iter_no_change']
        compare_threshold = config['compare_threshold']
        count_iter_no_change = 0
        convergence = 'NO'
        last_loss = 0

    for i in range(config['max_iter']):
        log_handler.info("-------- start iteration {} --------".format(i+1))    
        
        # model training
        host_param = host_channel.recv("host_param")
        guest_param = guest_channel.recv("guest_param")
        arbiter.model_aggregate(host_param, guest_param, param_weights)
        arbiter.broadcast_global_model_param()
        
        # metrics log
        if config['mode'] == 'Paillier': 
            arbiter.loss_log(num_train_examples_weights)
        else:
            arbiter.metrics_log(num_train_examples_weights)
                    
        # check convergence 
        if check_convergence:
            # convergence is checked using loss
            loss = arbiter.get_loss()
            if abs(last_loss - loss) < compare_threshold:
                count_iter_no_change += 1
            else:
                count_iter_no_change = 0
            last_loss = loss
    
            if count_iter_no_change > n_iter_no_change:
                convergence = 'YES'
            
            host_channel.send("convergence", convergence)
            guest_channel.send("convergence", convergence)

            if convergence == 'YES':
                log_handler.info("-------- end at iteration {} --------".format(i+1))
                break

    if config['mode'] == 'DPSGD':
        host_eps = host_channel.recv('host_eps')
        guest_eps = guest_channel.recv('guest_eps')
        eps = max(host_eps, guest_eps)
        log_handler.info('For delta={}, the current epsilon is: {:.2f}'.format(config['delta'], eps))
    elif config['mode'] == 'Paillier':
        arbiter.broadcast_plaintext_global_model_param()
        arbiter.metrics_log(num_train_examples_weights)

    indicator_file_path = ph.context.Context.get_indicator_file_path()
    log_handler.info("Current metrics file path is: {}".format(indicator_file_path))

    trainMetrics = arbiter.get_metrics()
    trainMetricsBuff = json.dumps(trainMetrics)
    with open(indicator_file_path, 'w') as filePath:
        filePath.write(trainMetricsBuff)

    log_handler.info("####### start predict ######")
    log_handler.info("All process done.")


def batch_generator(all_data, batch_size, shuffle=True):
    all_data = [np.array(d) for d in all_data]
    data_size = all_data[0].shape[0]
        
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


class Client(LRModel):

    def __init__(self, x, y, category, learning_rate, alpha,
                 arbiter_channel, client_name, log_handler):
        super().__init__(x, y, category, learning_rate, alpha)
        self.arbiter_channel = arbiter_channel
        self.client_name = client_name
        self.log_handler = log_handler
    
    def metrics_log(self, x, y):
        loss = self.loss(x, y)
        self.arbiter_channel.send(self.client_name+"_loss", loss)
        y_hat = self.predict_prob(x)
        acc = evaluator.getAccuracy(y, (y_hat >= 0.5).astype('int'))
        self.arbiter_channel.send(self.client_name+"_acc", acc)
        self.log_handler.info("loss={}, acc={}".format(loss, acc))


class Client_DPSGD(LRModel_DPSGD, Client):

    def __init__(self, x, y, category, learning_rate, alpha,
                 noise_multiplier, l2_norm_clip, secure_mode,
                 arbiter_channel, client_name, log_handler):
        super().__init__(x, y, category, learning_rate, alpha,
                         noise_multiplier, l2_norm_clip, secure_mode,
                         arbiter_channel, client_name, log_handler)
    

class Client_Paillier(LRModel_Paillier, Client):

    def __init__(self, x, y, category, learning_rate, alpha,
                 n_length,
                 arbiter_channel, client_name, log_handler):
        super().__init__(x, y, category, learning_rate, alpha,
                         n_length,
                         arbiter_channel, client_name, log_handler)
        self.public_key = arbiter_channel.recv("public_key")
        self.set_theta(self.encrypt_vector(self.theta))

    def send_loss(self, x, y):
        loss = self.loss(x, y)
        self.arbiter_channel.send(self.client_name+"_loss", loss)


def run_homo_lr_client(config,
                       role_node_map,
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

    client_ip, client_port = node_addr_map[client_nodes[0]].split(":")
    arbiter_ip, arbiter_port = node_addr_map[arbiter_nodes[0]].split(":")

    arbiter_channel = GrpcServer(local_ip=client_ip,
                                 local_port=client_port,
                                 remote_ip=arbiter_ip,
                                 remote_port=arbiter_port,
                                 context=ph.context.Context)

    log_handler.info("Create channel between {} and arbiter, ".format(client_name)+
                      "locoal ip {}, local port {}, ".format(client_ip, client_port)+
                      "remote ip {}, remote port {}.".format(arbiter_ip, arbiter_port)) 
    
    x, y = read_data(data_key, config['feature_names'])
    param_weight = config['batch_size']
    num_train_examples = x.shape[0]

    if config['mode'] == 'Plaintext':
        check_convergence = True
        client = Client(x, y, config['category'], config['learning_rate'], config['alpha'],
                        arbiter_channel, client_name, log_handler)
    elif config['mode'] == 'DPSGD':
        # Due to added noise, don't check convergence in DPSGD mode
        check_convergence = False
        client = Client_DPSGD(x, y, config['category'], config['learning_rate'], config['alpha'],
                              config['noise_multiplier'], config['l2_norm_clip'], config['secure_mode'],
                              arbiter_channel, client_name, log_handler)
    elif config['mode'] == 'Paillier':
        check_convergence = True
        client = Client_Paillier(x, y, config['category'], config['learning_rate'], config['alpha'],
                                 config['n_length'],
                                 arbiter_channel, client_name, log_handler)
    else:
        log_handler.info('Mode {} is not supported yet'.format(config['mode']))

    arbiter_channel.send(client_name+"_param_weight", param_weight)
    arbiter_channel.send(client_name+"_num_train_examples", num_train_examples) 
  
    # data preprocessing
    # minmaxscaler
    data_max = x.max(axis=0)
    data_min = x.min(axis=0)

    arbiter_channel.send(client_name+"_data_max", data_max)
    arbiter_channel.send(client_name+"_data_min", data_min)

    data_max = arbiter_channel.recv("data_max")
    data_min = arbiter_channel.recv("data_min")

    x = (x - data_min) / (data_max - data_min)

    batch_gen = batch_generator([x, y], config['batch_size'], False)

    for i in range(config['max_iter']):
        log_handler.info("-------- start iteration {} --------".format(i+1))
        
        # model training
        batch_x, batch_y = next(batch_gen)
        client.fit(batch_x, batch_y)

        arbiter_channel.send(client_name+"_param", client.get_theta())
        client.set_theta(arbiter_channel.recv("global_param"))
        
        # metrics log
        if config['mode'] == 'Paillier':
            client.send_loss(x, y)
        else:
            client.metrics_log(x, y)
        
        # check convergence
        if check_convergence:
            if arbiter_channel.recv('convergence') == 'YES':
                log_handler.info("-------- end at iteration {} --------".format(i+1))
                break
    
    if config['mode'] == 'DPSGD':
        eps = compute_epsilon(i+1, num_train_examples, config)
        arbiter_channel.send(client_name+"_eps", eps)
        log_handler.info('For delta={}, the current epsilon is: {:.2f}'.format(config['delta'], eps))
    elif config['mode'] == 'Paillier':
        client.set_theta(arbiter_channel.recv("global_param"))
        client.metrics_log(x, y)

    log_handler.info("{} training process done.".format(client_name))
    model_file_path = ph.context.Context.get_model_file_path() + "." + client_name
    log_handler.info("Current model file path is: {}".format(model_file_path))

    model = {
        'feature_names': config['feature_names'],
        'data_max': data_max,
        'data_min': data_min,
        'theta': client.theta,
    }
    with open(model_file_path, 'wb') as fm:
        pickle.dump(model, fm)


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


def run_party(party_name, config):
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
        run_homo_lr_arbiter(config,
                            role_node_map,
                            node_addr_map,
                            log_handler=fl_console_log)
    else:
        run_homo_lr_client(config,
                           role_node_map,
                           node_addr_map,
                           data_key,
                           client_name=party_name,
                           log_handler=fl_console_log)

    fl_console_log.info("Finish homo-LR {} logic.".format(party_name))


#@ph.context.function(role='arbiter',
#                     protocol='lr',
#                     datasets=['train_homo_lr'],
#                     port='9010',
#                     task_type="lr-train")
#def run_arbiter_party():
#    run_party('arbiter', config)
#
#
#@ph.context.function(role='host',
#                     protocol='lr',
#                     datasets=['train_homo_lr_host'],
#                     port='9020',
#                     task_type="lr-train")
#def run_host_party():
#    run_party('host', config)
#
#
#@ph.context.function(role='guest',
#                     protocol='lr',
#                     datasets=['train_homo_lr_guest'],
#                     port='9030',
#                     task_type="lr-train")
#def run_guest_party():
#    run_party('guest', config)
