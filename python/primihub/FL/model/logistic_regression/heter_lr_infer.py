import pickle
import ray
import json
import pandas as pd
import numpy as np
import modin.pandas as md
import primihub as ph
from sklearn import metrics
from primihub import dataset, context
from primihub.utils.net_worker import GrpcServer
from primihub.utils.evaluation import evaluate_ks_and_roc_auc
from primihub.FL.model.logistic_regression.hetero_lr_host import HeterLrHost
from primihub.FL.model.logistic_regression.hetero_lr_guest import HeterLrGuest


class HeteroLrHostInfer(HeterLrHost):

    def __init__(self,
                 learning_rate=0.01,
                 alpha=0.0001,
                 epochs=10,
                 penalty="l2",
                 batch_size=64,
                 optimal_method=None,
                 update_type=None,
                 loss_type='log',
                 random_state=2023,
                 host_channel=None,
                 add_noise=True,
                 tol=0.001,
                 momentum=0.7,
                 n_iter_no_change=5,
                 sample_method="random",
                 sample_ratio=0.5,
                 host_model=None,
                 host_data=None,
                 eval_path=None,
                 output_file=None):
        super().__init__(learning_rate, alpha, epochs, penalty, batch_size,
                         optimal_method, update_type, loss_type, random_state,
                         host_channel, add_noise, tol, momentum,
                         n_iter_no_change, sample_method, sample_ratio)
        self.model = host_model
        self.data = host_data
        self.scaler = None
        self.label = None
        self.eval_path = eval_path
        self.output_file = output_file

    def load_model(self):
        self.weights = self.model['weights']
        self.bias = self.model['bias']
        self.col_names = self.model['columns']
        self.std = self.model['std']

    def preprocess(self):
        if 'y' in self.data.columns:
            self.label = self.data.pop('y')

        if len(self.col_names) > 0:
            self.data = self.data[self.col_names].values

        if self.std is not None:
            self.data = self.std.transform(self.data)

    def predict_raw(self, x):
        host_part = np.dot(x, self.weights) + self.bias
        guest_part = self.channel.recv("guest_part")
        h = host_part + guest_part

        return h

    def run(self):
        self.load_model()
        self.preprocess()
        y_hat = self.predict_raw(self.data)
        pred_y = (self.sigmoid(y_hat) > 0.5).astype('int')

        pred_df = pd.DataFrame({'preds': pred_y, 'probs': self.sigmoid(y_hat)})
        pred_df.to_csv(self.output_file, index=False, sep='\t')

        if self.label is not None:
            acc = sum((pred_y == self.label).astype('int')) / self.data.shape[0]
            ks, auc = evaluate_ks_and_roc_auc(self.label, self.sigmoid(y_hat))
            fpr, tpr, threshold = metrics.roc_curve(self.label,
                                                    self.sigmoid(y_hat))

            evals = {
                "test_acc": acc,
                "test_ks": ks,
                "test_auc": auc,
                "test_fpr": fpr.tolist(),
                "test_tpr": tpr.tolist()
            }

            metrics_buff = json.dumps(evals)

            with open(self.eval_path, 'w') as filePath:
                filePath.write(metrics_buff)
            print("test acc is", evals)


class HeteroLrGuestInfer(HeterLrGuest):

    def __init__(self,
                 learning_rate=0.01,
                 alpha=0.0001,
                 epochs=10,
                 penalty="l2",
                 batch_size=64,
                 optimal_method=None,
                 update_type=None,
                 loss_type='log',
                 random_state=2023,
                 guest_channel=None,
                 add_noise=True,
                 n_iter_no_change=5,
                 momentum=0.7,
                 sample_method="random",
                 guest_model=None,
                 guest_data=None):
        super().__init__(learning_rate, alpha, epochs, penalty, batch_size,
                         optimal_method, update_type, loss_type, random_state,
                         guest_channel, add_noise, n_iter_no_change, momentum,
                         sample_method)
        self.model = guest_model
        self.data = guest_data
        self.scaler = None

    def load_model(self):
        self.weights = self.model['weights']
        self.bias = self.model['bias']
        self.col_names = self.model['columns']
        self.std = self.model['std']

    def preprocess(self):
        if 'y' in self.data.columns:
            self.label = self.data.pop('y')

        if len(self.col_names) > 0:
            self.data = self.data[self.col_names].values

        if self.std is not None:
            self.data = self.std.transform(self.data)

    def predict_raw(self, x):
        guest_part = np.dot(x, self.weights) + self.bias
        self.channel.sender('guest_part', guest_part)

    def run(self):
        self.load_model()
        self.preprocess()
        self.predict_raw(self.data)


@ph.context.function(role='host',
                     protocol='hetero_lr',
                     datasets=['test_hetero_xgb_host'],
                     port='8008',
                     task_type="classification")
def lr_host_infer():
    ray.init()
    role_node_map = ph.context.Context.get_role_node_map()
    node_addr_map = ph.context.Context.get_node_addr_map()
    dataset_map = ph.context.Context.dataset_map

    data_key = list(dataset_map.keys())[0]

    host_nodes = role_node_map["host"]
    host_port = node_addr_map[host_nodes[0]].split(":")[1]
    host_ip = node_addr_map[host_nodes[0]].split(":")[0]

    guest_nodes = role_node_map["guest"]
    guest_ip, guest_port = node_addr_map[guest_nodes[0]].split(":")

    host_channel = GrpcServer(remote_ip=guest_ip,
                              local_ip=host_ip,
                              remote_port=guest_port,
                              local_port=host_port,
                              context=ph.context.Context)

    # model_path = "hetero_lr_host.ml"
    model_path = ph.context.Context.get_model_file_path() + ".host"
    with open(model_path, 'rb') as lr_host_ml:
        host_model = pickle.load(lr_host_ml)

    # data = md.read_csv("/home/primihub/xusong/data/merged_large_host.csv")
    data = ph.dataset.read(dataset_key=data_key).df_data
    indicator_file_path = ph.context.Context.get_indicator_file_path()
    output_file = ph.context.Context.get_predict_file_path()

    heter_lr = HeteroLrHostInfer(host_channel=host_channel,
                                 host_data=data,
                                 host_model=host_model,
                                 eval_path=indicator_file_path,
                                 output_file=output_file)
    heter_lr.run()


@ph.context.function(role='guest',
                     protocol='hetero_lr',
                     datasets=['test_hetero_xgb_guest'],
                     port='9009',
                     task_type="classification")
def lr_guest_infer():
    ray.init()
    role_node_map = ph.context.Context.get_role_node_map()
    node_addr_map = ph.context.Context.get_node_addr_map()
    dataset_map = ph.context.Context.dataset_map

    data_key = list(dataset_map.keys())[0]
    guest_nodes = role_node_map["guest"]
    guest_port = node_addr_map[guest_nodes[0]].split(":")[1]
    guest_ip = node_addr_map[guest_nodes[0]].split(":")[0]

    host_nodes = role_node_map["host"]
    host_ip, host_port = node_addr_map[host_nodes[0]].split(":")

    guest_channel = GrpcServer(remote_ip=host_ip,
                               remote_port=host_port,
                               local_ip=guest_ip,
                               local_port=guest_port,
                               context=ph.context.Context)

    # model_path = "hetero_lr_guest.ml"
    model_path = ph.context.Context.get_model_file_path() + ".guest"

    with open(model_path, 'rb') as lr_guest_ml:
        guest_model = pickle.load(lr_guest_ml)

    # data = md.read_csv("/home/primihub/xusong/data/merged_large_guest.csv")
    data = ph.dataset.read(dataset_key=data_key).df_data
    heter_lr = HeteroLrGuestInfer(guest_channel=guest_channel,
                                  guest_data=data,
                                  guest_model=guest_model)
    heter_lr.run()