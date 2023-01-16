import pickle
import os
import numpy as np
import pandas as pd
import primihub as ph
from primihub import dataset
import logging
from primihub.FL.model.logistic_regression.homo_lr import read_data

#def read_data(dataset_key):
#    x = ph.dataset.read(dataset_key).df_data
#    if 'id' in x.columns:
#        x.pop('id')
#    y = x.pop('y').values
#    x = x.to_numpy()
#    return x, y


def sigmoid(x):
    return 1.0 / (1.0 + np.exp(-x))


def predict(model, x): 
    prob = sigmoid(x.dot(model[1:]) + model[0])
    return (prob > 0.5).astype('int')


class ModelInfer:
    def __init__(self, model_path, input_file, output_path, model_type="Homo-LR") -> None:
        model_f = open(model_path, 'rb')
        self.model = np.array(pickle.load(model_f))
        x, y = read_data(input_file)
        self.x = x
        self.type = model_type
        self.out = output_path

    def infer(self):
        if self.type == "Homo-LR":
            preds = predict(self.model, self.x)

        dir_name = os.path.dirname(self.out)

        if not os.path.exists(dir_name):
            os.makedirs(dir_name)

        pd.DataFrame(preds).to_csv(self.out, index=False)
        return preds


infer_data = ['test_homo_lr']


@ph.context.function(role='host', protocol='lr-infer', datasets=infer_data, port='9020', task_type="lr-regression-infer")
def run_infer():
    logging.info("Start machine learning inferring.")
    predict_file_path = ph.context.Context.get_predict_file_path()
    model_file_path = ph.context.Context.get_model_file_path()

    logging.info("Model file path is: {}".format(model_file_path))
    logging.info("Predict file path is: {}".format(predict_file_path))

    mli = ModelInfer(model_file_path, infer_data[0], predict_file_path)

    preds = mli.infer()

    logging.info(
        "Finish machine learning inferring. And the result is {}".format(preds))
