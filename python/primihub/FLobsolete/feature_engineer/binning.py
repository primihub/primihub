import pandas as pd
from sklearn.tree import DecisionTreeClassifier


class BaseBinning:

    def frequency_binning(self, data, bins):
        bin_result = pd.qcut(data, bins)
        return bin_result

    def equidistance_binning(self, data, bins):
        bin_result = pd.cut(data, bins)
        return bin_result

    def optimal_binning_boundary(self, x, y):
        boundary = []

        x = x.fillna(-1).values
        y = y.values

        clf = DecisionTreeClassifier(criterion='entropy',
                                     max_leaf_nodes=6,
                                     min_samples_leaf=0.05)

        clf.fit(x, y)

        n_nodes = clf.tree_.node_count
        children_left = clf.tree_.children_left
        children_right = clf.tree_.children_right
        threshold = clf.tree_.threshold

        for i in range(n_nodes):
            if children_left[i] != children_right[i]:
                boundary.append(threshold[i])

        boundary.sort()

        min_x = x.min()
        max_x = x.max() + 0.1
        boundary = [min_x] + boundary + [max_x]

        return boundary

    def chi_square(self):
        pass
