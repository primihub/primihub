from primihub.utils.logger_util import logger
import numpy as np
from scipy.stats import ks_2samp
from sklearn.metrics import (
    accuracy_score,
    f1_score,
    precision_score,
    recall_score,
    roc_auc_score,
    roc_curve,
)


def classification_metrics(y_true,
                           y_score,
                           multiclass=False,
                           prefix="",
                           metircs_name=["acc",
                                         "f1",
                                         "precision",
                                         "recall",
                                         "auc",
                                         "roc",
                                         "ks",],):
    metrics = {}

    if multiclass:
        y_pred = np.argmax(y_score, axis=1)
    else:
        y_pred = np.array(y_score > 0.5, dtype='int')

    for name in metircs_name:
        prefix_name = prefix + name
        if name == "acc":
            metrics[prefix_name] = accuracy_score(y_true, y_pred)
        elif name == "f1":
            if multiclass:
                metrics[prefix_name] = f1_score(y_true, y_pred, average="macro")
            else:
                metrics[prefix_name] = f1_score(y_true, y_pred, average="binary")
        elif name == "precision":
            if multiclass:
                metrics[prefix_name] = precision_score(y_true, y_pred, average="macro")
            else:
                metrics[prefix_name] = precision_score(y_true, y_pred, average="binary")
        elif name == "recall":
            if multiclass:
                metrics[prefix_name] = recall_score(y_true, y_pred, average="macro")
            else:
                metrics[prefix_name] = recall_score(y_true, y_pred, average="binary")
        elif name == "auc":
            metrics[prefix_name] = roc_auc_score(y_true, y_score, multi_class="ovr")
        elif name == "roc" and not multiclass:
            fpr, tpr, thresholds = roc_curve(y_true, y_score)
            # thresholds[0] is np.inf, but is not a valid JSON value
            thresholds[0] = thresholds[1] + 1.
            metrics[prefix + "fpr"] = fpr.tolist()
            metrics[prefix + "tpr"] = tpr.tolist()
            metrics[prefix + "thresholds"] = thresholds.tolist()
        elif name == "ks" and not multiclass:
            pos_idx = y_true == 1
            metrics[prefix_name] = ks_2samp(y_score[pos_idx],
                                            y_score[~pos_idx]).statistic
        else:
            raise ValueError(f"Unsupported metrics: {name}")
        
        if name == "roc":
            logger.info(f"{name} is computed but not printed")
        else:
            logger.info(f"{name}: {metrics[prefix_name]}")
        
    return metrics