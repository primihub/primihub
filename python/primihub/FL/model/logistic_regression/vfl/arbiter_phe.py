import math
import numpy as np
from primihub.primitive.opt_paillier_c2py_warpper import *
from phe import paillier
from primihub.FL.proxy.proxy import ServerChannelProxy
from primihub.FL.proxy.proxy import ClientChannelProxy
import logging


def get_logger(name):
    LOG_FORMAT = "[%(asctime)s][%(filename)s:%(lineno)d][%(levelname)s] %(message)s"
    DATE_FORMAT = "%m/%d/%Y %H:%M:%S %p"
    logging.basicConfig(level=logging.DEBUG,
                        format=LOG_FORMAT, datefmt=DATE_FORMAT)
    logger = logging.getLogger(name)
    return logger


logger = get_logger("hetero_LR_arbiter")


class Arbiter:
    """
    Arbiter is a trusted dealer.
    """

    def __init__(self, batch_size, proxy_server, proxy_client_guest, proxy_client_host):
        self.batch_size = batch_size
        self.public_key = None
        self.private_key = None
        self.data = {}
        self.loss = []
        self.proxy_server = proxy_server
        self.proxy_client_host = proxy_client_host
        self.proxy_client_guest = proxy_cient_guest

    # Arbiter: Send the public key to guest,host
    def send_pub(self):
        try:
            pub, prv = paillier.generate_paillier_keypair()
            self.public_key = pub
            self.private_key = prv
        except Exception as e:
            logger.error("Arbiter generate key pair error : %s" % e)

        data_to_guesthost = {"public_key": self.public_key}
        logger.info("start send pub")
        self.proxy_client_guest.Remote(data_to_guesthost, "pub")
        logger.info("send pub to guest Ok")
        self.proxy_client_host.Remote(data_to_guesthost, "pub")
        logger.info("send pub to host Ok")
        return

    # Arbiter:Receive the encrypted gradient from the guest host and decrypt
    # the received gradient

    def dec_gradient(self):
        self.data.update(proxy_server.Get("encrypted_masked_dJ_guest", 10000))
        self.data.update(proxy_server.Get("host_dJ_loss", 10000))
        dt = self.data
        assert "encrypted_masked_dJ_guest" in dt.keys() and "encrypted_masked_dJ_host" in dt.keys(
        ), "Error: 'masked_dJ_guest' from guest or 'masked_dJ_host' from host  not successfully received."
        encrypted_masked_dJ_guest = dt['encrypted_masked_dJ_guest']
        encrypted_masked_dJ_host = dt['encrypted_masked_dJ_host']
        masked_dJ_guest = np.asarray(
            [self.private_key.decrypt(x) for x in encrypted_masked_dJ_guest])
        masked_dJ_host = np.asarray(
            [self.private_key.decrypt(x) for x in encrypted_masked_dJ_host])
        logger.info("masked_dJ_guest", masked_dJ_guest)
        logger.info("masked_dJ_host", masked_dJ_host)
        assert "encrypted_loss" in dt.keys(
        ), "Error: 'encrypted_loss' from host  not successfully received."
        encrypted_loss = dt['encrypted_loss']
        loss = self.private_key.decrypt(
            encrypted_loss) / self.batch_size
        logger.info("loss: ", loss)
        self.loss.append(loss)

        data_to_guest = {"masked_dJ_guest": masked_dJ_guest}
        data_to_host = {"masked_dJ_host": masked_dJ_host}
        self.proxy_client_guest.Remote(data_to_guest, "masked_dJ_guest")
        selfproxy_client_host.Remote(data_to_host, "masked_dJ_host")
        return

    def dec_re(self):
        self.data.update(proxy_server.Get("pred_prob_en", 10000))
        dt = self.data
        assert "pred_prob_en" in dt.keys(
        ), "Error: 'pred_prob_en' from host not successfully received."
        pred_prob_en = dt['pred_prob_en']
        pred_prob = np.asarray(
            [self.private_key.decrypt(x) for x in pred_prob_en])
        logger.info("pred_prob", pred_prob)
        data_to_host = {"pred_prob": pred_prob}
        self.proxy_client_host.Remote(data_to_host, "pred_prob")


def run_hetero_lr_arbiter(role_node_map, node_addr_map, params_map={}):
    host_nodes = role_node_map["host"]
    guest_nodes = role_node_map["guest"]
    arbiter_nodes = role_node_map["arbiter"]

    if len(host_nodes) != 1:
        logger.error("Hetero LR only support one host party, but current "
                     "task have {} host party.".format(len(host_nodes)))
        return

    if len(guest_nodes) != 1:
        logger.error("Hetero LR only support one guest party, but current "
                     "task have {} guest party.".format(len(host_nodes)))
        return

    if len(arbiter_nodes) != 1:
        logger.error("Hetero LR only support one arbiter party, but current "
                     "task have {} arbiter party.".format(len(host_nodes)))
        return

    arbiter_port = node_addr_map[arbiter_nodes[0]].split(":")
    proxy_server = ServerChannelProxy(arbiter_port)
    proxy_server.StartRecvLoop()

    host_ip, host_port = node_addr_map[host_nodes[0]].split(":")
    self.proxy_client_host = ClientChannelProxy(host_ip, host_port)

    guest_ip, guest_port = node_addr_map[guest_nodes[0]].split(":")
    self.proxy_client_guest = ClientChannelProxy(guest_ip, guest_port)

    config = {
        'epochs': 1,
        'batch_size': 200
    }

    batch_num = proxy_server.Get("batch_num")
    client_arbiter = Arbiter(
        config['batch_size'], proxy_server, proxy_client_guest, proxy_client_host)

    for i in range(config['epochs']):
        logger.info("##### epoch %s ##### " % i)
        for j in range(batch_num):
            logger.info("-----epoch=%s, batch=%s-----" % (i, j))
            client_arbiter.send_pub()
            client_arbiter.dec_gradient()
            logger.info("batch=%s done" % j)
        logger.info("epoch=%i done" % i)
    logger.info("All process done.")

    client_arbiter.send_pub()
    client_arbiter.dec_re()
    proxy_server.StopRecvLoop()
