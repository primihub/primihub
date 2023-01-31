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
from primihub.FL.model.logistic_regression.homo_lr import Client, run_party


class LRModel_DPSGD(LRModel):

    def __init__(self, X, y, category, learning_rate=0.2, alpha=0.0001, 
                    noise_multiplier=1.0, l2_norm_clip=1.0, secure_mode=True):
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
    'mode': 'DPSGD',
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


@ph.context.function(role='arbiter',
                     protocol='lr',
                     datasets=['train_homo_lr'],
                     port='9010',
                     task_type="lr-train")
def run_arbiter_party():
    run_party('arbiter', config,
              run_homo_lr_client,
              check_convergence=False)


@ph.context.function(role='host',
                     protocol='lr',
                     datasets=['train_homo_lr_host'],
                     port='9020',
                     task_type="lr-train")
def run_host_party():
    run_party('host', config,
              run_homo_lr_client,
              check_convergence=False)


@ph.context.function(role='guest',
                     protocol='lr',
                     datasets=['train_homo_lr_guest'],
                     port='9030',
                     task_type="lr-train")
def run_guest_party():
    run_party('guest', config,
              run_homo_lr_client,
              check_convergence=False)
