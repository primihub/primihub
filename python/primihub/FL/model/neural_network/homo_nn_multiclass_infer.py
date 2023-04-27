import pickle
import os
import numpy as np
import pandas as pd
import primihub as ph
from primihub import dataset
import logging
import torch

from primihub.FL.model.neural_network.mlp import NeuralNetwork


def read_data(dataset_key, feature_names):
    x = ph.dataset.read(dataset_key).df_data
    if 'id' in x.columns:
        x.pop('id')
    if 'y' in x.columns:
        x.pop('y')
    if feature_names != None:
        x = x[feature_names]
    x = x.to_numpy()
    return x


class ModelInfer:
    def __init__(self, device, model_path, input_file, output_path) -> None:
        self.device = device
        with open(model_path, 'rb') as model_f:
            model_file = pickle.load(model_f)
        x = read_data(input_file, model_file['feature_names'])

        # data preprocessing
        # minmaxscaler
        data_max = model_file['data_max']
        data_min = model_file['data_min']
        x = (x - data_min) / (data_max - data_min)

        self.x = torch.tensor(x, dtype=torch.float32).to(device)
        self.out = output_path
        
        self.output_dim = model_file['output_dim']
        self.task = model_file['task']

        self.model = NeuralNetwork(self.output_dim).to(device)
        self.model.load_state_dict(model_file['param'])

    def infer(self):
        self.model.eval()
        with torch.no_grad():
            pred = self.model(self.x)
            
            if self.task == 'classification':
                if self.output_dim == 1:
                    pred = torch.sigmoid(pred)
                    pred = (pred > 0.5).int()
                else:
                    pred = torch.softmax(pred, dim=1)
                    pred = pred.argmax(1)

        dir_name = os.path.dirname(self.out)

        if not os.path.exists(dir_name):
            os.makedirs(dir_name)

        pd.DataFrame(pred).to_csv(self.out, index=False)
        return pred

infer_data = ['test_homo_nn_multiclass_host']

def run_infer(party_name):
    logging.info("Start machine learning inferring.")
    predict_file_path = ph.context.Context.get_predict_file_path()
    model_file_path = ph.context.Context.get_model_file_path() + "." + party_name

    logging.info("Model file path is: {}".format(model_file_path))
    logging.info("Predict file path is: {}".format(predict_file_path))

    # Get cpu or gpu device for inferring.
    device = "cuda" if torch.cuda.is_available() else "cpu"
    logging.info(f"Using {device} device")

    mli = ModelInfer(device, model_file_path, infer_data[0], predict_file_path)

    preds = mli.infer()

    logging.info(
        "Finish machine learning inferring. And the result is {}".format(preds))


@ph.context.function(role='host', protocol='nn', datasets=infer_data, port='9020', task_type="nn-infer")
def run_infer_host():
    run_infer("host")
