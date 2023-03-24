import math
import numpy as np
from phe import paillier
import pandas as pd
from sklearn import datasets
from sklearn.datasets import load_diabetes
from sklearn.preprocessing import StandardScaler
from sklearn.datasets import load_breast_cancer
from sklearn.model_selection import train_test_split
from sklearn.utils import shuffle


class Client:
    def __init__(self, config):
        ## 模型参数
        self.config = config
        ## 中间计算结果
        self.data = {}
        ## 与其他节点的连接状况
        self.other_client = {}

    ## 与其他参与方建立连接
    def connect(self, client_name, target_client):
        self.other_client[client_name] = target_client

    ## 向特定参与方发送数据
    def send_data(self, data, target_client):
        target_client.data.update(data)


class ClientA(Client):
    def __init__(self, X, config):
        super().__init__(config)
        self.X = X
        self.weights = np.zeros(X.shape[1])

    def compute_z_a(self):
        z_a = np.dot(self.X, self.weights)
        return z_a

    ## 加密梯度的计算，对应step4
    def compute_encrypted_dJ_a(self, encrypted_u):
        encrypted_dJ_a = self.X.T.dot(encrypted_u) + self.config[
            'lambda'] * self.weights
        return encrypted_dJ_a

    ##参数的更新
    def update_weight(self, dJ_a):
        self.weights = self.weights - self.config["lr"] * dJ_a / len(self.X)
        return

    ## A: step2
    def task_1(self, client_B_name):
        dt = self.data
        assert "public_key" in dt.keys(), "Error: 'public_key' from C in step " \
                                          "1 not successfully received."
        public_key = dt['public_key']
        z_a = self.compute_z_a()
        u_a = 0.25 * z_a
        z_a_square = z_a ** 2
        encrypted_u_a = np.asarray([public_key.encrypt(x) for x in u_a])
        encrypted_z_a_square = np.asarray(
            [public_key.encrypt(x) for x in z_a_square])
        dt.update({"encrypted_u_a": encrypted_u_a})
        data_to_B = {"encrypted_u_a": encrypted_u_a,
                     "encrypted_z_a_square": encrypted_z_a_square}
        self.send_data(data_to_B, self.other_client[client_B_name])

    ## A: step3、4
    def task_2(self, client_C_name):
        dt = self.data
        assert "encrypted_u_b" in dt.keys(), "Error: 'encrypted_u_b' from B " \
                                             "in step 1 not successfully " \
                                             "received."
        encrypted_u_b = dt['encrypted_u_b']
        encrypted_u = encrypted_u_b + dt['encrypted_u_a']
        encrypted_dJ_a = self.compute_encrypted_dJ_a(encrypted_u)
        mask = np.random.rand(len(encrypted_dJ_a))
        encrypted_masked_dJ_a = encrypted_dJ_a + mask
        dt.update({"mask": mask})
        data_to_C = {'encrypted_masked_dJ_a': encrypted_masked_dJ_a}
        self.send_data(data_to_C, self.other_client[client_C_name])

    ## A: step6
    def task_3(self):
        dt = self.data
        assert "masked_dJ_a" in dt.keys(), "Error: 'masked_dJ_a' from C in " \
                                           "step 2 not successfully received."
        masked_dJ_a = dt['masked_dJ_a']
        dJ_a = masked_dJ_a - dt['mask']
        self.update_weight(dJ_a)
        print(f"A weight: {self.weights}")

        trainA = np.dot(self.X, self.weights)
        print(trainA.shape)
        return trainA

class ClientB(Client):
    def __init__(self, X, y, config):
        super().__init__(config)
        self.X = X
        self.y = y
        self.weights = np.zeros(X.shape[1])
        self.data = {}

    def compute_u_b(self):
        z_b = np.dot(self.X, self.weights)
        u_b = 0.25 * z_b - self.y + 0.5
        return z_b, u_b

    def compute_encrypted_dJ_b(self, encrypted_u):
        encrypted_dJ_b = self.X.T.dot(encrypted_u) + self.config[
            'lambda'] * self.weights
        return encrypted_dJ_b

    def update_weight(self, dJ_b):
        self.weights = self.weights - self.config["lr"] * dJ_b / len(self.X)

    ## B: step2
    def task_1(self, client_A_name):
        try:
            dt = self.data
            assert "public_key" in dt.keys(), "Error: 'public_key' from C in " \
                                              "step 1 not successfully " \
                                              "received."
            public_key = dt['public_key']
        except Exception as e:
            print("B step 1 exception: %s" % e)
        try:
            z_b, u_b = self.compute_u_b()
            encrypted_u_b = np.asarray([public_key.encrypt(x) for x in u_b])
            dt.update({"encrypted_u_b": encrypted_u_b})
            dt.update({"z_b": z_b})
        except Exception as e:
            print("Wrong 1 in B: %s" % e)

        data_to_A = {"encrypted_u_b": encrypted_u_b}
        self.send_data(data_to_A, self.other_client[client_A_name])

    ## B: step3、4
    def task_2(self, client_C_name):
        try:
            dt = self.data
            assert "encrypted_u_a" in dt.keys(), "Error: 'encrypt_u_a' from A " \
                                                 "in step 1 not successfully " \
                                                 "received."
            encrypted_u_a = dt['encrypted_u_a']
            encrypted_u = encrypted_u_a + dt['encrypted_u_b']
            encrypted_dJ_b = self.compute_encrypted_dJ_b(encrypted_u)
            mask = np.random.rand(len(encrypted_dJ_b))
            encrypted_masked_dJ_b = encrypted_dJ_b + mask
            dt.update({"mask": mask})
        except Exception as e:
            print("B step 2 exception: %s" % e)
        try:
            assert "encrypted_z_a_square" in dt.keys(), "Error: " \
                                                        "'encrypted_z_a_square' from A in step 1 not successfully received."
            encrypted_z = 4 * encrypted_u_a + dt['z_b']
            encrypted_loss = np.sum((0.5 - self.y) * encrypted_z + 0.125 * dt[
                "encrypted_z_a_square"] + 0.125 * dt["z_b"] * (
                                                encrypted_z + 4 *
                                                encrypted_u_a))
        except Exception as e:
            print("B step 2 exception: %s" % e)
        data_to_C = {"encrypted_masked_dJ_b": encrypted_masked_dJ_b,
                     "encrypted_loss": encrypted_loss}
        self.send_data(data_to_C, self.other_client[client_C_name])

    ## B: step6
    def task_3(self):
        try:
            dt = self.data
            assert "masked_dJ_b" in dt.keys(), "Error: 'masked_dJ_b' from C " \
                                               "in step 2 not successfully " \
                                               "received."
            masked_dJ_b = dt['masked_dJ_b']
            dJ_b = masked_dJ_b - dt['mask']
            self.update_weight(dJ_b)
        except Exception as e:
            print("A step 3 exception: %s" % e)
        print(f"B weight: {self.weights}")
        trainB = np.dot(self.X,self.weights)
        print(trainB.shape)
        return trainB


class ClientC(Client):
    """
    Client C as trusted dealer.
    """

    def __init__(self, A_d_shape, B_d_shape, config):
        super().__init__(config)
        self.A_data_shape = A_d_shape
        self.B_data_shape = B_d_shape
        self.public_key = None
        self.private_key = None
        ## 保存训练中的损失值（泰展开近似）
        self.loss = []

    ## C: step1
    def task_1(self, client_A_name, client_B_name):
        try:
            public_key, private_key = paillier.generate_paillier_keypair()
            self.public_key = public_key
            self.private_key = private_key
        except Exception as e:
            print("C step 1 error 1: %s" % e)

        data_to_AB = {"public_key": public_key}
        self.send_data(data_to_AB, self.other_client[client_A_name])
        self.send_data(data_to_AB, self.other_client[client_B_name])
        return

    ## C: step5
    def task_2(self, client_A_name, client_B_name):
        try:
            dt = self.data
            assert "encrypted_masked_dJ_a" in dt.keys() and \
                   "encrypted_masked_dJ_b" in dt.keys(), "Error: " \
                                                         "'masked_dJ_a' from " \
                                                         "A or 'masked_dJ_b' " \
                                                         "from B in step 2 " \
                                                         "not successfully " \
                                                         "received."
            encrypted_masked_dJ_a = dt['encrypted_masked_dJ_a']
            encrypted_masked_dJ_b = dt['encrypted_masked_dJ_b']
            masked_dJ_a = np.asarray(
                [self.private_key.decrypt(x) for x in encrypted_masked_dJ_a])
            masked_dJ_b = np.asarray(
                [self.private_key.decrypt(x) for x in encrypted_masked_dJ_b])
        except Exception as e:
            print("C step 2 exception: %s" % e)

        try:
            assert "encrypted_loss" in dt.keys(), "Error: 'encrypted_loss' " \
                                                  "from B in step 2 not " \
                                                  "successfully received."
            encrypted_loss = dt['encrypted_loss']
            loss = self.private_key.decrypt(encrypted_loss) / self.A_data_shape[
                0] + math.log(2)
            print("******loss: ", loss, "******")
            self.loss.append(loss)
        except Exception as e:
            print("C step 2 exception: %s" % e)

        data_to_A = {"masked_dJ_a": masked_dJ_a}
        data_to_B = {"masked_dJ_b": masked_dJ_b}
        self.send_data(data_to_A, self.other_client[client_A_name])
        self.send_data(data_to_B, self.other_client[client_B_name])
        return

def load_data():
    # 加载数据
    breast = load_breast_cancer()
    # 数据拆分
    X_train, X_test, y_train, y_test = train_test_split(breast.data, breast.target, random_state=1)
    # 数据标准化
    std = StandardScaler()
    X_train = std.fit_transform(X_train)
    X_test = std.transform(X_test)
    return X_train, y_train, X_test, y_test


## 将特征分配给A和B
def vertically_partition_data(X, X_test, A_idx, B_idx):
    """
    Vertically partition feature for party A and B
    :param X: train feature
    :param X_test: test feature
    :param A_idx: feature index of party A
    :param B_idx: feature index of party B
    :return: train data for A, B; test data for A, B
    """
    XA = X[:, A_idx]
    XB = X[:, B_idx]
    XB = np.c_[np.ones(X.shape[0]), XB]
    XA_test = X_test[:, A_idx]
    XB_test = X_test[:, B_idx]
    XB_test = np.c_[np.ones(XB_test.shape[0]), XB_test]
    return XA, XB, XA_test, XB_test


def vertical_logistic_regression(X, y, X_test, y_test, config):
    """
    Start the processes of the three clients: A, B and C.
    :param X: features of the training dataset
    :param y: labels of the training dataset
    :param X_test: features of the test dataset
    :param y_test: labels of the test dataset
    :param config: the config dict
    :return: True
    """

    ## 获取数据
    XA, XB, XA_test, XB_test = vertically_partition_data(X, X_test,
                                                         config['A_idx'],
                                                         config['B_idx'])
    print('XA:', XA.shape, '   XB:', XB.shape)

    ## 各参与方的初始化
    client_A = ClientA(XA, config)
    print("Client_A successfully initialized.")
    client_B = ClientB(XB, y, config)
    print("Client_B successfully initialized.")
    client_C = ClientC(XA.shape, XB.shape, config)
    print("Client_C successfully initialized.")

    ## 各参与方之间连接的建立
    client_A.connect("B", client_B)
    client_A.connect("C", client_C)
    client_B.connect("A", client_A)
    client_B.connect("C", client_C)
    client_C.connect("A", client_A)
    client_C.connect("B", client_B)

    ## 训练
    for i in range(config['n_iter']):
        client_C.task_1("A", "B")
        client_A.task_1("B")
        client_B.task_1("A")
        client_A.task_2("C")
        client_B.task_2("C")
        client_C.task_2("A", "B")
        preA = client_A.task_3()
        preB = client_B.task_3()
        acc = predict(preA,preB,y)
        print("the train of {}:{:.2f}%".format(i,acc))
    print("All process done.")
    return True


config = {
    'n_iter': 10,
    'lambda': 10,
    'lr': 0.05,
    'A_idx': [10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25,
               26, 27, 28, 29],
    'B_idx': [0, 1, 2, 3, 4, 5, 6, 7, 8, 9],
}

def predict(A, B,y):
    pre = A + B
    pre[pre > 0.5] = 1
    pre[pre<=0.5] = 0
    pre_label = pre
    pre_label = np.array(pre_label)
    pre_true = pre_label.sum()
    test_acc = (pre_true / len(y))*100
    return test_acc

x_train, y_train, x_test, y_test = load_data()
x_train = x_train[:50,:]
y_train = y_train[:50]
x_test = x_test[:10,:]
y_test = y_test[:10]
vertical_logistic_regression(x_train, y_train, x_test, y_test, config)




