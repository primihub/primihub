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


# Tom Fawcett. An introduction to ROC analysis.
# Pattern Recognition Letters, 2006.
# https://doi.org/10.1016/j.patrec.2005.10.010


def roc_vertical_avg(samples, FPR, TPR):
    nrocs = len(FPR)
    tpravg = []
    fpr = [i / samples for i in range(samples + 1)]

    for fpr_sample in fpr:
        tprsum = 0

        for i in range(nrocs):
            tprsum += tpr_for_fpr(fpr_sample, FPR[i], TPR[i])

        tpravg.append(tprsum / nrocs)

    return fpr, tpravg


def tpr_for_fpr(fpr_sample, fpr, tpr):
    i = 0
    while i < len(fpr) - 1 and fpr[i + 1] <= fpr_sample:
        i += 1

    if fpr[i] == fpr_sample:
        return tpr[i]
    else:
        return interpolate(fpr[i], tpr[i], fpr[i + 1], tpr[i + 1], fpr_sample)


def interpolate(fprp1, tprp1, fprp2, tprp2, x):
    slope = (tprp2 - tprp1) / (fprp2 - fprp1)
    return tprp1 + slope * (x - fprp1)


def roc_threshold_avg(samples, FPR, TPR, THRESHOLDS):
    nrocs = len(FPR)
    T = []
    fpravg = []
    tpravg = []

    for thresholds in THRESHOLDS:
        for t in thresholds:
            T.append(t)
    T.sort(reverse=True)

    for tidx in range(0, len(T), int(len(T) / samples)):
        fprsum = 0
        tprsum = 0

        for i in range(nrocs):
            fprp, tprp = roc_point_at_threshold(FPR[i], TPR[i], THRESHOLDS[i],
                                                T[tidx])
            fprsum += fprp
            tprsum += tprp

        fpravg.append(fprsum / nrocs)
        tpravg.append(tprsum / nrocs)

    return fpravg, tpravg


def roc_point_at_threshold(fpr, tpr, thresholds, thresh):
    i = 0
    while i < len(fpr) - 1 and thresholds[i] > thresh:
        i += 1
    return fpr[i], tpr[i]
    