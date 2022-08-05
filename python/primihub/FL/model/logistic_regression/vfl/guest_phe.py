from primihub.primitive.opt_paillier_c2py_warpper import *
import numpy as np
import time
from phe import paillier
from os import path
from primihub.FL.proxy.proxy import ServerChannelProxy
from primihub.FL.proxy.proxy import ClientChannelProxy
proxy_server_arbiter=ServerChannelProxy("10090")#guest接收arbiter消息
proxy_server_host=ServerChannelProxy("10093")#guest接收Host消息
proxy_client_arbiter=ClientChannelProxy("127.0.0.1","10094")#guest发送消息给arbiter
proxy_client_host=ClientChannelProxy("127.0.0.1","10095")#guest发送消息给host

dir = path.join(path.dirname(__file__), '../../../../tests/data')#路径注意
data_guest = np.loadtxt(path.join(dir, "wisconsin_guest.data"), str, delimiter=',')
data_host = np.loadtxt(path.join(dir, "wisconsin_host.data"), str, delimiter=',')
data_test=np.loadtxt(path.join(dir, "wisconsin_test.data"), str, delimiter=',')


class Guest:
    def __init__(self,
                 x,
                 config
                 ):
        self.x = x
        self.weights = np.zeros(x.shape[1])
        # self.weights=np.full(x.shape[1],fill_value=0.2)
        self.mask = 0
        self.data = {}
        self.config=config

    def compute_z_guest(self, we, x):
        z_guest = np.dot(x, we)
        return z_guest

    def compute_encrypted_dJ_guest(self, batch_x, encrypted_u):
        encrypted_dJ_guest = batch_x.T.dot(
            encrypted_u) + self.config['lambda'] * self.weights
        return encrypted_dJ_guest

    def update_weight(self, dJ_guest, batch_x):
        self.weights = self.weights - \
            self.config['lr'] * dJ_guest / len(batch_x)

    '''
    guest Send part of encrypted gradient and loss  to host
    '''

    def cal_uz(self, batch_x):
        self.data.update(proxy_server_arbiter.Get("pub",10000))
        dt = self.data
        assert "public_key" in dt.keys(
        ), "Error: 'public_key' from arbiter not successfully received."
        public_key = dt['public_key']
        z_guest = self.compute_z_guest(self.weights, batch_x)
        u_guest = 0.25 * z_guest
        z_guest_square = z_guest ** 2
        encrypted_u_guest = []
        encrypted_z_guest_square = []
        for x in u_guest:
            encrypted_u_guest.append(public_key.encrypt(x))
        for y in z_guest_square:
            encrypted_z_guest_square.append(public_key.encrypt(y))
        encrypted_u_guest = np.asarray(encrypted_u_guest)
        encrypted_z_guest_square = np.asarray(encrypted_z_guest_square)
        dt.update({"encrypted_u_guest": encrypted_u_guest})
        data_to_host = {"encrypted_u_guest": encrypted_u_guest,
                        "encrypted_z_guest_square": encrypted_z_guest_square}
        proxy_client_host.Remote(data_to_host,"u_z")

    '''
    1.guest receive the partial gradient from the host
    2.add it to own gradient
    3.multiply it by  own X to get the full gradient
    4.plus the mask
    5.send the encrypted loss to the arbiter party to decrypt it
    '''

    def cal_dJ(self, batch_x):
        self.data.update(proxy_server_host.Get("encrypted_u_host",10000))
        dt = self.data
        assert "encrypted_u_host" in dt.keys(
        ), "Error: 'encrypt_u_host' from host not successfully received."
        encrypted_u_host = dt['encrypted_u_host']
        encrypted_u = encrypted_u_host + dt["encrypted_u_guest"]
        # encrypted_u = []
        # for x, y in zip(encrypted_u_host, dt["encrypted_u_guest"]):
        #     encrypted_u = np.append(
        #         encrypted_u, opt_paillier_add(
        #             dt['public_key'], x, y))
        encrypted_dJ_guest = self.compute_encrypted_dJ_guest(
            batch_x, encrypted_u)
        self.mask = np.random.rand(len(encrypted_dJ_guest))
        encrypted_masked_dJ_guest = encrypted_dJ_guest + self.mask
        data_to_arbiter = {
            'encrypted_masked_dJ_guest': encrypted_masked_dJ_guest}
        proxy_client_arbiter.Remote(data_to_arbiter, "encrypted_masked_dJ_guest")


    '''
    guest receive arbiter's decrypted gradient minus its own mask
    '''

    def update(self, batch_x):
        self.data.update(proxy_server_arbiter.Get("masked_dJ_guest",10000))
        dt = self.data
        assert "masked_dJ_guest" in dt.keys(
        ), "Error: 'masked_dJ_guest' from arbiter  not successfully received."
        masked_dJ_guest = dt['masked_dJ_guest']
        dJ_guest = masked_dJ_guest - self.mask
        self.update_weight(dJ_guest, batch_x)
        print(f"guest weight: {self.weights}")
        return self.weights

    def batch_generator(self,all_data, batch_size, shuffle=True):
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

    def predict(self,w_test,x_test):
        self.data.update(proxy_server_arbiter.Get("pub", 10000))
        dt = self.data
        assert "public_key" in dt.keys(
        ), "Error: 'public_key' from arbiter not successfully received when testing."
        public_key = dt['public_key']
        z_guest_test = self.compute_z_guest(w_test, x_test)
        encrypted_z_guest_test = []
        for x in z_guest_test:
            encrypted_z_guest_test.append(public_key.encrypt(x))
        encrypted_z_guest_test = np.asarray(encrypted_z_guest_test)
        data_to_host = {"encrypted_z_guest_test":encrypted_z_guest_test}
        proxy_client_host.Remote(data_to_host, "encrypted_z_guest_test")


if __name__ == "__main__":
    config = {
        'epochs': 1,
        'lambda': 10,
        'threshold': 0.5,
        'lr': 0.05,
        'batch_size': 200
        }
    x = data_guest[1:, :]
    x=x.astype(np.float)
    count = x.shape[0]
    batch_num = count // config['batch_size'] + 1
    proxy_server_arbiter.StartRecvLoop()
    proxy_server_host.StartRecvLoop()
    client_guest = Guest(x, config)
    batch_gen_guest = client_guest.batch_generator(
        [x], config['batch_size'], False)
    for i in range(config['epochs']):
        print("##### epoch %s ##### " % i)
        for j in range(batch_num):
            print("-----epoch=%s, batch=%s-----" % (i, j))
            batch_guest_x= next(batch_gen_guest)[0]
            print("batch_guest_x.shape", batch_guest_x.shape)
            client_guest.cal_uz(batch_guest_x)
            client_guest.cal_dJ(batch_guest_x)
            client_guest.update(batch_guest_x)
            print("batch=%s done" % j)
        print("epoch=%i done" % i)
    print("guest training process done.")

    # load test data
    x_test = data_test[1:, 1:3]
    x_test = x_test.astype(float)
    print("x_test.shape", x_test.shape)
    print(x_test[0])
    client_guest.predict(client_guest.weights,x_test)
    proxy_server_host.StopRecvLoop()
    proxy_server_arbiter.StopRecvLoop()


