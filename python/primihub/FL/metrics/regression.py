from primihub.utils.logger_util import logger
from sklearn.metrics import (
    explained_variance_score,
    max_error,
    mean_absolute_error,
    mean_squared_error,
    mean_squared_log_error,
    median_absolute_error,
    mean_absolute_percentage_error,
    r2_score,
)


def regression_metrics(y_true,
                       y_pred,
                       prefix="",
                       metircs_name=["ev",
                                     "maxe",
                                     "mae",
                                     "mse",
                                     "rmse",
                                     "msle",
                                     "medae",
                                     "mape",
                                     "r2",],):
    metrics = {}

    for name in metircs_name:
        prefix_name = prefix + name
        if name == "ev":
            metrics[prefix_name] = explained_variance_score(y_true, y_pred)
        elif name == "maxe":
            metrics[prefix_name] = max_error(y_true, y_pred)
        elif name == "mae":
            metrics[prefix_name] = mean_absolute_error(y_true, y_pred)
        elif name == "mse":
            metrics[prefix_name] = mean_squared_error(y_true, y_pred, squared=True)
        elif name == "rmse":
            metrics[prefix_name] = mean_squared_error(y_true, y_pred, squared=False)
        elif name == "msle":
            metrics[prefix_name] = mean_squared_log_error(y_true, y_pred)
        elif name == "medae":
            metrics[prefix_name] = median_absolute_error(y_true, y_pred)
        elif name == "mape":
            metrics[prefix_name] = mean_absolute_percentage_error(y_true, y_pred)
        elif name == "r2":
            metrics[prefix_name] = r2_score(y_true, y_pred)
        else:
            raise ValueError(f"Unsupported metrics: {name}")
        
        logger.info(f"{name}: {metrics[prefix_name]}")
        
    return metrics