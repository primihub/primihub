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


import pandas as pd
from primihub.FL.model.xgboost.plain_xgb import XGB


class XGB_GUEST(XGB):
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

    def find_split(self, GH):
        # Find the feature corresponding to the best split and the split value
        GH_best = pd.DataFrame(columns=['G_left', 'G_right', 'H_left', 'H_right', 'var', 'cut'])
        max_gain = 0
        for item in GH.index:
            gain = GH.loc[item, 'G_left'] ** 2 / (GH.loc[item, 'H_left'] + self.reg_lambda) + \
                   GH.loc[item, 'G_right'] ** 2 / (GH.loc[item, 'H_right'] + self.reg_lambda) - \
                   (GH.loc[item, 'G_left'] + GH.loc[item, 'G_right']) ** 2 / (
                           GH.loc[item, 'H_left'] + GH.loc[item, 'H_right'] + + self.reg_lambda)
            gain = gain / 2 - self.gamma
            if gain > max_gain:
                max_gain = gain
                GH_best.loc[0, 'G_left'] = GH.loc[item, 'G_left']
                GH_best.loc[0, 'G_right'] = GH.loc[item, 'G_right']
                GH_best.loc[0, 'H_left'] = GH.loc[item, 'H_left']
                GH_best.loc[0, 'H_right'] = GH.loc[item, 'H_right']
                GH_best.loc[0, 'var'] = GH.loc[item, 'var']
                GH_best.loc[0, 'cut'] = GH.loc[item, 'cut']
        return GH_best

    def split(self, X, best_var, best_cut, GH_best, w):
        # Calculate the weight of leaf nodes after splitting
        id_left = X.loc[X[best_var] < best_cut].index.tolist()
        w_left = -GH_best['G_left_best'] / (GH_best['H_left_best'] + self.reg_lambda)
        id_right = X.loc[X[best_var] >= best_cut].index.tolist()
        w_right = -GH_best['G_right_best'] / (GH_best['H_right_best'] + self.reg_lambda)
        w[id_left] = w_left
        w[id_right] = w_right
        return w, id_right, id_left, w_right, w_left
