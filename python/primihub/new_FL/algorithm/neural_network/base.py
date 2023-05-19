import torch
from primihub.utils.logger_util import logger
from opacus.validators import ModuleValidator
from .mlp import NeuralNetwork


def create_model(method, output_dim, device):
    model = NeuralNetwork(output_dim)
    if method == 'DPSGD':
        errors = ModuleValidator.validate(model, strict=False)
        if len(errors) != 0:
            logger.error(errors)
            model = ModuleValidator.fix(model)
    logger.info(model)
    return model.to(device)


def choose_loss_fn(output_dim, task):
    if task == 'classification':
        if output_dim == 1:
            return torch.nn.BCEWithLogitsLoss()
        else:
            return torch.nn.CrossEntropyLoss()
    if task == 'regression':
        return torch.nn.MSELoss()
    else:
        logger.error(f"Not supported task: {task}")


def choose_optimizer(model, optimizer, learning_rate, alpha):
    if optimizer == 'adadelta':
        return torch.optim.Adadelta(model.parameters(),
                                    lr=learning_rate,
                                    weight_decay=alpha)
    elif optimizer == 'adagrad':
        return torch.optim.Adagrad(model.parameters(),
                                   lr=learning_rate,
                                   weight_decay=alpha)
    elif optimizer == 'adam':
        return torch.optim.Adam(model.parameters(),
                                lr=learning_rate,
                                weight_decay=alpha)
    elif optimizer == 'adamw':
        return torch.optim.AdamW(model.parameters(),
                                 lr=learning_rate,
                                 weight_decay=alpha)
    elif optimizer == 'adamax':
        return torch.optim.Adamax(model.parameters(),
                                  lr=learning_rate,
                                  weight_decay=alpha)
    elif optimizer == 'asgd':
        return torch.optim.ASGD(model.parameters(),
                                lr=learning_rate,
                                weight_decay=alpha)
    elif optimizer == 'nadam':
        return torch.optim.NAdam(model.parameters(),
                                 lr=learning_rate,
                                 weight_decay=alpha)
    elif optimizer == 'radam':
        return torch.optim.RAdam(model.parameters(),
                                 lr=learning_rate,
                                 weight_decay=alpha)
    elif optimizer == 'rmsprop':
        return torch.optim.RMSprop(model.parameters(),
                                   lr=learning_rate,
                                   weight_decay=alpha)
    elif optimizer == 'sgd':
        return torch.optim.SGD(model.parameters(),
                               lr=learning_rate,
                               weight_decay=alpha)
    else:
        logger.error(f"Not supported optimizer: {optimizer}")