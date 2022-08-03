import math
import numpy as np
from primihub.primitive.opt_paillier_c2py_warpper import *
from phe import paillier
from primihub.FL.proxy.proxy import ServerChannelProxy
from primihub.FL.proxy.proxy import ClientChannelProxy
proxy_server_guest = ServerChannelProxy("10094")  # arbiter接收guest消息
proxy_server_host = ServerChannelProxy("10092")  # arbiter接收host消息
proxy_client_guest = ClientChannelProxy(
    "127.0.0.1", "10090")  # arbiter发送消息给guest
proxy_client_host = ClientChannelProxy(
    "127.0.0.1", "10091")  # arbiter发送消息给host


class Arbiter:
    """
    Arbiter is a trusted dealer.
    """

    def __init__(self,
                 batch_size,
                 # channel=None,
                 # random_seed=112
                 ):
        self.batch_size = batch_size
        # self.channel = channel
        # self.random_seed = random_seed
        self.public_key = None
        self.private_key = None
        # Save loss values (Taylor expansion approximation)
        self.data = {}
        self.loss = []

    # Arbiter: Send the public key to guest,host
    def send_pub(self):
        try:
            pub, prv = paillier.generate_paillier_keypair()
            self.public_key = pub
            self.private_key = prv
        except Exception as e:
            print("Arbiter generate key pair error : %s" % e)

        data_to_guesthost = {"public_key": self.public_key}
        print("start send pub")
        proxy_client_guest.Remote(data_to_guesthost, "pub")
        print("send pub to guest Ok")
        proxy_client_host.Remote(data_to_guesthost, "pub")
        print("send pub to host Ok")
        return

    # Arbiter:Receive the encrypted gradient from the guest host and decrypt
    # the received gradient

    def dec_gradient(self):
        self.data.update(proxy_server_guest.Get("encrypted_masked_dJ_guest",10000))
        self.data.update(proxy_server_host.Get("host_dJ_loss", 10000))
        dt = self.data
        assert "encrypted_masked_dJ_guest" in dt.keys() and "encrypted_masked_dJ_host" in dt.keys(
        ), "Error: 'masked_dJ_guest' from guest or 'masked_dJ_host' from host  not successfully received."
        encrypted_masked_dJ_guest = dt['encrypted_masked_dJ_guest']
        encrypted_masked_dJ_host = dt['encrypted_masked_dJ_host']
        masked_dJ_guest = np.asarray(
            [self.private_key.decrypt(x) for x in encrypted_masked_dJ_guest])
        masked_dJ_host = np.asarray(
            [self.private_key.decrypt(x) for x in encrypted_masked_dJ_host])
        print("masked_dJ_guest", masked_dJ_guest)
        print("masked_dJ_host", masked_dJ_host)
        assert "encrypted_loss" in dt.keys(
        ), "Error: 'encrypted_loss' from host  not successfully received."
        encrypted_loss = dt['encrypted_loss']
        loss = self.private_key.decrypt(
            encrypted_loss) / self.batch_size
        print("loss: ", loss)
        self.loss.append(loss)

        data_to_guest = {"masked_dJ_guest": masked_dJ_guest}
        data_to_host = {"masked_dJ_host": masked_dJ_host}
        proxy_client_guest.Remote(data_to_guest, "masked_dJ_guest")
        proxy_client_host.Remote(data_to_host, "masked_dJ_host")
        return

    def dec_re(self):
        self.data.update(proxy_server_host.Get("pred_prob_en", 10000))
        dt = self.data
        assert "pred_prob_en" in dt.keys(), "Error: 'pred_prob_en' from host not successfully received."
        pred_prob_en = dt['pred_prob_en']
        pred_prob = np.asarray(
            [self.private_key.decrypt(x) for x in pred_prob_en])
        print("pred_prob", pred_prob)
        data_to_host = {"pred_prob": pred_prob}
        proxy_client_host.Remote(data_to_host, "pred_prob")



if __name__ == "__main__":
    config = {
        'epochs': 1,
        'batch_size': 200
    }
    proxy_server_host.StartRecvLoop()
    batch_num = proxy_server_host.Get("batch_num")
    client_arbiter = Arbiter(config['batch_size'])
    proxy_server_guest.StartRecvLoop()

    for i in range(config['epochs']):
        print("##### epoch %s ##### " % i)
        for j in range(batch_num):
            print("-----epoch=%s, batch=%s-----" % (i, j))
            client_arbiter.send_pub()
            client_arbiter.dec_gradient()
            print("batch=%s done" % j)
        print("epoch=%i done" % i)
    print("All process done.")

    client_arbiter.send_pub()
    client_arbiter.dec_re()
    proxy_server_guest.StopRecvLoop()
    proxy_server_host.StopRecvLoop()
