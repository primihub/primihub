import primihub as ph
from primihub.FL.model.neural_network.homo_nn_dev import run_party

from primihub.FL.model.neural_network.mlp import NeuralNetwork


config = {
    'mode': 'DPSGD',
    'delta': 1e-3,
    'max_grad_norm': 1.0,
    'noise_multiplier': 2.0,
    'nn_model': NeuralNetwork,
    'task': 'regression',
    'learning_rate': 3e-2,
    'alpha': 0.,
    'optimizer': 'adam',
    'batch_size': 50,
    'epoch': 100,
    'feature_names': None,
}


# regression dataset
@ph.context.function(role='arbiter',
                     protocol='nn',
                     datasets=['train_homo_regression'],
                     port='9010',
                     task_type="nn-train")
def run_arbiter_party():
    run_party('arbiter', config)


@ph.context.function(role='host',
                     protocol='nn',
                     datasets=['train_homo_regression_host'],
                     port='9020',
                     task_type="nn-train")
def run_host_party():
    run_party('host', config)


@ph.context.function(role='guest',
                     protocol='nn',
                     datasets=['train_homo_regression_guest'],
                     port='9030',
                     task_type="nn-train")
def run_guest_party():
    run_party('guest', config)
