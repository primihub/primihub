from primihub.primitive.opt_paillier_c2py_warpper import *
import time
import pandas as pd
import numpy as np


class XGB_GUEST_EN:
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
        self.lookup_table_sum = {}

    def get_GH(self, X, pub):
        # Calculate G_left、G_right、H_left、H_right under feature segmentation
        arr = np.zeros((X.shape[0] * 10, 6))
        GH = pd.DataFrame(arr, columns=['G_left', 'G_right', 'H_left', 'H_right', 'var', 'cut'])
        i = 0
        for item in [x for x in X.columns if x not in ['g', 'h']]:
            # Categorical variables using greedy algorithm
            # if len(list(set(X[item]))) < 5:
            for cuts in list(set(X[item])):
                if self.min_child_sample:
                    if (X.loc[X[item] < cuts].shape[0] < self.min_child_sample) \
                            | (X.loc[X[item] >= cuts].shape[0] < self.min_child_sample):
                        continue
                for ind in X.index:
                    if X.loc[ind, item] < cuts:
                        if GH.loc[i, 'G_left'] == 0:
                            GH.loc[i, 'G_left'] = X.loc[ind, 'g']
                        else:
                            GH.loc[i, 'G_left'] = opt_paillier_add(pub, GH.loc[i, 'G_left'], X.loc[ind, 'g'])
                        if GH.loc[i, 'H_left'] == 0:
                            GH.loc[i, 'H_left'] = X.loc[ind, 'h']
                        else:
                            GH.loc[i, 'H_left'] = opt_paillier_add(pub, GH.loc[i, 'H_left'], X.loc[ind, 'h'])
                    else:
                        if GH.loc[i, 'G_right'] == 0:
                            GH.loc[i, 'G_right'] = X.loc[ind, 'g']
                        else:
                            GH.loc[i, 'G_right'] = opt_paillier_add(pub, GH.loc[i, 'G_right'], X.loc[ind, 'g'])
                        if GH.loc[i, 'H_right'] == 0:
                            GH.loc[i, 'H_right'] = X.loc[ind, 'h']
                        else:
                            GH.loc[i, 'H_right'] = opt_paillier_add(pub, GH.loc[i, 'H_right'], X.loc[ind, 'h'])
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

    # def find_split(self, GH):
    #     # Find the feature corresponding to the best split and the split value
    #     GH_best = pd.DataFrame(columns=['G_left', 'G_right', 'H_left', 'H_right', 'var', 'cut'])
    #     max_gain = 0
    #     for item in GH.index:
    #         gain = GH.loc[item, 'G_left'] ** 2 / (GH.loc[item, 'H_left'] + self.reg_lambda) + \
    #                GH.loc[item, 'G_right'] ** 2 / (GH.loc[item, 'H_right'] + self.reg_lambda) - \
    #                (GH.loc[item, 'G_left'] + GH.loc[item, 'G_right']) ** 2 / (
    #                        GH.loc[item, 'H_left'] + GH.loc[item, 'H_right'] + + self.reg_lambda)
    #         gain = gain / 2 - self.gamma
    #         if gain > max_gain:
    #             max_gain = gain
    #             GH_best.loc[0, 'G_left'] = GH.loc[item, 'G_left']
    #             GH_best.loc[0, 'G_right'] = GH.loc[item, 'G_right']
    #             GH_best.loc[0, 'H_left'] = GH.loc[item, 'H_left']
    #             GH_best.loc[0, 'H_right'] = GH.loc[item, 'H_right']
    #             GH_best.loc[0, 'var'] = GH.loc[item, 'var']
    #             GH_best.loc[0, 'cut'] = GH.loc[item, 'cut']
    #     return GH_best

    def split(self, X, best_var, best_cut, GH_best, w):
        # Calculate the weight of leaf nodes after splitting
        # print("=====guest index====", X.index, best_cut)
        id_left = X.loc[X[best_var] < best_cut].index.tolist()
        w_left = -GH_best['G_left_best'] / \
                 (GH_best['H_left_best'] + self.reg_lambda)
        id_right = X.loc[X[best_var] >= best_cut].index.tolist()
        w_right = -GH_best['G_right_best'] / \
                  (GH_best['H_right_best'] + self.reg_lambda)
        w[id_left] = w_left
        w[id_right] = w_right
        return w, id_right, id_left, w_right, w_left

    def cart_tree(self, X_guest_gh, mdep, pub):
        print("guest dept", mdep)
        if mdep > self.max_depth:
            return
        best_var = self.channel.recv()
        if best_var == "True":
            self.channel.send("True")
            return None

        self.channel.send("true")
        if best_var in [x for x in X_guest_gh.columns]:
            var_cut_GH = self.channel.recv()
            best_var = var_cut_GH['best_var']
            best_cut = var_cut_GH['best_cut']
            GH_best = var_cut_GH['GH_best']
            f_t = var_cut_GH['f_t']
            self.lookup_table.loc[self.record, 'record_id'] = self.record
            self.lookup_table.loc[self.record, 'feature_id'] = best_var
            self.lookup_table.loc[self.record, 'threshold_value'] = best_cut
            f_t, id_right, id_left, w_right, w_left = self.split(
                X_guest_gh, best_var, best_cut, GH_best, f_t)

            gh_sum_left = self.get_GH(X_guest_gh.loc[id_left], pub)
            gh_sum_right = self.get_GH(X_guest_gh.loc[id_right], pub)

            data = {'f_t': f_t, 'id_right': id_right, 'id_left': id_left, 'w_right': w_right,
                    'w_left': w_left, 'gh_sum_right': gh_sum_right, 'gh_sum_left': gh_sum_left,
                    'record_id': self.record, 'party_id': self.sid}
            self.channel.send(data)
            self.record = self.record + 1
            print("data", type(data), data)
            time.sleep(5)

        else:
            id_dic = self.channel.recv()
            id_right = id_dic['id_right']

            id_left = id_dic['id_left']
            print("++++++", mdep)
            print("=======guest index-2 ======",
                  len(X_guest_gh.index.tolist()), X_guest_gh.index)

            # .reset_index(drop='True'))
            gh_sum_left = self.get_GH(X_guest_gh.loc[id_left], pub)
            # .reset_index(drop='True'))
            gh_sum_right = self.get_GH(X_guest_gh.loc[id_right], pub)
            self.channel.send(
                {'gh_sum_right': gh_sum_right, 'gh_sum_left': gh_sum_left})

        # left tree
        print("=====guest shape========",
              X_guest_gh.loc[id_left].shape, X_guest_gh.loc[id_right].shape)
        self.cart_tree(X_guest_gh.loc[id_left], mdep + 1, pub)

        # right tree
        self.cart_tree(X_guest_gh.loc[id_right], mdep + 1, pub)

    def host_record(self, record_id, id_list, tree, X):
        id_after_record = {"id_left": [], "id_right": []}
        record_tree = self.lookup_table_sum[tree + 1]
        feature = record_tree.loc[record_id, "feature_id"]
        threshold_value = record_tree.loc[record_id, 'threshold_value']
        for i in id_list:
            feature_value = X.loc[i, feature]
            if feature_value >= threshold_value:
                id_after_record["id_right"].append(i)
            else:
                id_after_record["id_left"].append(i)

        return id_after_record

    def predict(self, X):
        flag = True
        while (flag):
            need_record = self.channel.recv()

            if need_record == -1:
                flag = False
                self.channel.send(b"finished predict")
            else:
                record_id = need_record["record_id"]
                id_list = need_record["id"]
                tree = need_record["tree"]
                id_after_record = self.host_record(record_id, id_list, tree, X)
                self.channel.send(id_after_record)
