import primihub as ph
from primihub.FL.model.logistic_regression.homo_lr import run_party


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


@ph.context.function(role='arbiter',
                     protocol='lr',
                     datasets=['train_homo_lr'],
                     port='9010',
                     task_type="lr-train")
def run_arbiter_party():
    run_party('arbiter', config,
              check_convergence=False)


@ph.context.function(role='host',
                     protocol='lr',
                     datasets=['train_homo_lr_host'],
                     port='9020',
                     task_type="lr-train")
def run_host_party():
    run_party('host', config,
              check_convergence=False)


@ph.context.function(role='guest',
                     protocol='lr',
                     datasets=['train_homo_lr_guest'],
                     port='9030',
                     task_type="lr-train")
def run_guest_party():
    run_party('guest', config,
              check_convergence=False)
