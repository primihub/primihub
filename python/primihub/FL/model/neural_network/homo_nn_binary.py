import primihub as ph
from primihub.FL.model.neural_network.homo_nn_dev import run_party

from primihub.FL.model.neural_network.mlp import NeuralNetwork


config = {
    'mode': 'Plaintext',
    'nn_model': NeuralNetwork,
    'task': 'classification',
    'learning_rate': 1e-2,
    'alpha': 1e-4,
    'optimizer': 'adam',
    'batch_size': 100,
    'epoch': 100,
    'feature_names': None,
}


# binary classification dataset
@ph.context.function(role='arbiter',
                     protocol='nn',
                     datasets=['train_homo_lr'],
                     port='9010',
                     task_type="nn-train")
def run_arbiter_party():
    run_party('arbiter', config)


@ph.context.function(role='host',
                     protocol='nn',
                     datasets=['train_homo_lr_host'],
                     port='9020',
                     task_type="nn-train")
def run_host_party():
    run_party('host', config)


@ph.context.function(role='guest',
                     protocol='nn',
                     datasets=['train_homo_lr_guest'],
                     port='9030',
                     task_type="nn-train")
def run_guest_party():
    run_party('guest', config)
