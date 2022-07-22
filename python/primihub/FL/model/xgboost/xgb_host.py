import numpy as np
import pandas as pd
import copy

class XGB_HOST:
    def __init__(self,
                 base_score=0.5,
                 max_depth=3,
                 n_estimators=10,
                 learning_rate=0.1,
                 reg_lambda=1,
                 gamma=0,
                 min_child_sample=None,
                 min_child_weight=1,
                 objective='linear',
                 channel=None,
                 sid=0,
                 record=0,
                 lookup_table=pd.DataFrame(
                     columns=['record_id', 'feature_id', 'threshold_value'])
                 ):

        self.channel = channel
        self.base_score = base_score
        self.max_depth = max_depth
        self.n_estimators = n_estimators
        self.learning_rate = learning_rate
        self.reg_lambda = reg_lambda
        self.gamma = gamma
        self.min_child_sample = min_child_sample
        self.min_child_weight = min_child_weight
        self.objective = objective
        self.sid = sid
        self.record = record
        self.lookup_table = lookup_table
        self.tree_structure = {}
        self.lookup_table_sum = {}

    def _grad(self, y_hat, Y):

        if self.objective == 'logistic':
            y_hat = 1.0 / (1.0 + np.exp(-y_hat))
            return y_hat - Y
        elif self.objective == 'linear':
            return y_hat - Y
        else:
            raise KeyError('objective must be linear or logistic!')

    def _hess(self, y_hat, Y):

        if self.objective == 'logistic':
            y_hat = 1.0 / (1.0 + np.exp(-y_hat))
            return y_hat * (1.0 - y_hat)
        elif self.objective == 'linear':
            return np.array([1] * Y.shape[0])
        else:
            raise KeyError('objective must be linear or logistic!')

    def get_gh(self, y_hat, Y):
        # Calculate the g and h of each sample based on the labels of the local data
        gh = pd.DataFrame(columns=['g', 'h'])
        for i in range(0, Y.shape[0]):
            gh['g'] = self._grad(y_hat, Y)
            gh['h'] = self._hess(y_hat, Y)
        return gh

    def get_GH(self, X):
        # Calculate G_left、G_right、H_left、H_right under feature segmentation
        GH = pd.DataFrame(
            columns=['G_left', 'G_right', 'H_left', 'H_right', 'var', 'cut'])
        i = 0
        for item in [x for x in X.columns if x not in ['g', 'h', 'y']]:
            # Categorical variables using greedy algorithm
            # if len(list(set(X[item]))) < 5:
            for cuts in list(set(X[item])):
                if self.min_child_sample:
                    if (X.loc[X[item] < cuts].shape[0] < self.min_child_sample) \
                            | (X.loc[X[item] >= cuts].shape[0] < self.min_child_sample):
                        continue
                GH.loc[i, 'G_left'] = X.loc[X[item] < cuts, 'g'].sum()
                GH.loc[i, 'G_right'] = X.loc[X[item] >= cuts, 'g'].sum()
                GH.loc[i, 'H_left'] = X.loc[X[item] < cuts, 'h'].sum()
                GH.loc[i, 'H_right'] = X.loc[X[item] >= cuts, 'h'].sum()
                GH.loc[i, 'var'] = item
                GH.loc[i, 'cut'] = cuts
                i = i + 1
            # Continuous variables using approximation algorithm
            # else:
            #     old_list = list(set(X[item]))
            #     new_list = []
            #     # four candidate points
            #     j = int(len(old_list) / 4)
            #     for z in range(0, len(old_list), j):
            #         new_list.append(old_list[z])
            #     for cuts in new_list:
            #         if self.min_child_sample:
            #             if (X.loc[X[item] < cuts].shape[0] < self.min_child_sample) \
            #                     | (X.loc[X[item] >= cuts].shape[0] < self.min_child_sample):
            #                 continue
            #         GH.loc[i, 'G_left'] = X.loc[X[item] < cuts, 'g'].sum()
            #         GH.loc[i, 'G_right'] = X.loc[X[item] >= cuts, 'g'].sum()
            #         GH.loc[i, 'H_left'] = X.loc[X[item] < cuts, 'h'].sum()
            #         GH.loc[i, 'H_right'] = X.loc[X[item] >= cuts, 'h'].sum()
            #         GH.loc[i, 'var'] = item
            #         GH.loc[i, 'cut'] = cuts
            #         i = i + 1
        return GH

    def find_split(self, GH_host, GH_guest):
        # Find the feature corresponding to the best split and the split value
        best_var, best_cut = None, None
        GH_best = {}
        max_gain = 0
        GH = pd.concat([GH_host, GH_guest], axis=0, ignore_index=True)
        for item in GH.index:
            gain = GH.loc[item, 'G_left'] ** 2 / (GH.loc[item, 'H_left'] + self.reg_lambda) + \
                GH.loc[item, 'G_right'] ** 2 / (GH.loc[item, 'H_right'] + self.reg_lambda) - \
                (GH.loc[item, 'G_left'] + GH.loc[item, 'G_right']) ** 2 / (
                GH.loc[item, 'H_left'] + GH.loc[item, 'H_right'] + + self.reg_lambda)
            gain = gain / 2 - self.gamma
            if gain > max_gain:
                best_var = GH.loc[item, 'var']
                best_cut = GH.loc[item, 'cut']
                max_gain = gain
                GH_best['G_left_best'] = GH.loc[item, 'G_left']
                GH_best['G_right_best'] = GH.loc[item, 'G_right']
                GH_best['H_left_best'] = GH.loc[item, 'H_left']
                GH_best['H_right_best'] = GH.loc[item, 'H_right']
        return best_var, best_cut, GH_best

    def split(self, X, best_var, best_cut, GH_best, w):
        # Calculate the weight of leaf nodes after splitting
        # print("++++++host index-1+++++++", len(X.index.tolist()), X.index)
        id_left = X.loc[X[best_var] < best_cut].index.tolist()
        w_left = -GH_best['G_left_best'] / \
            (GH_best['H_left_best'] + self.reg_lambda)
        id_right = X.loc[X[best_var] >= best_cut].index.tolist()
        w_right = -GH_best['G_right_best'] / \
            (GH_best['H_right_best'] + self.reg_lambda)
        w[id_left] = w_left
        w[id_right] = w_right
        return w, id_right, id_left, w_right, w_left

    def xgb_tree(self, X_host, GH_guest, gh, f_t, m_dpth):
        print("=====host mdep===", m_dpth)
        if m_dpth > self.max_depth:
            return
        X_host = X_host
        gh = gh
        X_host_gh = pd.concat([X_host, gh], axis=1)
        GH_host = self.get_GH(X_host_gh)

        best_var, best_cut, GH_best = self.find_split(GH_host, GH_guest)
        if best_var is None:
            self.channel.send("True")
            self.channel.recv()
            return None

        self.channel.send(best_var)
        flag = self.channel.recv()
        if flag:
            if best_var not in [x for x in X_host.columns]:
                data = {'best_var': best_var, 'best_cut': best_cut,
                        'GH_best': GH_best, 'f_t': f_t}
                self.channel.send(data)
                id_w_gh = self.channel.recv()
                f_t = id_w_gh['f_t']
                id_right = id_w_gh['id_right']
                id_left = id_w_gh['id_left']
                w_right = id_w_gh['w_right']
                w_left = id_w_gh['w_left']
                record_id = id_w_gh['record_id']
                party_id = id_w_gh['party_id']
                tree_structure = {(party_id, record_id): {}}
                gh_sum_right = id_w_gh['gh_sum_right']
                gh_sum_left = id_w_gh['gh_sum_left']

            else:
                self.lookup_table.loc[self.record, 'record_id'] = self.record
                self.lookup_table.loc[self.record, 'feature_id'] = best_var
                self.lookup_table.loc[self.record, 'threshold_value'] = best_cut
                record_id = self.record
                party_id = self.sid
                tree_structure = {(party_id, record_id): {}}
                self.record = self.record + 1
                f_t, id_right, id_left, w_right, w_left = self.split(
                    X_host, best_var, best_cut, GH_best, f_t)
                print("host split", X_host.index)
                self.channel.send(
                    {'id_right': id_right, 'id_left': id_left, "best_cut": best_cut})
                gh_sum_dic = self.channel.recv()
                gh_sum_left = gh_sum_dic['gh_sum_left']
                gh_sum_right = gh_sum_dic['gh_sum_right']

            print("=====x host index=====", X_host.index)
            print("host shape",
                  X_host.loc[id_left].shape, X_host.loc[id_right].shape)

            result_left = self.xgb_tree(X_host.loc[id_left],
                                        gh_sum_left,
                                        gh.loc[id_left],
                                        f_t,
                                        m_dpth + 1)
            if isinstance(result_left, tuple):
                tree_structure[(party_id, record_id)][('left', w_left)] = copy.deepcopy(result_left[0])
                f_t = result_left[1]
            else:
                tree_structure[(party_id, record_id)][('left', w_left)] = copy.deepcopy(result_left)
            result_right = self.xgb_tree(X_host.loc[id_right],
                                         gh_sum_right,
                                         gh.loc[id_right],
                                         f_t,
                                         m_dpth + 1)
            if isinstance(result_right, tuple):
                tree_structure[(party_id, record_id)][('right', w_right)] = copy.deepcopy(result_right[0])
                f_t = result_right[1]
            else:
                tree_structure[(party_id, record_id)][('right', w_right)] = copy.deepcopy(result_right)
        return tree_structure, f_t

    def _get_tree_node_w(self, X, tree, lookup_table, w, t):
        if not tree is None:
            if isinstance(tree, tuple):
                tree = tree[0]
            k = list(tree.keys())[0]
            party_id, record_id = k[0], k[1]
            id = X.index.tolist()
            if party_id != self.sid:
                self.channel.send(
                    {'id': id, 'record_id': record_id, 'tree': t})
                split_id = self.channel.recv()
                id_left = split_id['id_left']
                id_right = split_id['id_right']
                if id_left == [] or id_right == []:
                    return
                else:
                    X_left = X.loc[id_left, :]
                    X_right = X.loc[id_right, :]
                for kk in tree[k].keys():
                    if kk[0] == 'left':
                        tree_left = tree[k][kk]
                        w[id_left] = kk[1]
                    elif kk[0] == 'right':
                        tree_right = tree[k][kk]
                        w[id_right] = kk[1]

                self._get_tree_node_w(X_left, tree_left, lookup_table, w, t)
                self._get_tree_node_w(X_right, tree_right, lookup_table, w, t)
            else:
                for index in lookup_table.index:
                    if lookup_table.loc[index, 'record_id'] == record_id:
                        var = lookup_table.loc[index, 'feature_id']
                        cut = lookup_table.loc[index, 'threshold_value']
                        X_left = X.loc[X[var] < cut]
                        id_left = X_left.index.tolist()
                        X_right = X.loc[X[var] >= cut]
                        id_right = X_right.index.tolist()
                        if id_left == [] or id_right == []:
                            return
                        for kk in tree[k].keys():
                            if kk[0] == 'left':
                                tree_left = tree[k][kk]
                                w[id_left] = kk[1]
                            elif kk[0] == 'right':
                                tree_right = tree[k][kk]
                                w[id_right] = kk[1]

                        self._get_tree_node_w(X_left, tree_left, lookup_table, w, t)
                        self._get_tree_node_w(X_right, tree_right, lookup_table, w, t)

    def predict_raw(self, X: pd.DataFrame):
        X = X.reset_index(drop='True')
        Y = pd.Series([self.base_score] * X.shape[0])

        for t in range(self.n_estimators):
            tree = self.tree_structure[t + 1]
            lookup_table = self.lookup_table_sum[t + 1]
            y_t = pd.Series([0] * X.shape[0])
            self._get_tree_node_w(X, tree, lookup_table, y_t, t)
            Y = Y + self.learning_rate * y_t

        self.channel.send(-1)
        print(self.channel.recv())
        return Y

    def predict_prob(self, X: pd.DataFrame):

        Y = self.predict_raw(X)

        def sigmoid(x): return 1 / (1 + np.exp(-x))

        Y = Y.apply(sigmoid)

        return Y
