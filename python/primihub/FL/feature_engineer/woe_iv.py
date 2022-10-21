import primihub as ph
from primihub import dataset
import pandas as pd
import numpy as np
import os
import logging
import time
# from loguru import logger
from primihub.FL.proxy.proxy import ServerChannelProxy
from primihub.FL.proxy.proxy import ClientChannelProxy


class MyServerChannelProxy(ServerChannelProxy):
    def __init__(self, port):
        super().__init__(port)

    def Get(self, tag, retries=100):
        for i in range(retries):
            val = self.recv_cache_.get(tag, None)
            if val is not None:
                del self.recv_cache_[tag]
                logging.debug("Get val with tag '{}' finish.".format(tag))
                return val
            else:
                time.sleep(0.3)

        logging.warn("Can't get value for tag '{}', timeout.".format(tag))
        return None


def cal_hist_bins(features, cut_points, label):
    m, _ = features.shape
    pos_cnts = []
    neg_cnts = []

    for i in range(len(cut_points)):
        tmp_point = cut_points[i, :]
        exp_tmp_point = np.tile(tmp_point, (m, 1))

        less_flag = (features < exp_tmp_point).astype('int').T
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

    return pos_cnts, neg_cnts


def woe_iv(host_bin_cnts, guest_bin_cnts):
    global_pos_cnts = host_bin_cnts['pos_cnts'] + guest_bin_cnts['pos_cnts']
    global_neg_cnts = host_bin_cnts['neg_cnts'] + guest_bin_cnts['neg_cnts']
    logging.info("global_pos_cnts and global_neg_cnts are: {} {}".format(
        global_pos_cnts, global_neg_cnts))

    # global_cnts = global_pos_cnts + global_pos_cnts
    sum_pos_cnts = np.sum(global_pos_cnts, axis=0)
    sum_neg_cnts = np.sum(global_neg_cnts, axis=0)

    exp_sum_pos_cnts = np.tile(sum_pos_cnts, (len(global_pos_cnts), 1))
    exp_sum_neg_cnts = np.tile(sum_neg_cnts, (len(global_neg_cnts), 1))

    global_pos_rates = np.maximum(global_pos_cnts, 0.5) / exp_sum_pos_cnts
    global_neg_rates = np.maximum(global_neg_cnts, 0.5) / exp_sum_neg_cnts

    woes = np.log(global_pos_rates / global_neg_rates)
    logging.info("Global woes are: {}".format(woes))

    diff = (global_pos_rates - global_neg_rates)
    ivs = np.dot(diff.T, woes).diagonal()

    return woes, ivs


def cut_points(host_max_min, guest_max_min, bins=10):
    max_arr = np.vstack([host_max_min['max_arr'], guest_max_min['max_arr']])
    min_arr = np.vstack([host_max_min['min_arr'], guest_max_min['min_arr']])

    global_max = np.max(max_arr, axis=0)
    global_min = np.min(min_arr, axis=0)

    split_points = []
    bin_width = (global_max - global_min) / bins

    for _ in range(bins):
        cur_point = global_min + bin_width
        global_min = cur_point

        split_points.append(cur_point)

    split_points = np.array(split_points)

    return split_points


@ph.context.function(role='arbiter', protocol='woe-iv', datasets=['breast_0'], port='9010', task_type="feature-engineer")
def iv_arbiter(bins=15):
    logging.info("Start woe-iv arbiter.")

    role_node_map = ph.context.Context.get_role_node_map()
    node_addr_map = ph.context.Context.get_node_addr_map()

    host_nodes = role_node_map["host"]
    guest_nodes = role_node_map["guest"]
    arbiter_nodes = role_node_map["arbiter"]

    logging.info("host_nodes: {}, guest_nodes: {}, arbiter_nodes: {}".format(
        host_nodes, guest_nodes, arbiter_nodes))

    arbiter_port = node_addr_map[arbiter_nodes[0]].split(":")[1]
    proxy_server = MyServerChannelProxy(arbiter_port)
    proxy_server.StartRecvLoop()

    host_ip, host_port = node_addr_map[host_nodes[0]].split(":")
    proxy_client_host = ClientChannelProxy(host_ip, host_port, "host")

    guest_ip, guest_port = node_addr_map[guest_nodes[0]].split(":")
    proxy_client_guest = ClientChannelProxy(guest_ip, guest_port, "guest")

    # client_arbiter = Arbiter(proxy_server, proxy_client_host,
    #                          proxy_client_guest)
    host_max_min = proxy_server.Get('host_max_min')
    guest_max_min = proxy_server.Get('guest_max_min')

    split_points = cut_points(host_max_min, guest_max_min, bins=bins)

    logging.info("Starting send split points: {}".format(split_points))
    proxy_client_host.Remote(split_points, "split_points")
    proxy_client_guest.Remote(split_points, 'split_points')
    logging.info("Ending send split points")

    host_bin_cnts = proxy_server.Get('host_bin_cnts')
    guest_bin_cnts = proxy_server.Get('guest_bin_cnts')

    _, ivs = woe_iv(host_bin_cnts, guest_bin_cnts)

    ivs_df = pd.DataFrame(ivs.reshape(1, -1))
    ivs_df.columns = host_max_min['headers']
    logging.info("Global ivs are: {}".format(ivs_df))


@ph.context.function(role='host', protocol='woe-iv', datasets=['iv_host'], port='9020', task_type="feature-engineer")
def iv_host():

    logging.info("Start woe-iv host.")

    role_node_map = ph.context.Context.get_role_node_map()
    node_addr_map = ph.context.Context.get_node_addr_map()
    dataset_map = ph.context.Context.dataset_map
    data_key = list(dataset_map.keys())[0]

    logging.info("role_node_map: {}, node_addr_map: {}".format(
        role_node_map, node_addr_map))

    host_nodes = role_node_map["host"]
    arbiter_nodes = role_node_map["arbiter"]

    host_port = node_addr_map[host_nodes[0]].split(":")[1]
    arbiter_ip, arbiter_port = node_addr_map[arbiter_nodes[0]].split(":")

    # host_port = node_addr_map[host_nodes[0]].split(":")[1]
    # host_port = host_info['port']
    proxy_server = MyServerChannelProxy(host_port)
    proxy_server.StartRecvLoop()
    logging.info("Create server proxy for host, port {}.".format(host_port))

    # arbiter_ip, arbiter_port = node_addr_map[arbiter_nodes[0]].split(":")
    # arbiter_ip, arbiter_port = arbiter_info['ip'], arbiter_info['port']
    proxy_client_arbiter = ClientChannelProxy(arbiter_ip, arbiter_port,
                                              "arbiter")
    logging.info("Create client proxy to arbiter,"
                 " ip {}, port {}.".format(arbiter_ip, arbiter_port))

    data = ph.dataset.read(dataset_key=data_key).df_data
    label = data.pop('y').values
    headers = data.columns

    data_arr = data.values
    m, n = data_arr.shape
    # bins = 15
    max_arr = np.max(data_arr, axis=0)
    min_arr = np.min(data_arr, axis=0)
    proxy_client_arbiter.Remote(
        {'max_arr': max_arr, 'min_arr': min_arr, 'headers': headers}, "host_max_min")

    split_points = proxy_server.Get('split_points')

    pos_cnts, neg_cnts = cal_hist_bins(data_arr, split_points, label)

    logging.info('Host bin counts are: {} and {}'.format(
        np.array(pos_cnts), np.array(neg_cnts)))
    proxy_client_arbiter.Remote({'pos_cnts': np.array(
        pos_cnts), 'neg_cnts': np.array(neg_cnts)}, 'host_bin_cnts')


@ph.context.function(role='guest', protocol='woe-iv', datasets=['iv_guest'], port='9030', task_type="feature-engineer")
def iv_guest():

    logging.info("Start woe-iv guest.")

    role_node_map = ph.context.Context.get_role_node_map()
    node_addr_map = ph.context.Context.get_node_addr_map()
    dataset_map = ph.context.Context.dataset_map
    data_key = list(dataset_map.keys())[0]

    logging.info("role_node_map: {}, node_addr_map: {}".format(
        role_node_map, node_addr_map))

    guest_nodes = role_node_map["guest"]
    arbiter_nodes = role_node_map["arbiter"]

    guest_port = node_addr_map[guest_nodes[0]].split(":")[1]
    proxy_server = MyServerChannelProxy(guest_port)
    proxy_server.StartRecvLoop()

    arbiter_ip, arbiter_port = node_addr_map[arbiter_nodes[0]].split(":")

    proxy_client_arbiter = ClientChannelProxy(arbiter_ip, arbiter_port,
                                              "arbiter")

    data = ph.dataset.read(dataset_key=data_key).df_data
    label = data.pop('y')

    data_arr = data.values
    m, n = data_arr.shape
    # bins = 15
    max_arr = np.max(data_arr, axis=0)
    min_arr = np.min(data_arr, axis=0)

    proxy_client_arbiter.Remote(
        {'max_arr': max_arr, 'min_arr': min_arr}, "guest_max_min")

    split_points = proxy_server.Get('split_points')

    pos_cnts, neg_cnts = cal_hist_bins(data_arr, split_points, label)

    logging.info('Guest bin counts are: {} and {}'.format(
        np.array(pos_cnts), np.array(neg_cnts)))
    proxy_client_arbiter.Remote({'pos_cnts': np.array(
        pos_cnts), 'neg_cnts': np.array(neg_cnts)}, 'guest_bin_cnts')
