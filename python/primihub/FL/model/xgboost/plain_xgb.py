import numpy as np
import pandas as pd


class XGB:

    def __init__(self,
                 base_score=0.5,
                 max_depth=3,
                 n_estimators=10,
                 learning_rate=0.1,
                 reg_lambda=1,
                 gamma=0,
                 min_child_sample=None,
                 min_child_weight=1,
                 objective='linear'):

        self.base_score = base_score  # 最开始时给叶子节点权重所赋的值，默认0.5，迭代次数够多的话，结果对这个初值不敏感
        self.max_depth = max_depth  # 最大数深度
        self.n_estimators = n_estimators  # 树的个数
        self.learning_rate = learning_rate  # 学习率，别和梯度下降里的学习率搞混了，这里是每棵树要乘以的权重系数
        self.reg_lambda = reg_lambda  # L2正则项的权重系数
        self.gamma = gamma  # 正则项中，叶子节点数T的权重系数
        self.min_child_sample = min_child_sample  # 每个叶子节点的样本数（自己加的）
        self.min_child_weight = min_child_weight  # 每个叶子节点的Hessian矩阵和，下面代码会细讲
        self.objective = objective  # 目标函数，可选linear和logistic
        self.tree_structure = {}  # 用一个字典来存储每一颗树的树结构

    def xgb_cart_tree(self, X, w, m_dpth):
        '''
        递归的方式构造XGB中的Cart树
        X：训练数据集
        w：每个样本的权重值，递归赋值
        m_dpth：树的深度
        '''

        # 边界条件：递归到指定最大深度后，跳出
        if m_dpth > self.max_depth:
            return

        best_var, best_cut = None, None
        # 这里增益的初值一定要设置为0，相当于对树做剪枝，即如果算None出的增益小于0则不做分裂
        max_gain = 0
        G_left_best, G_right_best, H_left_best, H_right_best = 0, 0, 0, 0
        # 遍历每个变量的每个切点，寻找分裂增益gain最大的切点并记录下来
        for item in [x for x in X.columns if x not in ['g', 'h', 'y']]:
            for cut in list(set(X[item])):

                # 这里如果指定了min_child_sample则限制分裂后叶子节点的样本数都不能小于指定值
                if self.min_child_sample:
                    if (X.loc[X[item] < cut].shape[0] < self.min_child_sample) \
                            | (X.loc[X[item] >= cut].shape[0] < self.min_child_sample):
                        continue

                G_left = X.loc[X[item] < cut, 'g'].sum()
                G_right = X.loc[X[item] >= cut, 'g'].sum()
                H_left = X.loc[X[item] < cut, 'h'].sum()
                H_right = X.loc[X[item] >= cut, 'h'].sum()

                # min_child_weight在这里起作用，指的是每个叶子节点上的H，即目标函数二阶导的加和
                # 当目标函数为linear，即1/2*(y-y_hat)**2时，它的二阶导是1，那min_child_weight就等价于min_child_sample
                # 当目标函数为logistic，其二阶导为sigmoid(y_hat)*(1-sigmoid(y_hat))，可理解为叶子节点的纯度，更详尽的解释可参看：
                # https://stats.stackexchange.com/questions/317073/explanation-of-min-child-weight-in-xgboost-algorithm#
                if self.min_child_weight:
                    if (H_left < self.min_child_weight) | (H_right < self.min_child_weight):
                        continue

                gain = G_left ** 2 / (H_left + self.reg_lambda) + \
                       G_right ** 2 / (H_right + self.reg_lambda) - \
                       (G_left + G_right) ** 2 / (H_left + H_right + self.reg_lambda)
                gain = gain / 2 - self.gamma
                if gain > max_gain:
                    best_var, best_cut = item, cut
                    max_gain = gain
                    G_left_best, G_right_best, H_left_best, H_right_best = G_left, G_right, H_left, H_right

        # 如果遍历完找不到可分列的点，则返回None
        if best_var is None:
            return None

        # 给每个叶子节点上的样本分别赋上相应的权重值
        id_left = X.loc[X[best_var] < best_cut].index.tolist()
        w_left = - G_left_best / (H_left_best + self.reg_lambda)

        id_right = X.loc[X[best_var] >= best_cut].index.tolist()
        w_right = - G_right_best / (H_right_best + self.reg_lambda)

        w[id_left] = w_left
        w[id_right] = w_right

        # 用俄罗斯套娃式的json串把树的结构给存下来
        tree_structure = {(best_var, best_cut): {}}
        tree_structure[(best_var, best_cut)][('left', w_left)] = self.xgb_cart_tree(X.loc[id_left], w, m_dpth + 1)
        tree_structure[(best_var, best_cut)][('right', w_right)] = self.xgb_cart_tree(X.loc[id_right], w, m_dpth + 1)

        return tree_structure

    def _grad(self, y_hat, Y):
        '''
        计算目标函数的一阶导
        支持linear和logistic
        '''

        if self.objective == 'logistic':
            y_hat = 1.0 / (1.0 + np.exp(-y_hat))
            return y_hat - Y
        elif self.objective == 'linear':
            return y_hat - Y
        else:
            raise KeyError('objective must be linear or logistic!')

    def _hess(self, y_hat, Y):
        '''
        计算目标函数的二阶导
        支持linear和logistic
        '''

        if self.objective == 'logistic':
            y_hat = 1.0 / (1.0 + np.exp(-y_hat))
            return y_hat * (1.0 - y_hat)
        elif self.objective == 'linear':
            return np.array([1] * Y.shape[0])
        else:
            raise KeyError('objective must be linear or logistic!')

    def fit(self, X: pd.DataFrame, Y):
        '''
        根据训练数据集X和Y训练出树结构和权重
        '''

        if X.shape[0] != Y.shape[0]:
            raise ValueError('X and Y must have the same length!')

        X = X.reset_index(drop='True')
        Y = Y.values
        # 这里根据base_score参数设定权重初始值
        y_hat = np.array([self.base_score] * Y.shape[0])
        for t in range(self.n_estimators):
            print('fitting tree {}...'.format(t + 1))

            X['g'] = self._grad(y_hat, Y)
            X['h'] = self._hess(y_hat, Y)

            f_t = pd.Series([0] * Y.shape[0])
            self.tree_structure[t + 1] = self.xgb_cart_tree(X, f_t, 1)

            y_hat = y_hat + self.learning_rate * f_t

            print('tree {} fit done!'.format(t + 1))

        print(self.tree_structure)

    def _get_tree_node_w(self, X, tree, w):
        '''
        以递归的方法，把树结构解构出来，把权重值赋到w上面
        '''

        if not tree is None:
            k = list(tree.keys())[0]
            var, cut = k[0], k[1]
            X_left = X.loc[X[var] < cut]
            id_left = X_left.index.tolist()
            X_right = X.loc[X[var] >= cut]
            id_right = X_right.index.tolist()
            for kk in tree[k].keys():
                if kk[0] == 'left':
                    tree_left = tree[k][kk]
                    w[id_left] = kk[1]
                elif kk[0] == 'right':
                    tree_right = tree[k][kk]
                    w[id_right] = kk[1]

            self._get_tree_node_w(X_left, tree_left, w)
            self._get_tree_node_w(X_right, tree_right, w)

    def predict_raw(self, X: pd.DataFrame):
        '''
        根据训练结果预测
        返回原始预测值
        '''

        X = X.reset_index(drop='True')
        Y = pd.Series([self.base_score] * X.shape[0])

        for t in range(self.n_estimators):
            tree = self.tree_structure[t + 1]
            y_t = pd.Series([0] * X.shape[0])
            self._get_tree_node_w(X, tree, y_t)
            Y = Y + self.learning_rate * y_t

        return Y

    def predict_prob(self, X: pd.DataFrame):
        '''
        当指定objective为logistic时，输出概率要做一个logistic转换
        '''

        Y = self.predict_raw(X)
        sigmoid = lambda x: 1 / (1 + np.exp(-x))
        Y = Y.apply(sigmoid)

        return Y
