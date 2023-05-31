import pickle
import json
import pandas as pd
import numpy as np
from sklearn import metrics
from primihub.FL.algorithm.utils.net_work import GrpcClient
from primihub.utils.evaluation import evaluate_ks_and_roc_auc, plot_lift_and_gain, eval_acc
from primihub.FL.algorithm.utils.base import BaseModel
from primihub.FL.algorithm.utils.dataset import read_csv
from primihub.FL.algorithm.utils.file import check_directory_exist


class HeteroLrHostInfer(BaseModel):

    def __init__(self, **kwargs):
        super().__init__(**kwargs)
        self.set_inputs()
        remote_party = self.roles[self.role_params['others_role']][0]
        self.channel = GrpcClient(local_party=self.role_params['self_name'],
                                    remote_party=remote_party,
                                    node_info=self.node_info,
                                    task_info=self.task_info)

    def set_inputs(self):
        # set common params
        self.model = self.common_params['model']
        self.task_name = self.common_params['task_name']
        self.metric_path = self.common_params['metric_path']
        self.model_pred = self.common_params['model_pred']

        # set role params
        self.data_set = self.role_params['data_set']
        self.id = self.role_params['id']
        self.selected_column = self.role_params['selected_column']
        self.label = self.role_params['label']
        self.model_path = self.role_params['model_path']

        # read from data path
        data_path = self.role_params['data']['data_path']
        self.data = read_csv(data_path, selected_column=None, id=None)

    def load_dict(self):
        with open(self.model_path, "rb") as current_model:
            model_dict = pickle.load(current_model)

        self.weights = model_dict['weights']
        self.bias = model_dict['bias']
        self.col_names = model_dict['columns']
        self.std = model_dict['std']

    def preprocess(self):
        if self.id in self.data.columns:
            self.data.pop(self.id)

        if self.label in self.data.columns:
            self.y = self.data.pop(self.label).values

        if len(self.col_names) > 0:
            self.data = self.data[self.col_names].values

        if self.std is not None:
            self.data = self.std.transform(self.data)

    def predict_raw(self, x):
        host_part = np.dot(x, self.weights) + self.bias
        guest_part = self.channel.recv("guest_part")
        h = host_part + guest_part

        return h

    def sigmoid(self, x):
        return 1.0 / (1 + np.exp(-x))

    def run(self):
        origin_data = self.data.copy()
        self.load_dict()
        self.preprocess()
        y_hat = self.predict_raw(self.data)
        pred_prob = self.sigmoid(y_hat)
        pred_y = (pred_prob > 0.5).astype('int')

        pred_df = pd.DataFrame({'pred_prob': pred_prob,
                                'pred_y': pred_y})
        data_result = pd.concat([origin_data, pred_df], axis=1)
        check_directory_exist(self.model_pred)
        data_result.to_csv(self.model_pred, index=False)

        # if self.label is not None:
        #     acc = sum((pred_y == self.y).astype('int')) / self.data.shape[0]
        #     ks, auc = evaluate_ks_and_roc_auc(self.y, self.sigmoid(y_hat))
        #     fpr, tpr, threshold = metrics.roc_curve(self.y, self.sigmoid(y_hat))

        #     evals = {
        #         "test_acc": acc,
        #         "test_ks": ks,
        #         "test_auc": auc,
        #         "test_fpr": fpr.tolist(),
        #         "test_tpr": tpr.tolist()
        #     }

        #     metrics_buff = json.dumps(evals)

        #     check_directory_exist(self.metric_path)
        #     with open(self.metric_path, 'w') as filePath:
        #         filePath.write(metrics_buff)
        #     print("test acc is", evals)

    def get_summary(self):
        return {}

    def get_outputs(self):
        return {}

    def get_status(self):
        return {}


class HeteroLrGuestInfer(BaseModel):

    def __init__(self, **kwargs):
        super().__init__(**kwargs)
        self.set_inputs()
        remote_party = self.roles[self.role_params['others_role']][0]
        self.channel = GrpcClient(local_party=self.role_params['self_name'],
                                    remote_party=remote_party,
                                    node_info=self.node_info,
                                    task_info=self.task_info)

    def set_inputs(self):
        self.model = self.common_params['model']
        self.task_name = self.common_params['task_name']

        # set role params
        self.data_set = self.role_params['data_set']
        self.id = self.role_params['id']
        self.selected_column = self.role_params['selected_column']
        self.label = self.role_params['label']
        self.model_path = self.role_params['model_path']

        # read from data path
        data_path = self.role_params['data']['data_path']
        self.data = read_csv(data_path, selected_column=None, id=None)

    def load_dict(self):
        with open(self.model_path, "rb") as current_model:
            model_dict = pickle.load(current_model)

        self.weights = model_dict['weights']
        self.bias = model_dict['bias']
        self.col_names = model_dict['columns']
        self.std = model_dict['std']

    def preprocess(self):
        if self.id in self.data.columns:
            self.data.pop(self.id)

        if self.label in self.data.columns:
            self.y = self.data.pop(self.label).values

        if len(self.col_names) > 0:
            self.data = self.data[self.col_names].values

        if self.std is not None:
            self.data = self.std.transform(self.data)

    def predict_raw(self, x):
        guest_part = np.dot(x, self.weights) + self.bias
        self.channel.send("guest_part", guest_part)

    def run(self):
        self.load_dict()
        self.preprocess()
        self.predict_raw(self.data)

    def get_summary(self):
        return {}

    def get_outputs(self):
        return {}

    def get_status(self):
        return {}