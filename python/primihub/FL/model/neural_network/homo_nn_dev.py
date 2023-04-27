import primihub as ph
from primihub import dataset, context

from os import path
import json
import os
from phe import paillier
import pickle

from sklearn import metrics
from primihub.FL.model.metrics.metrics import fpr_tpr_merge2, ks_from_fpr_tpr, auc_from_fpr_tpr
from primihub.utils.net_worker import GrpcServer

import torch
from torch import nn
from torch.utils.data import DataLoader
import torch.utils.data as data_utils
import torch.nn.functional as F
from torchvision.transforms import ConvertImageDtype

from opacus.validators import ModuleValidator
from opacus import PrivacyEngine

import numpy as np
import pandas as pd
from primihub.utils.logger_util import FLFileHandler, FLConsoleHandler, FORMAT


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


def read_image(dataset_key): 
    return ph.dataset.read(dataset_key, transform=ConvertImageDtype(torch.float32))


def create_model(output_dim, config, log_handler):
    if config['task'] == 'regression' or config['task'] == 'classification':
        return config['nn_model'](output_dim)
    else: 
        log_handler.error(f"Not supported task: {config['task']}")


class Arbiter:
    """
    Tips: Arbiter is a trusted third party !!!
    """

    def __init__(self, output_dim, config, device, host_channel, guest_channel, log_handler):
        self.model = create_model(output_dim, config, log_handler).to(device)
        log_handler.info(self.model)
        self.device = device
        self.host_channel = host_channel
        self.guest_channel = guest_channel
        self.log_handler = log_handler
        self.metrics = {}
        self.task = config['task']
        self.output_dim = output_dim

    def model_aggregate(self, host_param, guest_param, param_weights):
        global_param = self.model.state_dict()
        param_weights_sum = param_weights.sum()
        for layer in global_param:
            global_param[layer] = torch.stack((host_param[layer], guest_param[layer]),\
                                    dim=len(global_param[layer].size())) \
                                    @ param_weights / param_weights_sum
        self.model.load_state_dict(global_param)

    def broadcast_global_model_param(self):
        model_state_dict = self.model.state_dict()
        self.host_channel.sender("global_param", model_state_dict)
        self.guest_channel.sender("global_param", model_state_dict)

    def metrics_log(self, num_examples_weights,
                          num_positive_examples_weights,
                          num_negtive_examples_weights):
        if self.task == 'classification':
            # loss
            host_loss = self.host_channel.recv("host_loss")
            guest_loss = self.guest_channel.recv("guest_loss")
            losses = [host_loss, guest_loss]
            loss = np.average(losses, weights=num_examples_weights)
            
            # acc
            host_acc = self.host_channel.recv("host_acc")
            guest_acc = self.guest_channel.recv("guest_acc")
            accs = [host_acc, guest_acc]
            acc = np.average(accs, weights=num_examples_weights)

            if self.output_dim == 1:
                # fpr, tpr
                host_fpr = self.host_channel.recv("host_fpr")
                guest_fpr = self.guest_channel.recv("guest_fpr")
                host_tpr = self.host_channel.recv("host_tpr")
                guest_tpr = self.guest_channel.recv("guest_tpr")
                host_thresholds = self.host_channel.recv("host_thresholds")
                guest_thresholds = self.guest_channel.recv("guest_thresholds")
                fpr, tpr, thresholds = fpr_tpr_merge2(host_fpr, host_tpr, host_thresholds,
                                                      guest_fpr, guest_tpr, guest_thresholds,
                                                      num_positive_examples_weights,
                                                      num_negtive_examples_weights)

                # ks
                ks = ks_from_fpr_tpr(fpr, tpr)

                # auc
                auc = auc_from_fpr_tpr(fpr, tpr)

                self.log_handler.info("loss={}, acc={}, ks={}, auc={}".format(loss, acc, ks, auc))

                self.metrics = {
                    "train_loss": loss,
                    "train_acc": acc,
                    "train_fpr": fpr,
                    "train_tpr": tpr,
                    "train_ks": ks,
                    "train_auc": auc,
                }

            else:
                host_auc = self.host_channel.recv("host_auc")
                guest_auc = self.guest_channel.recv("guest_auc")
                aucs = [host_auc, guest_auc]
                auc = np.average(aucs, weights=num_examples_weights)

                self.log_handler.info("loss={}, acc={}, auc={}".format(loss, acc, auc))

                self.metrics = {
                    "train_loss": loss,
                    "train_acc": acc,
                    "train_auc": auc,
                }

        if self.task == 'regression':
            # mse
            host_mse = self.host_channel.recv("host_mse")
            guest_mse = self.guest_channel.recv("guest_mse")
            mses = [host_mse, guest_mse]
            mse = np.average(mses, weights=num_examples_weights)

            # mae
            host_mae = self.host_channel.recv("host_mae")
            guest_mae = self.guest_channel.recv("guest_mae")
            maes = [host_mae, guest_mae]
            mae = np.average(maes, weights=num_examples_weights)
            
            self.log_handler.info("mse={}, mae={}".format(mse, mae))

            self.metrics = {
                "train_mse": mse,
                "train_mae": mae,
            }

    def get_metrics(self):
        return self.metrics


class Arbiter_DPSGD(Arbiter):

    def __init__(self, output_dim, config, device, host_channel, guest_channel, log_handler):
        super().__init__(output_dim, config, device, host_channel, guest_channel, log_handler)
        errors = ModuleValidator.validate(self.model, strict=False)
        if len(errors) != 0:
            log_handler.info(errors)
            self.model = ModuleValidator.fix(self.model)
            self.model = self.model.to(device)

    def broadcast_global_model_param_prefix(self):
        # opacus make_private will add '_module.' prefix
        model_state_dict = self.model.state_dict(prefix='_module.')
        self.host_channel.sender("global_param", model_state_dict)
        self.guest_channel.sender("global_param", model_state_dict)


def run_homo_nn_arbiter(config,
                        role_node_map,
                        node_addr_map,
                        task_params={},
                        log_handler=None):
    host_nodes = role_node_map["host"]
    guest_nodes = role_node_map["guest"]
    arbiter_nodes = role_node_map["arbiter"]
    eva_type = ph.context.Context.params_map.get("taskType", None)

    if len(host_nodes) != 1:
        log_handler.error("Homo nn only support one host party, but current "
                          "task have {} host party.".format(len(host_nodes)))
        return

    if len(guest_nodes) != 1:
        log_handler.error("Homo nn only support one guest party, but current "
                          "task have {} guest party.".format(len(guest_nodes)))
        return

    if len(arbiter_nodes) != 1:
        log_handler.error(
            "Homo nn only support one arbiter party, but current "
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
    
    # Get cpu or gpu device for training.
    device = "cuda" if torch.cuda.is_available() else "cpu"
    log_handler.info(f"Using {device} device")

    host_file_type = host_channel.recv("host_file_type")
    guest_file_type = guest_channel.recv("guest_file_type")
    if host_file_type == guest_file_type:
        file_type = host_file_type
    else:
        log_handler.error(f"File types don't match: host {host_file_type}, guest {guest_file_type}")

    output_dim = None
    host_output_dim = host_channel.recv("host_output_dim")
    guest_output_dim = guest_channel.recv("guest_output_dim")
    output_dim = max(host_output_dim, guest_output_dim)
    host_channel.sender("global_output_dim", output_dim)
    guest_channel.sender("global_output_dim", output_dim)
    
    host_num_examples = host_channel.recv("host_num_examples")
    guest_num_examples = guest_channel.recv("guest_num_examples")
    num_examples_weights = [host_num_examples, guest_num_examples]
    param_weights = torch.tensor(num_examples_weights, dtype=torch.float32)

    num_positive_examples_weights, num_negtive_examples_weights = None, None
    if config['task'] == 'classification' and output_dim == 1:
        host_num_positive_examples = host_channel.recv("host_num_positive_examples")
        guest_num_positive_examples = guest_channel.recv("guest_num_positive_examples")
        num_positive_examples_weights = [host_num_positive_examples, guest_num_positive_examples]

        host_num_negtive_examples = host_num_examples - host_num_positive_examples
        guest_num_negtive_examples = guest_num_examples - guest_num_positive_examples
        num_negtive_examples_weights = [host_num_negtive_examples, guest_num_negtive_examples]
    
    if config['mode'] == 'Plaintext':
        arbiter = Arbiter(output_dim, config, device, host_channel, guest_channel, log_handler)
    elif config['mode'] == 'DPSGD':
        arbiter = Arbiter_DPSGD(output_dim, config, device, host_channel, guest_channel, log_handler)
    else:
        log_handler.error(f"Not supported mode: {config['mode']}")

    host_input_shape = host_channel.recv("host_input_shape")
    guest_input_shape = guest_channel.recv("guest_input_shape")
    if host_input_shape == guest_input_shape:
        input_shape = host_input_shape
    else:
        log_handler.error(f"Input shapes don't match: host {host_input_shape}, guest {guest_input_shape}")
    input_shape.insert(0, 1) # set batch size equals to 1 to initilize lazy module
    arbiter.model.forward(torch.ones(input_shape))
    arbiter.model.load_state_dict(arbiter.model.state_dict())

    # data preprocessing
    if file_type == 'csv': 
        # minmaxscaler
        host_data_max = host_channel.recv("host_data_max")
        guest_data_max = guest_channel.recv("guest_data_max")
        host_data_min = host_channel.recv("host_data_min")
        guest_data_min = guest_channel.recv("guest_data_min")

        data_max = np.maximum(host_data_max, guest_data_max)
        data_min = np.minimum(host_data_min, guest_data_min)

        host_channel.sender("data_max", data_max)
        guest_channel.sender("data_max", data_max)
        host_channel.sender("data_min", data_min)
        guest_channel.sender("data_min", data_min)

    # broadcast global model 
    arbiter.broadcast_global_model_param()

    for i in range(config['epoch']):
        log_handler.info("-------- start epoch {} --------".format(i+1))
        
        # model training
        host_param = host_channel.recv("host_param")
        guest_param = guest_channel.recv("guest_param")
        arbiter.model_aggregate(host_param, guest_param, param_weights)
        if config['mode'] == 'DPSGD':
            arbiter.broadcast_global_model_param_prefix()
        else:
            arbiter.broadcast_global_model_param()
        
        # metrics log
        arbiter.metrics_log(num_examples_weights,
                            num_positive_examples_weights,
                            num_negtive_examples_weights)

    if config['mode'] == 'DPSGD':
        host_eps = host_channel.recv('host_eps')
        guest_eps = guest_channel.recv('guest_eps')
        eps = max(host_eps, guest_eps)
        log_handler.info('For delta={}, the current epsilon is: {:.2f}'.format(config['delta'], eps))

    indicator_file_path = ph.context.Context.get_indicator_file_path()
    log_handler.info("Current metrics file path is: {}".format(indicator_file_path))

    trainMetrics = arbiter.get_metrics()
    trainMetricsBuff = json.dumps(trainMetrics)
    with open(indicator_file_path, 'w') as filePath:
        filePath.write(trainMetricsBuff)

    log_handler.info("####### start predict ######")
    log_handler.info("All process done.")


def choose_loss_fn(output_dim, config, log_handler):
    if config['task'] == 'classification':
        if output_dim == 1:
            return nn.BCEWithLogitsLoss()
        else:
            return nn.CrossEntropyLoss()
    if config['task'] == 'regression':
        return nn.MSELoss()
    else:
        log_handler.error(f"Not supported task: {config['task']}")
        

def choose_optimizer(model, config, log_handler):
    if config['optimizer'] == 'adadelta':
        return torch.optim.Adadelta(model.parameters(),
                                    lr=config['learning_rate'],
                                    weight_decay=config['alpha'])
    elif config['optimizer'] == 'adagrad':
        return torch.optim.Adagrad(model.parameters(),
                                   lr=config['learning_rate'],
                                   weight_decay=config['alpha'])
    elif config['optimizer'] == 'adam':
        return torch.optim.Adam(model.parameters(),
                                lr=config['learning_rate'],
                                weight_decay=config['alpha'])
    elif config['optimizer'] == 'adamw':
        return torch.optim.AdamW(model.parameters(),
                                 lr=config['learning_rate'],
                                 weight_decay=config['alpha'])
    elif config['optimizer'] == 'adamax':
        return torch.optim.Adamax(model.parameters(),
                                  lr=config['learning_rate'],
                                  weight_decay=config['alpha'])
    elif config['optimizer'] == 'asgd':
        return torch.optim.ASGD(model.parameters(),
                                lr=config['learning_rate'],
                                weight_decay=config['alpha'])
    elif config['optimizer'] == 'nadam':
        return torch.optim.NAdam(model.parameters(),
                                 lr=config['learning_rate'],
                                 weight_decay=config['alpha'])
    elif config['optimizer'] == 'radam':
        return torch.optim.RAdam(model.parameters(),
                                 lr=config['learning_rate'],
                                 weight_decay=config['alpha'])
    elif config['optimizer'] == 'rmsprop':
        return torch.optim.RMSprop(model.parameters(),
                                   lr=config['learning_rate'],
                                   weight_decay=config['alpha'])
    elif config['optimizer'] == 'sgd':
        return torch.optim.SGD(model.parameters(),
                               lr=config['learning_rate'],
                               weight_decay=config['alpha'])
    else:
        log_handler.error(f"Not supported optimizer: {config['optimizer']}")


class Client:

    def __init__(self, output_dim, config, device, arbiter_channel, client_name, log_handler):
        self.model = create_model(output_dim, config, log_handler).to(device)
        log_handler.info(self.model)
        self.device = device
        self.loss_fn = choose_loss_fn(output_dim, config, log_handler)
        self.optimizer = choose_optimizer(self.model, config, log_handler)
        self.arbiter_channel = arbiter_channel
        self.client_name = client_name
        self.log_handler = log_handler
        self.task = config['task']
        self.output_dim = output_dim

    def fit(self, dataloader):
        self.model.train()
        for x, y in dataloader:
            x, y = x.to(self.device), y.to(self.device)

            # Compute prediction error
            pred = self.model(x)
            loss = self.loss_fn(pred, y)

            # Backpropagation
            self.optimizer.zero_grad()
            loss.backward()
            self.optimizer.step()

    def send_metrics(self, dataloader):
        size = len(dataloader.dataset)
        self.model.eval()
        y_true, y_score = torch.tensor([]), torch.tensor([])
        loss, acc = 0, 0
        mse, mae = 0, 0

        with torch.no_grad():
            for x, y in dataloader:
                x, y = x.to(self.device), y.to(self.device)
                pred = self.model(x)

                if self.task == 'classification':
                    y_true = torch.cat((y_true, y))
                    y_score = torch.cat((y_score, pred))
                    loss += self.loss_fn(pred, y).item() * len(x)
                    if self.output_dim == 1:
                        acc += ((pred > 0.).float() == y).type(torch.float).sum().item()
                    else:
                        acc += (pred.argmax(1) == y).type(torch.float).sum().item()
                elif self.task == 'regression':
                    mae += F.l1_loss(pred, y) * len(x)
                    mse += F.mse_loss(pred, y) * len(x)

        if self.task == 'classification':
            # loss
            loss /= size
            self.arbiter_channel.sender(self.client_name+"_loss", loss)

            # acc
            acc /= size
            self.arbiter_channel.sender(self.client_name+"_acc", acc)

            if self.output_dim == 1:
                # fpr, tpr
                y_score = torch.sigmoid(y_score)
                fpr, tpr, thresholds = metrics.roc_curve(y_true, y_score,
                                                         drop_intermediate=False)
                self.arbiter_channel.sender(self.client_name+"_fpr", fpr)
                self.arbiter_channel.sender(self.client_name+"_tpr", tpr)
                self.arbiter_channel.sender(self.client_name+"_thresholds", thresholds)

                # ks
                ks = ks_from_fpr_tpr(fpr, tpr)

                # auc
                auc = metrics.roc_auc_score(y_true, y_score)

                self.log_handler.info("loss={}, acc={}, ks={}, auc={}".format(loss, acc, ks, auc))

            else:
                y_score = torch.softmax(y_score, dim=1)
                auc = metrics.roc_auc_score(y_true, y_score, multi_class='ovr') # one-vs-rest
                self.arbiter_channel.sender(self.client_name+"_auc", auc)

                self.log_handler.info("loss={}, acc={}, auc={}".format(loss, acc, auc))

        elif self.task == 'regression':
            # mse
            mse /= size
            self.arbiter_channel.sender(self.client_name+"_mse", mse)

            # mae
            mae /= size
            self.arbiter_channel.sender(self.client_name+"_mae", mae)

            self.log_handler.info("mse={}, mae={}".format(mse, mae))

    def get_param(self):
        return self.model.state_dict()

    def set_param(self, model_state_dict):
        self.model.load_state_dict(model_state_dict)


class Client_DPSGD(Client):

    def __init__(self, output_dim, config, device, arbiter_channel, client_name, log_handler):
        super().__init__(output_dim, config, device, arbiter_channel, client_name, log_handler)
        errors = ModuleValidator.validate(self.model, strict=False)
        if len(errors) != 0:
            log_handler.info(errors)
            self.model = ModuleValidator.fix(self.model)
            self.model = self.model.to(device)
        self.noise_multiplier = config['noise_multiplier']
        self.max_grad_norm = config['max_grad_norm']
        self.privacy_engine = PrivacyEngine(accountant='rdp')
        self.train_dataloader = None

    def enable_DP_training(self, train_dataloader):
        self.model, self.optimizer, self.train_dataloader = self.privacy_engine.make_private(
            module=self.model,
            optimizer=self.optimizer,
            data_loader=train_dataloader,
            noise_multiplier=self.noise_multiplier,
            max_grad_norm=self.max_grad_norm,
        )

    def get_param(self):
        # remove '_module.' prefix added by opacus make_private
        return self.model._module.state_dict()


def run_homo_nn_client(config,
                       role_node_map,
                       node_addr_map,
                       data_key,
                       file_type,
                       client_name,
                       task_params={},
                       log_handler=None):
    client_nodes = role_node_map[client_name]
    arbiter_nodes = role_node_map["arbiter"]

    if len(client_nodes) != 1:
        log_handler.error("Homo nn only support one {0} party, but current "
                          "task have {1} {0} party.".format(client_name, len(client_nodes)))
        return

    if len(arbiter_nodes) != 1:
        log_handler.error("Homo nn only support one arbiter party, but current "
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
    
    # Get cpu or gpu device for training.
    device = "cuda" if torch.cuda.is_available() else "cpu"
    log_handler.info(f"Using {device} device")

    arbiter_channel.sender(client_name+"_file_type", file_type)
    
    if file_type == 'csv': 
        x, y = read_data(data_key, config['feature_names'])
        num_examples = x.shape[0]
    elif file_type == 'image': 
        data_tensor = read_image(data_key)
        num_examples = data_tensor.__len__()
    else:
        log_handler.error(f"Not supported file_type: {config['file_type']}")
    
    output_dim = None
    if config['task'] == 'regression':
        output_dim = 1
    elif config['task'] == 'classification':
        if file_type == 'csv':
            output_dim = y.max() + 1 # assume labels start from 0
        if file_type == 'image':
            output_dim = max(data_tensor.img_labels['y']) + 1

        if output_dim == 2:
            output_dim = 1 # binary classification
    else:
        log_handler.error(f"Not supported task: {config['task']}")

    arbiter_channel.sender(client_name+"_output_dim", output_dim)
    output_dim = arbiter_channel.recv("global_output_dim")
    
    arbiter_channel.sender(client_name+"_num_examples", num_examples)
    if config['task'] == 'classification' and output_dim == 1:
        num_positive_examples = y.sum()
        arbiter_channel.sender(client_name+"_num_positive_examples", num_positive_examples)
    
    if output_dim == 1:
        y = torch.tensor(y.reshape(-1, 1), dtype=torch.float32)
    else:
        if file_type == 'csv':
            y = torch.tensor(y)
    
    if config['mode'] == 'Plaintext':
        client = Client(output_dim, config, device, arbiter_channel, client_name, log_handler)
    elif config['mode'] == 'DPSGD':
        client = Client_DPSGD(output_dim, config, device, arbiter_channel, client_name, log_handler)
    else:
        log_handler.error(f"Not supported mode: {config['mode']}")

    if file_type == 'csv':
        input_shape = list(x[0].shape)
    if file_type == 'image':
        input_shape = list(data_tensor[0][0].size())
    arbiter_channel.sender(client_name+"_input_shape", input_shape)
    
    # data preprocessing 
    if file_type == 'csv': 
        # minmaxscaler
        data_max = x.max(axis=0)
        data_min = x.min(axis=0)

        arbiter_channel.sender(client_name+"_data_max", data_max)
        arbiter_channel.sender(client_name+"_data_min", data_min)

        data_max = arbiter_channel.recv("data_max")
        data_min = arbiter_channel.recv("data_min")

        x = (x - data_min) / (data_max - data_min)
    
        # Create data loaders
        x = torch.tensor(x, dtype=torch.float32)
        data_tensor = data_utils.TensorDataset(x, y)

    train_dataloader = DataLoader(data_tensor, batch_size=config['batch_size'], shuffle=True)
    
    # client models initilization
    if config['mode'] == 'DPSGD':
        input_shape.insert(0, 1) # set batch size equals to 1 to initilize lazy module
        client.model.forward(torch.ones(input_shape))
    client.set_param(arbiter_channel.recv("global_param"))
    if config['mode'] == 'DPSGD':
        client.enable_DP_training(train_dataloader)
 
    for i in range(config['epoch']):
        log_handler.info("-------- start epoch {} --------".format(i+1))
        
        # model training
        if config['mode'] == 'DPSGD':
            # DP data loader: Poisson sampling 
            client.fit(client.train_dataloader)
        else:
            client.fit(train_dataloader)

        arbiter_channel.sender(client_name+"_param", client.get_param())
        client.set_param(arbiter_channel.recv("global_param"))
        
        # metrics log
        client.send_metrics(train_dataloader)

    if config['mode'] == 'DPSGD':
        delta = config['delta']
        if delta >= 1. / num_examples:
            log_handler.error(f"delta {delta} should be set less than 1 / {num_examples}")
        eps = client.privacy_engine.get_epsilon(delta)
        arbiter_channel.sender(client_name+"_eps", eps)
        log_handler.info('For delta={}, the current epsilon is: {:.2f}'.format(delta, eps))
        
    log_handler.info("{} training process done.".format(client_name))
    model_file_path = ph.context.Context.get_model_file_path() + "." + client_name
    log_handler.info("Current model file path is: {}".format(model_file_path))

    model = {
        'feature_names': config['feature_names'],
        'param': client.get_param(),
        'output_dim': output_dim,
        'task': config['task'],
    } 
    if file_type == 'csv': 
        model['data_max'] = data_max
        model['data_min'] = data_min
    with open(model_file_path, 'wb') as fm:
        pickle.dump(model, fm)


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
    file_type = eval(dataset_map[data_key])['type']

    fl_console_log.debug("role_nodeid_map {}".format(role_node_map))

    fl_console_log.debug("node_addr_map {}".format(node_addr_map))
    fl_console_log.info("Start homo-NN {} logic.".format(party_name))
    
    if party_name == 'arbiter':
        run_homo_nn_arbiter(config,
                            role_node_map,
                            node_addr_map,
                            log_handler=fl_console_log)
    else:
        run_homo_nn_client(config,
                           role_node_map,
                           node_addr_map,
                           data_key,
                           file_type,
                           client_name=party_name,
                           log_handler=fl_console_log)

    fl_console_log.info("Finish homo-NN {} logic.".format(party_name))
