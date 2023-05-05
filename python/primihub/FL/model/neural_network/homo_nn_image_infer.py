import pickle
import os
import numpy as np
import pandas as pd
import primihub as ph
from primihub import dataset
import logging
import torch
from torch.utils.data import DataLoader
from torchvision.transforms import ConvertImageDtype

from primihub.FL.model.neural_network.cnn import NeuralNetwork


def read_image(dataset_key): 
    return ph.dataset.read(dataset_key, transform=ConvertImageDtype(torch.float32))


class ModelInfer:
    def __init__(self, device, model_path, input_file, output_path) -> None:
        self.device = device
        with open(model_path, 'rb') as model_f:
            model_file = pickle.load(model_f) 
        data = read_image(input_file)

        self.data = DataLoader(data, batch_size=len(data))
        self.out = output_path
        
        output_dim = model_file['output_dim']
        self.model = NeuralNetwork(output_dim).to(device)
        self.model.load_state_dict(model_file['param'])

    def infer(self):
        self.model.eval()
        y_pred = torch.tensor([])
        with torch.no_grad():
            for x, y in self.data:
                x, y = x.to(self.device), y.to(self.device)
                pred = self.model(x)
                pred = torch.softmax(pred, dim=1)
                preds = pred.argmax(1)
                y_pred = torch.cat((y_pred, preds))

        dir_name = os.path.dirname(self.out)

        if not os.path.exists(dir_name):
            os.makedirs(dir_name)

        pd.DataFrame(y_pred).to_csv(self.out, index=False)
        return preds

infer_data = ['test_mnist_host']

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
