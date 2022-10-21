import primihub as ph
from primihub import dataset
import pandas as pd
import numpy as np
import os
from primihub.utils.logger_util import logger
from primihub.FL.proxy.proxy import ServerChannelProxy
from primihub.FL.proxy.proxy import ClientChannelProxy


class Arbiter:
    """
    Tips: Arbiter is a trusted third party !!!
    """

    def __init__(self, proxy_server, proxy_client_host, proxy_client_guest):
        self.need_one_vs_rest = None
        self.public_key = None
        self.private_key = None
        self.need_encrypt = None
        self.epoch = None
        self.weight_host = None
        self.weight_guest = None
        self.theta = None
        self.proxy_server = proxy_server
        self.proxy_client_host = proxy_client_host
        self.proxy_client_guest = proxy_client_guest

    def model_aggregate(self, host_parm, guest_param, host_data_weight,
                        guest_data_weight):
        param = []
        weight_all = []
        if self.need_encrypt == 'YES':
            if self.need_one_vs_rest == False:
                param.append(self.decrypt_vector(host_parm))
            else:
                param.append(self.decrypt_matrix(host_parm))
            weight_all.append(self.decrypt_vector(host_data_weight)[0])
        else:
            param.append(host_parm)
            weight_all.append(host_data_weight)
        param.append(guest_param)
        weight_all.append(guest_data_weight)
        weight = np.array([weight * 1.0 for weight in weight_all])
        if self.need_one_vs_rest == True:
            agg_param = np.zeros_like(np.array(param))
            for id_c, p in enumerate(param):
                w = weight[id_c] / np.sum(weight, axis=0)
                for id_d, d in enumerate(p):
                    d = np.array(d)
                    agg_param[id_c, id_d] += d * w
            self.theta = np.sum(agg_param, axis=0)
            return list(self.theta)
        else:
            agg_param = np.zeros(len(host_parm))
            for id_c, p in enumerate(param):
                w = weight[id_c] / np.sum(weight, axis=0)
                for id_d, d in enumerate(p):
                    agg_param[id_d] += d * w
            self.theta = agg_param
            return list(self.theta)

    def broadcast_global_model_param(self, host_param, guest_param,
                                     host_data_weight, guest_data_weight):
        self.theta = self.model_aggregate(host_param, guest_param,
                                          host_data_weight, guest_data_weight)
        # send guest plaintext
        self.proxy_client_guest.Remote(self.theta, "global_guest_model_param")
        # send host ciphertext
        if self.need_encrypt == 'YES':
            if self.need_one_vs_rest == False:
                self.theta = self.encrypt_vector(self.theta)
            else:
                self.theta = self.encrypt_matrix(self.theta)

            self.proxy_client_host.Remote(
                self.theta, "global_host_model_param")
        else:
            self.proxy_client_host.Remote(
                self.theta, "global_host_model_param")


@ph.context.function(role='arbiter', protocol='woe-iv', datasets=['iv_arbiter'], port='9010', task_type="feature-engineer")
def iv_arbiter(bins=15):
    logger.info("Start woe-iv arbiter.")

    role_node_map = ph.context.Context.get_role_node_map()
    node_addr_map = ph.context.Context.get_node_addr_map()

    host_nodes = role_node_map["host"]
    guest_nodes = role_node_map["guest"]
    arbiter_nodes = role_node_map["arbiter"]

    logger.info("host_nodes: {}, guest_nodes: {}, arbiter_nodes: {}".format(
        host_nodes, guest_nodes, arbiter_nodes))

    arbiter_port = node_addr_map[arbiter_nodes[0]].split(":")[1]
    proxy_server = ServerChannelProxy(arbiter_port)
    proxy_server.StartRecvLoop()

    host_ip, host_port = node_addr_map[host_nodes[0]].split(":")
    proxy_client_host = ClientChannelProxy(host_ip, host_port, "host")

    guest_ip, guest_port = node_addr_map[guest_nodes[0]].split(":")
    proxy_client_guest = ClientChannelProxy(guest_ip, guest_port, "guest")

    # client_arbiter = Arbiter(proxy_server, proxy_client_host,
    #                          proxy_client_guest)
    host_max_min = proxy_server.Get('host_max_min')
    guest_max_min = proxy_server.Get('guest_max_min')

    max_arr = np.vstack([host_max_min['max_arr'], guest_max_min['max_arr']])
    min_arr = np.vstack([host_max_min['min_arr'], guest_max_min['min_arr']])

    global_max = np.max(max_arr, axis=0)
    global_min = np.min(min_arr, axis=0)

    split_points = []
    bin_width = (global_max - global_min) / bins

    for _ in range(bins):
        cur_point = global_min + bin_width
        split_points.append(cur_point)

    split_points = np.array(split_points)

    logger.info("Starting send split points: {}".format(split_points))
    proxy_client_host.Remote(split_points, "split_points")
    proxy_client_guest.Remote(split_points, 'split_points')
    logger.info("Ending send split points")

    host_bin_cnts = proxy_server.Get('host_bin_cnts')
    guest_bin_cnts = proxy_server.Get('guest_bin_cnts')

    global_pos_cnts = host_bin_cnts['pos_cnts'] + guest_bin_cnts['pos_cnts']
    global_neg_cnts = host_bin_cnts['neg_cnts'] + guest_bin_cnts['neg_cnts']
    global_cnts = global_pos_cnts + global_pos_cnts

    woes = np.log(global_pos_cnts / global_neg_cnts)
    logger.info("Global woes are: {}".format(woes))
    # exp_global_cn

    # global_bin_cnts = host_bin_cnts + guest_bin_cnts

    # max_min = np.vstack([host_max_min, guest_max_min])


@ph.context.function(role='host', protocol='woe-iv', datasets=['iv_host'], port='9020', task_type="feature-engineer")
def iv_host():

    logger.info("Start woe-iv host.")

    role_node_map = ph.context.Context.get_role_node_map()
    node_addr_map = ph.context.Context.get_node_addr_map()
    dataset_map = ph.context.Context.dataset_map
    data_key = list(dataset_map.keys())[0]

    logger.info("role_node_map: {}, node_addr_map: {}".format(
        role_node_map, node_addr_map))

    host_nodes = role_node_map["host"]
    arbiter_nodes = role_node_map["arbiter"]

    host_port = node_addr_map[host_nodes[0]].split(":")[1]
    arbiter_ip, arbiter_port = node_addr_map[arbiter_nodes[0]].split(":")

    # host_port = node_addr_map[host_nodes[0]].split(":")[1]
    # host_port = host_info['port']
    proxy_server = ServerChannelProxy(host_port)
    proxy_server.StartRecvLoop()
    logger.info("Create server proxy for host, port {}.".format(host_port))

    # arbiter_ip, arbiter_port = node_addr_map[arbiter_nodes[0]].split(":")
    # arbiter_ip, arbiter_port = arbiter_info['ip'], arbiter_info['port']
    proxy_client_arbiter = ClientChannelProxy(arbiter_ip, arbiter_port,
                                              "arbiter")
    logger.info("Create client proxy to arbiter,"
                " ip {}, port {}.".format(arbiter_ip, arbiter_port))

    data = ph.dataset.read(dataset_key=data_key).df_data
    label = data.pop('y').values

    data_arr = data.values
    m, n = data_arr.shape
    # bins = 15
    max_arr = np.max(data_arr, axis=0)
    min_arr = np.min(data_arr, axis=0)
    proxy_client_arbiter.Remote(
        {'max_arr': max_arr, 'min_arr': min_arr}, "host_max_min")

    split_points = proxy_server.Get('split_points')
    pos_cnts = []
    neg_cnts = []

    for i in range(len(split_points)):
        tmp_point = split_points[i, :]
        exp_tmp_point = np.tile(tmp_point, (m, 1))

        less_flag = (data_arr < exp_tmp_point).astype('int').T
        pos_num = np.dot(less_flag, label)
        neg_num = np.dot(less_flag, 1-label)
        # less_sum = np.sum(less_flag, axis=0)

        if i == 0:
            pos_cnts.append(pos_num)
            neg_cnts.append(neg_num)
        else:
            pos_cnts.append(pos_num - pre_pos_num)
            neg_cnts.append(neg_num - pre_neg_num)
        # pre_sum = less_sum
        pre_pos_num = pos_num
        pre_neg_num = neg_num

    logger.info('Host bin counts are: {} and {}'.format(
        np.array(pos_cnts), np.array(neg_cnts)))
    proxy_client_arbiter.Remote({'pos_cnts': np.array(
        pos_cnts), 'neg_cnts': np.array(neg_cnts)}, 'host_bin_cnts')


@ph.context.function(role='guest', protocol='woe-iv', datasets=['iv_guest'], port='9030', task_type="feature-engineer")
def iv_guest():

    logger.info("Start woe-iv guest.")

    role_node_map = ph.context.Context.get_role_node_map()
    node_addr_map = ph.context.Context.get_node_addr_map()
    dataset_map = ph.context.Context.dataset_map
    data_key = list(dataset_map.keys())[0]

    logger.info("role_node_map: {}, node_addr_map: {}".format(
        role_node_map, node_addr_map))

    guest_nodes = role_node_map["guest"]
    arbiter_nodes = role_node_map["arbiter"]

    guest_port = node_addr_map[guest_nodes[0]].split(":")[1]
    proxy_server = ServerChannelProxy(guest_port)
    proxy_server.StartRecvLoop()

    arbiter_ip, arbiter_port = node_addr_map[arbiter_nodes[0]].split(":")

    proxy_client_arbiter = ClientChannelProxy(arbiter_ip, arbiter_port,
                                              "arbiter")

    data = ph.dataset.read(dataset_key=data_key).df_data
    label = data.pop('y')

    data_arr = data.values
    # bins = 15
    max_arr = np.max(data_arr, axis=0)
    min_arr = np.min(data_arr, axis=0)

    proxy_client_arbiter.Remote(
        {'max_arr': max_arr, 'min_arr': min_arr}, "guest_max_min")

    split_points = proxy_server.Get('split_points')
    pos_cnts = []
    neg_cnts = []

    for i in range(len(split_points)):
        tmp_point = split_points[i, :]
        exp_tmp_point = np.tile(tmp_point, (m, 1))

        less_flag = (data_arr < exp_tmp_point).astype('int').T
        pos_num = np.dot(less_flag, label)
        neg_num = np.dot(less_flag, 1-label)
        # less_sum = np.sum(less_flag, axis=0)

        if i == 0:
            pos_cnts.append(pos_num)
            neg_cnts.append(neg_num)
        else:
            pos_cnts.append(pos_num - pre_pos_num)
            neg_cnts.append(neg_num - pre_neg_num)
        # pre_sum = less_sum
        pre_pos_num = pos_num
        pre_neg_num = neg_num

    logger.info('Guest bin counts are: {} and {}'.format(
        np.array(pos_cnts), np.array(neg_cnts)))
    proxy_client_arbiter.Remote({'pos_cnts': np.array(
        pos_cnts), 'neg_cnts': np.array(neg_cnts)}, 'guest_bin_cnts')
