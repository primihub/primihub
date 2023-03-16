import pandas as pd
import numpy as np
from scipy.stats import ks_2samp
from sklearn.metrics import roc_auc_score, accuracy_score, precision_score, recall_score, f1_score


def eval_acc(y_real, pred_y):
    acc = accuracy_score(y_real, pred_y)
    precision = precision_score(y_real, pred_y)
    recall = recall_score(y_real, pred_y)
    f1 = f1_score(y_real, pred_y)

    return {'acc': acc, 'precision': precision, 'recall': recall, 'f1': f1}


def evaluate_ks_and_roc_auc(y_real, y_proba):
    # Unite both visions to be able to filter
    df = pd.DataFrame()

    if isinstance(y_real, np.ndarray) or isinstance(y_real, list):
        df['real'] = y_real
    else:
        df['real'] = y_real.values

    if isinstance(y_proba, np.ndarray) or isinstance(y_proba, list):
        df['proba'] = y_proba
    else:
        df['proba'] = y_proba.values

    # Recover each class
    class0 = df[df['real'] == 0]
    class1 = df[df['real'] == 1]

    ks = ks_2samp(class0['proba'], class1['proba'])
    roc_auc = roc_auc_score(df['real'], df['proba'])

    print(f"KS: {ks.statistic:.4f} (p-value: {ks.pvalue:.3e})")
    print(f"ROC AUC: {roc_auc:.4f}")
    return ks.statistic, roc_auc


def plot_lift_and_gain(y_real, y_proba):
    merge_df = pd.DataFrame()

    if isinstance(y_real, np.ndarray) or isinstance(y_real, list):
        merge_df['y_real'] = y_real
    else:
        merge_df['y_real'] = y_real.values

    if isinstance(y_proba, np.ndarray) or isinstance(y_proba, list):
        merge_df['y_proba'] = y_proba
    else:
        merge_df['y_proba'] = y_proba.values

    sorted_merge_df = merge_df.sort_values('y_proba', ascending=False)
    total_positive_ratio = sum(
        sorted_merge_df['y_real'] == 1) / sorted_merge_df.shape[0]

    total_positive_count = sum(sorted_merge_df['y_real'] == 1)
    subs_pos_count = 0

    gains = []
    lifts = []

    for i in sorted_merge_df.index:
        # calculate tmp_lift
        thres = sorted_merge_df.loc[i]['y_proba']
        subs = sorted_merge_df[sorted_merge_df['y_proba'] >= thres]
        subs_pos_rate = sum(subs['y_real'] == 1) / subs.shape[0]

        tmp_lift = subs_pos_rate / total_positive_ratio
        lifts.append(tmp_lift)

        # calculate tmp_gain
        if sorted_merge_df.loc[i]['y_real'] == 1:
            subs_pos_count += 1

        gain = subs_pos_count / total_positive_count
        gains.append(gain)

    x = np.arange(len(lifts)) / len(lifts)
    plot_lift = dict({'axis_x': x, 'axis_y': lifts})
    plot_gain = dict({'axis_x': x, 'axis_y': gains})
    return plot_lift, plot_gain
