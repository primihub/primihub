import primihub as ph

from primihub.FL.model.logistic_regression.homo_lr import run_party

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
