import numpy as np


# merge fpr and tpr from two clients
def fpr_tpr_merge2(fpr1, tpr1, thresholds1,
                   fpr2, tpr2, thresholds2,
                   num_positive_examples_weights,
                   num_negtive_examples_weights):
    fpr_merge = []
    tpr_merge = []
    thresholds_merge = []

    idx1, idx2 = 0, 0
    fpr1_prev, tpr1_prev, fpr2_prev, tpr2_prev = 0, 0, 0, 0

    # merge two descending thresholds arrays
    while idx1 < len(thresholds1) or idx2 < len(thresholds2):
        if idx1 == len(thresholds1):
            fpr2_prev = fpr2[idx2]
            tpr2_prev = tpr2[idx2]
            thresholds_merge.append(thresholds2[idx2])
            idx2 += 1
        elif idx2 == len(thresholds2):
            fpr1_prev = fpr1[idx1]
            tpr1_prev = tpr1[idx1]
            thresholds_merge.append(thresholds1[idx1])
            idx1 += 1
        else:
            # handle equally scored instances (thresholds)
            score = max(thresholds1[idx1], thresholds2[idx2])
            if score <= thresholds1[idx1]:
                fpr1_prev = fpr1[idx1]
                tpr1_prev = tpr1[idx1]
                thresholds_merge.append(thresholds1[idx1])
                idx1 += 1
            if score <= thresholds2[idx2]:
                fpr2_prev = fpr2[idx2]
                tpr2_prev = tpr2[idx2]
                thresholds_merge.append(thresholds2[idx2])
                idx2 += 1

        # compute weighted averaged metrics
        fpr = np.average([fpr1_prev, fpr2_prev],
                         weights=num_negtive_examples_weights)
        tpr = np.average([tpr1_prev, tpr2_prev],
                         weights=num_positive_examples_weights)
        fpr_merge.append(fpr)
        tpr_merge.append(tpr)

    return fpr_merge, tpr_merge, thresholds_merge


def ks_from_fpr_tpr(fpr, tpr):
    return max(np.array(tpr) - np.array(fpr))


def trapezoid_area(x1, x2, y1, y2):
    base = x2 - x1
    height_avg = (y1 + y2) / 2
    return base * height_avg


def auc_from_fpr_tpr(fpr, tpr):
    area = 0
    for i in range(len(fpr) - 1):
        if fpr[i] != fpr[i + 1]:
            area += trapezoid_area(fpr[i], fpr[i + 1], tpr[i], tpr[i + 1])
    return area
