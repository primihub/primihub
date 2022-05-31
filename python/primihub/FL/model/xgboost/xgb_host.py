"""
 Copyright 2022 Primihub

 Licensed under the Apache License, Version 2.0 (the "License");
 you may not use this file except in compliance with the License.
 You may obtain a copy of the License at

      https://www.apache.org/licenses/LICENSE-2.0

 Unless required by applicable law or agreed to in writing, software
 distributed under the License is distributed on an "AS IS" BASIS,
 WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 See the License for the specific language governing permissions and
 limitations under the License.
 """


from primihub.FL.model.xgboost.plain_xgb import XGB
from primihub.FL.model.xgboost.xgb_guest import XGB_GUEST
import numpy as np
import pandas as pd


class XGB_HOST(XGB):
    def get_gh(self, y_hat, Y):
        # Calculate the g and h of each sample based on the labels of the local data
        gh = pd.DataFrame(columns=['g', 'h'])
        for i in range(0, Y.shape[0]):
            gh['g'] = self._grad(y_hat, Y)
            gh['h'] = self._hess(y_hat, Y)
        return gh

    def get_GH(self, X):
        # Calculate G_left、G_right、H_left、H_right under feature segmentation
        GH = pd.DataFrame(columns=['G_left', 'G_right', 'H_left', 'H_right', 'var', 'cut'])
        i = 0
        for item in [x for x in X.columns if x not in ['g', 'h']]:
            # Categorical variables using greedy algorithm
            if len(list(set(X[item]))) < 5:
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
            else:
                old_list = list(set(X[item]))
                new_list = []
                # four candidate points
                j = int(len(old_list) / 4)
                for z in range(0, len(old_list), j):
                    new_list.append(old_list[z])
                for cuts in new_list:
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
        id_left = X.loc[X[best_var] < best_cut].index.tolist()
        w_left = -GH_best['G_left_best'] / (GH_best['H_left_best'] + self.reg_lambda)
        id_right = X.loc[X[best_var] >= best_cut].index.tolist()
        w_right = -GH_best['G_right_best'] / (GH_best['H_right_best'] + self.reg_lambda)
        w[id_left] = w_left
        w[id_right] = w_right
        return w, id_right, id_left, w_right, w_left

    def xgb_tree(self, X_host, X_guest, gh, f_t, m_dpth):
        if m_dpth > self.max_depth:
            return
        xgb_guest = XGB_GUEST(n_estimators=1, max_depth=2, reg_lambda=1, min_child_weight=1, objective='linear')
        X_host = X_host.reset_index(drop='True')
        X_guest = X_guest.reset_index(drop='True')
        gh = gh.reset_index(drop='True')
        X_host_gh = pd.concat([X_host, gh], axis=1)
        X_guest_gh = pd.concat([X_guest, gh], axis=1)
        GH_guest = xgb_guest.get_GH(X_guest_gh)
        GH_guest_best = xgb_guest.find_split(GH_guest)
        GH_host = self.get_GH(X_host_gh)
        best_var, best_cut, GH_best = self.find_split(GH_host, GH_guest_best)
        tree_structure = {(best_var, best_cut): {}}
        if best_var not in [x for x in X_host.columns]:
            f_t, id_right, id_left, w_right, w_left = xgb_guest.split(X_guest, best_var, best_cut, GH_best, f_t)
            tree_structure[(best_var, best_cut)][('left', w_left)] = self.xgb_tree(X_host.loc[id_left],
                                                                                   X_guest.loc[id_left], gh.loc[id_left], f_t,
                                                                                   m_dpth + 1)
            tree_structure[(best_var, best_cut)][('right', w_right)] = self.xgb_tree(X_host.loc[id_right],
                                                                                     X_guest.loc[id_right], gh.loc[id_right], f_t,
                                                                                     m_dpth + 1)
        else:
            f_t, id_right, id_left, w_right, w_left = self.split(X_host, best_var, best_cut, GH_best, f_t)
            tree_structure[(best_var, best_cut)][('left', w_left)] = self.xgb_tree(X_host.loc[id_left],
                                                                                   X_guest.loc[id_left],
                                                                                   gh.loc[id_left],
                                                                                   f_t,
                                                                                   m_dpth + 1)
            tree_structure[(best_var, best_cut)][('right', w_right)] = self.xgb_tree(X_host.loc[id_right],
                                                                                     X_guest.loc[id_right],
                                                                                     gh.loc[id_right],
                                                                                     f_t,
                                                                                     m_dpth + 1)
        return tree_structure

    def fits(self, X_host, X_guest, Y):
        # Train the tree structure and weights
        if X_host.shape[0] != Y.shape[0]:
            raise ValueError('X and Y must have the same length!')
        if X_guest.shape[0] != Y.shape[0]:
            raise ValueError('X and Y must have the same length!')

        Y = Y.values
        # Set the initial value of the weight according to the base_score parameter
        y_hat = np.array([self.base_score] * Y.shape[0])
        for t in range(self.n_estimators):
            gh = self.get_gh(y_hat, Y)
            f_t = pd.Series([0] * Y.shape[0])
            self.tree_structure[t + 1] = self.xgb_tree(X_host, X_guest, gh, f_t, 1)
            y_hat = y_hat + self.learning_rate * f_t

    def predict(self, X):
        X = X.reset_index(drop='True')
        Y = pd.Series([self.base_score] * X.shape[0])
        for t in range(self.n_estimators):
            tree = self.tree_structure[t + 1]
            y_t = pd.Series([0] * X.shape[0])
            self._get_tree_node_w(X, tree, y_t)
            Y = Y + self.learning_rate * y_t

        return Y
