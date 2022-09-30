import pickle
import numpy as np
import pandas as pd
import primihub as ph
from primihub import dataset
import logging


def sigmoid(x):
    return 1.0 / (1.0 + np.exp(-x))


def predict_prob(weights, bias, x):
    prob = sigmoid(np.dot(x, weights)+bias)
    return prob


def predict(model, x):
    bias = model[-1]
    weights = model[:-1]
    prob = predict_prob(weights, bias, x)
    return (prob > 0.5).astype('int')


class ModelInfer:
    def __init__(self, model_path, input_file, output_path, model_type="Homo-LR") -> None:
        model_f = open(model_path, 'rb')
        self.model = np.array(pickle.load(model_f))
        # self.arr = pd.read_csv(input_path, header=None).values
        data = dataset.read(dataset_key=input_file).df_data
        self.arr = data.values
        self.type = model_type
        self.out = output_path

    def infer(self):
        if self.type == "Homo-LR":
            preds = predict(self.model, self.arr)

        pd.DataFrame(preds).to_csv(self.out, index=False)
        return preds


infer_data = ['homo_lr_test']


@ph.context.function(role='host', protocol='lr-infer', datasets=infer_data, port='9020', task_type="lr-regression-infer")
def run_infer():
    logging.info("Start machine learning inferring.")
    predict_file_path = ph.context.Context.get_predict_file_path()
    model_file_path = ph.context.Context.get_model_file_path()

    mli = ModelInfer(model_file_path, infer_data[0], predict_file_path)

    preds = mli.infer()

    logging.info(
        f"Finish machine learning inferring. And the result is {preds}")
