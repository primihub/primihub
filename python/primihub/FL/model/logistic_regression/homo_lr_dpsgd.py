import primihub as ph
import logging
from primihub import dataset, context

from os import path
import json
import os
from phe import paillier
import pickle
from primihub.FL.model.logistic_regression.vfl.evaluation_lr import evaluator

from primihub.FL.model.logistic_regression.homo_lr_base import LRModel
from primihub.FL.model.logistic_regression.homo_lr import read_data, Client, run_party


class LRModel_DPSGD(LRModel):

    def __init__(self, X, y, category, learning_rate=0.2, alpha=0.0001, 
                    noise_multiplier=1.0, l2_norm_clip=1.0, secure_mode=False):
        super().__init__(X, y, category, learning_rate=0.2, alpha=0.0001)
        self.noise_multiplier = noise_multiplier
        self.l2_norm_clip = l2_norm_clip
        self.secure_mode = secure_mode

    def set_noise_multiplier(self, noise_multiplier):
        self.noise_multiplier = noise_multiplier

    def set_l2_norm_clip(self, l2_norm_clip):
        self.l2_norm_clip = l2_norm_clip
    
    def compute_grad(self, x, y):
        batch_size = x.shape[0]
        
        temp = self.predict_prob(x) - y
        batch_grad = np.hstack([np.expand_dims(temp, axis=1),
                                x * np.expand_dims(temp, axis=1)])

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
        

import numpy as np
import pandas as pd
import copy
from primihub.FL.proxy.proxy import ServerChannelProxy
from primihub.FL.proxy.proxy import ClientChannelProxy
from os import path
import logging
from primihub.utils.logger_util import FLFileHandler, FLConsoleHandler, FORMAT
import dp_accounting

config = {
    'learning_rate': 2.0,
    'alpha': 0.0001,
    'noise_multiplier': 2.0,
    'l2_norm_clip': 1.0,
    'batch_size': 50,
    'max_iter': 100,
    'need_one_vs_rest': False,
    'need_encrypt': 'False',
    'category': 2,
    'feature_names': None,
    'delta': 1e-3,
    'secure_mode': True,
}


def compute_epsilon(steps, num_train_examples):
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


class Client_DPSGD(Client, LRModel_DPSGD):

    def __init__(self, X, y, proxy_server, proxy_client_arbiter):
        LRModel_DPSGD.__init__(self, X, y, category=config['category'],
                               learning_rate=config['learning_rate'],
                               alpha=config['alpha'],
                               noise_multiplier=config['noise_multiplier'],
                               l2_norm_clip=config['l2_norm_clip'],
                               secure_mode=config['secure_mode'])
        self.public_key = None
        self.need_encrypt = config['need_encrypt']
        self.need_one_vs_rest = None
        self.flag = True
        self.proxy_server = proxy_server
        self.proxy_client_arbiter = proxy_client_arbiter
    
    def fit(self, X, y):
        if not self.multi_class:
            self.need_one_vs_rest = False
            LRModel_DPSGD.fit(self, X, y)
        else:
            self.need_one_vs_rest = True
            self.one_vs_rest(X, y, category)


def run_homo_lr_client(role_node_map,
                       node_addr_map,
                       data_key,
                       check_convergence,
                       feature_names,
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

    x, y = read_data(data_key, feature_names)
    num_train_examples = x.shape[0]

    client = Client_DPSGD(x, y, proxy_server, proxy_client_arbiter)
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
    
    batch_gen = client.batch_generator([x, y],
                                            config['batch_size'], False)

    for i in range(config['max_iter']):
        log_handler.info("-------- start iteration {} --------".format(i+1))
        
        batch_x, batch_y = next(batch_gen)
        client.fit(batch_x, batch_y)

        proxy_client_arbiter.Remote(client.get_theta(), client_name+"_param")
        client.set_theta(proxy_server.Get(
                        "global_"+client_name+"_model_param"))
 
        if check_convergence:
            if proxy_server.Get('convergence') == 'YES':
                log_handler.info("-------- end at iteration {} --------".format(i+1))
                break
   
    eps = compute_epsilon(i+1, num_train_examples)
    log_handler.info('For delta={}, the current epsilon is: {:.2f}'.format(config['delta'], eps))

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

    proxy_server.StopRecvLoop()


@ph.context.function(role='arbiter',
                     protocol='lr',
                     datasets=['train_homo_lr'],
                     port='9010',
                     task_type="lr-train")
def run_arbiter_party():
    run_party('arbiter', config['feature_names'],
              run_homo_lr_client,
              check_convergence=False)


@ph.context.function(role='host',
                     protocol='lr',
                     datasets=['train_homo_lr_host'],
                     port='9020',
                     task_type="lr-train")
def run_host_party():
    run_party('host', config['feature_names'],
              run_homo_lr_client,
              check_convergence=False)


@ph.context.function(role='guest',
                     protocol='lr',
                     datasets=['train_homo_lr_guest'],
                     port='9030',
                     task_type="lr-train")
def run_guest_party():
    run_party('guest', config['feature_names'],
              run_homo_lr_client,
              check_convergence=False)
