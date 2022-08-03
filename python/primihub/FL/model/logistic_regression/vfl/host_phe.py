import numpy as np
from sklearn import metrics
from os import path
# from primihub.FL.model.evaluation.evaluation import Evaluator, Classification_eva
from primihub.FL.model.logistic_regression.vfl.evaluation_lr import classification_eva
from primihub.FL.proxy.proxy import ServerChannelProxy
from primihub.FL.proxy.proxy import ClientChannelProxy
proxy_server_arbiter = ServerChannelProxy("10091")  # host接收来自arbiter消息
proxy_server_guest = ServerChannelProxy("10095")  # host接收来自guest消息
proxy_client_arbiter = ClientChannelProxy(
    "127.0.0.1", "10092")  # host发送消息给arbiter
proxy_client_guest = ClientChannelProxy("127.0.0.1", "10093")  # host发送消息给guest

dir = path.join(path.dirname(__file__), '../../../../tests/data')
data_host = np.loadtxt(
    path.join(
        dir,
        "wisconsin_host.data"),
    str,
    delimiter=',')
data_test = np.loadtxt(
    path.join(
        dir,
        "wisconsin_test.data"),
    str,
    delimiter=',')


class Host:
    def __init__(self,
                 x,
                 y,
                 config
                 ):
        self.x = x
        self.y = y
        self.weights = np.zeros(x.shape[1])
        # self.weights = np.full(x.shape[1], fill_value=0.2)
        self.mask = 0
        self.data = {}
        self.config = config

    @staticmethod
    def compute_z_host(we, x):
        z_host = np.dot(x, we)
        return z_host

    def compute_u_host(self, batch_x, batch_y):
        z_host = Host.compute_z_host(self.weights, batch_x)
        u_host = 0.25 * z_host - batch_y * 0.5
        return u_host

    def compute_encrypted_dJ_host(self, batch_x, encrypted_u):
        encrypted_dJ_host = batch_x.T.dot(
            encrypted_u) + self.config['lambda'] * self.weights
        # encrypted_dJ_host = []
        # for dim in batch_x.T:
        #     multiply_dim_u = opt_paillier_cons_mul(self.data['public_key'], encrypted_u[0], int(dim[0]*(2**20)))
        #     for i in range(1,len(dim)):
        #         u_element = opt_paillier_cons_mul(self.data['public_key'], encrypted_u[i], int(dim[i]*(2**20)))
        #         multiply_dim_u = opt_paillier_add(self.data['public_key'], u_element, multiply_dim_u)
        #     # multiply_dim_u = + self.config['lambda'] * self.weights
        #     encrypted_dJ_host.append(multiply_dim_u)
        return encrypted_dJ_host

    def update_weight(self, dJ_host, batch_x):
        self.weights = self.weights - \
            self.config['lr'] * dJ_host / len(batch_x)

    '''
    host Send part of encrypted gradient and loss  to host
    '''

    def cal_u(self, batch_x, batch_y):
        self.data.update(proxy_server_arbiter.Get("pub", 10000))
        dt = self.data
        assert "public_key" in dt.keys(
        ), "Error: 'public_key' from arbiter not successfully received."
        public_key = dt['public_key']
        z_host = Host.compute_z_host(self.weights, batch_x)
        u_host = self.compute_u_host(batch_x, batch_y)
        encrypted_u_host = []
        for x in u_host:
            encrypted_u_host.append(public_key.encrypt(x))
        encrypted_u_host = np.asarray(encrypted_u_host)
        dt.update({"encrypted_u_host": encrypted_u_host})
        dt.update({"z_host": z_host})
        data_to_guest = {"encrypted_u_host": encrypted_u_host}
        # self.channel.send(data_to_guest)
        proxy_client_guest.Remote(data_to_guest, "encrypted_u_host")

    '''
        1.host receive the partial gradient from the guest
        2.add it to own gradient
        3.multiply it by  own X to get the full gradient
        4.plus the mask
        5.send the encrypted loss to the arbiter party to decrypt it
    '''

    def cal_dJ_loss(self, batch_x, batch_y):
        self.data.update(proxy_server_guest.Get("u_z", 10000))
        dt = self.data
        assert "encrypted_u_guest" in dt.keys(
        ), "Error: 'encrypt_u_guest' from guest not successfully received."
        # encrypted_u_a = self.channel.recv()
        encrypted_u_guest = dt['encrypted_u_guest']
        encrypted_u = encrypted_u_guest + dt["encrypted_u_host"]
        # encrypted_u = []
        # for x, y in zip(encrypted_u_guest, dt["encrypted_u_host"]):
        #     encrypted_u = np.append(
        #         encrypted_u, opt_paillier_add(
        #             dt['public_key'], x, y))
        encrypted_dJ_host = self.compute_encrypted_dJ_host(
            batch_x, encrypted_u)
        self.mask = np.random.rand(len(encrypted_dJ_host))
        encrypted_masked_dJ_host = encrypted_dJ_host + self.mask
        assert "encrypted_z_guest_square" in dt.keys(
        ), "Error: 'encrypted_z_guest_square' from guest in step 1 not successfully received."
        encrypted_z = 4 * encrypted_u_guest + dt['z_host']  # w*x
        encrypted_loss = np.sum(np.log(2) - 0.5 * batch_y *
                                encrypted_z +
                                0.125 *
                                dt["encrypted_z_guest_square"] +
                                0.125 *
                                dt["z_host"] *
                                (encrypted_z +
                                 4 *
                                 encrypted_u_guest))
        data_to_arbiter = {
            "encrypted_masked_dJ_host": encrypted_masked_dJ_host,
            "encrypted_loss": encrypted_loss}
        proxy_client_arbiter.Remote(data_to_arbiter, "host_dJ_loss")

    '''
        host receive arbiter's decrypted gradient minus its own mask
    '''

    def update(self, batch_x):
        self.data.update(proxy_server_arbiter.Get("masked_dJ_host", 10000))
        try:
            dt = self.data
            assert "masked_dJ_host" in dt.keys(
            ), "Error: 'masked_dJ_host' from arbiter  not successfully received."
            masked_dJ_host = dt['masked_dJ_host']
            # proxy_server_arbiter.StopRecvLoop()
            dJ_host = masked_dJ_host - self.mask
            self.update_weight(dJ_host, batch_x)
        except Exception as e:
            print("guest update weight exception: %s" % e)
        print(f"host weight: {self.weights}")
        return self.weights

    def batch_generator(self, all_data, batch_size, shuffle=True):
        """
        :param all_data : incluing features and label
        :param batch_size: number of samples in one batch
        :param shuffle: Whether to disrupt the order
        :return:iterator to generate every batch of features and labels
        """
        # Each element is a numpy array
        all_data = [np.array(d) for d in all_data]
        data_size = all_data[0].shape[0]
        print("data_size: ", data_size)
        if shuffle:
            p = np.random.permutation(data_size)
            all_data = [d[p] for d in all_data]
        batch_count = 0
        while True:
            # The epoch completes, disrupting the order once
            if batch_count * batch_size + batch_size > data_size:
                batch_count = 0
                if shuffle:
                    p = np.random.permutation(data_size)
                    all_data = [d[p] for d in all_data]
            start = batch_count * batch_size
            end = start + batch_size
            batch_count += 1
            yield [d[start: end] for d in all_data]

    @staticmethod
    def sigmoid(x):
        a = np.exp(x)
        a /= (1. + a)
        return a

    @staticmethod
    def getKS(y_test, y_pred_prob):
        '''
        calculate value ks, list fpr , list tpr , list thresholds

        y_pred_prob: probability of true
        y_test: real y
        return :fpr, tpr, thresholds, ks.
        ROC curve：abscissa：fpr  ordinate：tpr
        ks curve：abscissa：thresholds ordinate：fpr and tpr two curves
        '''
        fpr, tpr, thresholds = metrics.roc_curve(y_test, y_pred_prob)
        ks = max(tpr - fpr)
        return fpr, tpr, thresholds, ks

    @staticmethod
    def get_threshold(true_y, pred_prob):
        fpr, tpr, thresholds, ks = Host.getKS(true_y, pred_prob)
        max_ks = 0
        # re = Host.config["threshold"]
        re = 0.5
        for i in range(len(thresholds)):
            if tpr[i] - fpr[i] > max_ks:
                max_ks = tpr[i] - fpr[i]
                re = thresholds[i]
        return re

    @staticmethod
    def predict_score_to_output(pred_prob, classes, threshold):
        class_neg, class_pos = classes[0], classes[1]
        if isinstance(threshold, float):
            pred_re = list(
                map((lambda x: class_neg if x < threshold else class_pos), pred_prob))
        else:
            pred_re = list(
                map((lambda x: class_neg if x < 0.5 else class_pos), pred_prob))
        return pred_re

    @staticmethod
    def predict(we, data_instances, true_y, guest_re):
        # print("weights.shape", we.shape, "x_test.shape", data_instances.shape)
        pred_prob_en = Host.compute_z_host(we, data_instances) + guest_re
        mask = np.random.rand(len(pred_prob_en))
        pred_prob_en = pred_prob_en + mask
        data_to_arbiter = {"pred_prob_en": pred_prob_en}
        proxy_client_arbiter.Remote(data_to_arbiter, "pred_prob_en")
        pred_prob = proxy_server_arbiter.Get("pred_prob", 10000)
        pred_prob=pred_prob["pred_prob"]
        pred_prob = pred_prob - mask
        print("pred_prob",pred_prob)
        pred_prob = list(map(lambda x: Host.sigmoid(x), pred_prob))
        print("true_y.shape", true_y.shape)
        print(true_y)
        print("pred_prob", len(pred_prob), pred_prob)
        threshold = Host.get_threshold(true_y, pred_prob)
        print("threshold", threshold)
        predict_result = Host.predict_score_to_output(
            pred_prob, classes=[0, 1], threshold=threshold)
        print("predict_result", predict_result)
        evaluation_result = classification_eva.getResult(
            true_y, predict_result, pred_prob)
        with open("result_phe.txt", 'a') as f:
            f.write(f"predict_result:{predict_result}")
            f.write("\n\n=============================================\n\n")
            f.write(f"pred_prob:{pred_prob}")
            f.write("\n\n=============================================\n\n")
            f.write(f"threshold:{threshold}")
            f.write("\n\n=============================================\n\n")
            for i in evaluation_result:
                f.write(i)
                f.write("\n")
            f.write("\n\n=============================================\n\n")
        return predict_result, pred_prob, threshold, evaluation_result


if __name__ == "__main__":
    config = {
        'epochs': 1,
        'lambda': 10,
        'threshold': 0.5,
        'lr': 0.05,
        'batch_size': 200
    }
    # load train data
    x = data_host[1:, 1:-1]
    x = x.astype(float)
    label = data_host[1:, -1]
    label = label.astype(int)
    for i in range(len(label)):
        if label[i] == 2:
            label[i] = 0
        else:
            label[i] = 1
    count = x.shape[0]
    batch_num = count // config['batch_size'] + 1
    proxy_client_arbiter.Remote(batch_num, "batch_num")

    proxy_server_arbiter.StartRecvLoop()
    proxy_server_guest.StartRecvLoop()
    client_host = Host(x, label, config)
    batch_gen_host = client_host.batch_generator(
        [x, label], config['batch_size'], False)
    for i in range(config['epochs']):
        print("##### epoch %s ##### " % i)
        for j in range(batch_num):
            print("-----epoch=%s, batch=%s-----" % (i, j))
            batch_host_x, batch_host_y = next(batch_gen_host)
            print("batch_host_x.shape", batch_host_x.shape)
            print("batch_host_y.shape", batch_host_y.shape)
            client_host.cal_u(batch_host_x, batch_host_y)
            client_host.cal_dJ_loss(batch_host_x, batch_host_y)
            client_host.update(batch_host_x)
            print("batch=%s done" % j)
        print("epoch=%i done" % i)
    print("host training process done.")
    client_host.data.update(
        proxy_server_guest.Get(
            "encrypted_z_guest_test", 10000))
    assert "encrypted_z_guest_test" in client_host.data.keys(
    ), "Error: 'encrypted_z_guest_test' from guest  not successfully received."
    guest_re = client_host.data["encrypted_z_guest_test"]
    # weights = np.concatenate(
    #     [client_host.data["guest_weights"], client_host.weights], axis=0)
    # print("weights.shape", weights.shape)
    # print(weights)
    # load test data
    x = data_test[1:, 3:-1]
    x = x.astype(float)
    print("x.shape", x.shape)
    print(x[0])
    label = data_test[1:, -1]
    label = label.astype(int)
    for i in range(len(label)):
        if label[i] == 2:
            label[i] = 0
        else:
            label[i] = 1
    client_host.predict(client_host.weights, x, label, guest_re)
    proxy_server_arbiter.StopRecvLoop()
    proxy_server_guest.StopRecvLoop()
