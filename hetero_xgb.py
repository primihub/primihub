import primihub as ph
from primihub import dataset, context
from phe import paillier
from sklearn import metrics
from primihub.primitive.opt_paillier_c2py_warpper import *
from primihub.FL.model.evaluation.evaluation import Regression_eva
from primihub.FL.model.evaluation.evaluation import Classification_eva
import pandas as pd
import numpy as np
import logging
import pickle

from primihub.primitive.opt_paillier_c2py_warpper import *
import time
import pandas as pd
import numpy as np
import copy
import logging
import time
from concurrent.futures import ThreadPoolExecutor
from primihub.channel.zmq_channel import IOService, Session
import functools
import ray
from ray.util import ActorPool
from line_profiler import LineProfiler

LOG_FORMAT = "[%(asctime)s][%(filename)s:%(lineno)d][%(levelname)s] %(message)s"
DATE_FORMAT = "%m/%d/%Y %H:%M:%S %p"
logging.basicConfig(level=logging.DEBUG, format=LOG_FORMAT, datefmt=DATE_FORMAT)
logger = logging.getLogger("proxy")


def opt_paillier_decrypt_crt(pub, prv, cipher_text):

    if not isinstance(cipher_text, Opt_paillier_ciphertext):
        print(f"{cipher_text} should be type of Opt_paillier_ciphertext()")
        return

    decrypt_text = opt_paillier_c2py.opt_paillier_decrypt_crt_warpper(
        pub, prv, cipher_text)

    decrypt_text_num = int(decrypt_text)

    return decrypt_text_num


@ray.remote
class PaillierActor(object):

    def __init__(self, prv, pub) -> None:
        self.prv = prv
        self.pub = pub

    def pai_enc(self, item):
        return opt_paillier_encrypt_crt(self.pub, self.prv, item)

    def pai_dec(self, item):
        return opt_paillier_decrypt_crt(self.pub, self.prv, item)

    def pai_add(self, enc1, enc2):
        return opt_paillier_add(self.pub, enc1, enc2)


@ray.remote
class ActorAdd(object):

    def __init__(self, pub):
        self.pub = pub

    def add(self, values):
        tmp_sum = None
        for item in values:
            if tmp_sum is None:
                tmp_sum = item
            else:
                tmp_sum = opt_paillier_add(self.pub, tmp_sum, item)
        return tmp_sum


@ray.remote
class PallierAdd(object):

    def __init__(self, pub, nums, add_actors, is_encrypted=True):
        self.pub = pub
        self.nums = nums
        self.add_actors = add_actors
        self.encrypted = is_encrypted

    def pai_add(self, items, min_num=3):
        if self.encrypted:
            nums = self.nums * min_num
            if len(items) < nums:
                return functools.reduce(
                    lambda x, y: opt_paillier_add(self.pub, x, y), items)
            N = int(len(items) / nums)
            items_list = []

            inter_results = []
            for i in range(nums):
                tmp_val = items[i * N:(i + 1) * N]
                # tmp_add_actor = self.add_actors[i]
                if i == (nums - 1):
                    tmp_val = items[i * N:]
                items_list.append(tmp_val)

            inter_results = list(
                self.add_actors.map(lambda a, v: a.add.remote(v), items_list))

            #     tmp_g_left, tmp_g_right, tmp_h_left,tmp_h_right  = list(
            #         self.pools.map(lambda a, v: a.pai_add.remote(v), [G_left_g, G_right_g, H_left_h, H_right_h]))
            # # inter_results = [ActorAdd.remote(self.pub, items[i*N:(i+1)*N]).add.remote() for i in range(nums)]
            # final_result = ray.get(inter_results)
            final_result = functools.reduce(
                lambda x, y: opt_paillier_add(self.pub, x, y), inter_results)
            # final_results = ActorAdd.remote(self.pub, ray.get(inter_results)).add.remote()
        else:
            final_result = sum(items)

        return final_result


@ray.remote
class MapGH(object):

    def __init__(self, item, col, cut_points, g, h, pub, min_child_sample,
                 pools):
        self.item = item
        self.col = col
        self.cut_points = cut_points
        self.g = g
        self.h = h
        self.pub = pub
        self.min_child_sample = min_child_sample
        self.pools = pools

    def map_gh(self):
        if isinstance(self.col, pd.DataFrame) or isinstance(self.g, pd.Series):
            self.col = self.col.values

        if isinstance(self.g, pd.DataFrame) or isinstance(self.g, pd.Series):
            self.g = self.g.values

        if isinstance(self.h, pd.DataFrame) or isinstance(self.h, pd.Series):
            self.h = self.h.values

        G_lefts = []
        G_rights = []
        H_lefts = []
        H_rights = []
        vars = []
        cuts = []

        try:
            candidate_points = self.cut_points.values
        except:
            candidate_points = self.cut_points

        for tmp_cut in candidate_points:
            flag = (self.col < tmp_cut)
            less_sum = sum(flag.astype('int'))
            great_sum = sum((1 - flag).astype('int'))

            if self.min_child_sample:
                if (less_sum < self.min_child_sample) \
                        | (great_sum < self.min_child_sample):
                    continue

            G_left_g = self.g[flag].tolist()
            G_right_g = self.g[(1 - flag).astype('bool')].tolist()
            H_left_h = self.h[flag].tolist()
            H_right_h = self.h[(1 - flag).astype('bool')].tolist()
            print("++++++++++", len(G_left_g), len(G_right_g), len(H_left_h),
                  len(H_right_h))

            tmp_g_left, tmp_g_right, tmp_h_left, tmp_h_right = list(
                self.pools.map(lambda a, v: a.pai_add.remote(v),
                               [G_left_g, G_right_g, H_left_h, H_right_h]))
            # tmp_g_left, tmp_g_right, tmp_h_left,tmp_h_right = list(ray.get([self.pools.pai_add.remote(tmp_li, nums) for tmp_li in [G_left_g, G_right_g, H_left_h, H_right_h]]))

            # tmp_g_left = functools.reduce(
            #     lambda x, y: opt_paillier_add(self.pub, x, y), G_left_g)
            # tmp_g_right = functools.reduce(
            #     lambda x, y: opt_paillier_add(self.pub, x, y), G_right_g)
            # tmp_h_left = functools.reduce(
            #     lambda x, y: opt_paillier_add(self.pub, x, y), H_left_h)
            # tmp_h_right = functools.reduce(
            #     lambda x, y: opt_paillier_add(self.pub, x, y), H_right_h)

            G_lefts.append(tmp_g_left)
            G_rights.append(tmp_g_right)
            H_lefts.append(tmp_h_left)
            H_rights.append(tmp_h_right)
            vars.append(self.item)
            cuts.append(tmp_cut)

        return G_lefts, G_rights, H_lefts, H_rights, vars, cuts


@ray.remote
class ReduceGH(object):

    def __init__(self, maps) -> None:
        self.maps = maps

    def reduce_gh(self):
        global_g_left = []
        global_g_right = []
        global_h_left = []
        global_h_right = []
        global_vars = []
        global_cuts = []
        reduce_gh = ray.get([tmp_map.map_gh.remote() for tmp_map in self.maps])

        for tmp_gh in reduce_gh:
            tmp_g_left, tmp_g_right, tmp_h_left, tmp_h_right, tmp_var, tmp_cut = tmp_gh
            global_g_left += tmp_g_left
            global_g_right += tmp_g_right
            global_h_left += tmp_h_left
            global_h_right += tmp_h_right
            global_vars += tmp_var
            global_cuts += tmp_cut

        GH = pd.DataFrame({
            'G_left': global_g_left,
            'G_right': global_g_right,
            'H_left': global_h_left,
            'H_right': global_h_right,
            'var': global_vars,
            'cut': global_cuts
        })

        return GH


def phe_map_enc(pub, pri, item):
    # return pub.encrypt(item)
    return opt_paillier_encrypt_crt(pub, pri, item)


def phe_map_dec(pub, prv, item):
    if not item:
        return
    # return pri.decrypt(item)
    return opt_paillier_decrypt_crt(pub, prv, item)


def phe_add(enc1, enc2):
    return enc1 + enc2


# def opt_paillier_add(pub, x, y):
#     return opt_paillier_add(pub, x, y)


class ClientChannelProxy:

    def __init__(self, host, port, dest_role="NotSetYet"):
        self.ios_ = IOService()
        self.sess_ = Session(self.ios_, host, port, "client")
        self.chann_ = self.sess_.addChannel()
        self.executor_ = ThreadPoolExecutor()
        self.host = host
        self.port = port
        self.dest_role = dest_role

    # Send val and it's tag to server side, server
    # has cached val when the method return.
    def Remote(self, val, tag):
        msg = {"v": pickle.dumps(val), "tag": tag}
        self.chann_.send(msg)
        _ = self.chann_.recv()
        logger.debug("Send value with tag '{}' to {} finish".format(
            tag, self.dest_role))

    # Send val and it's tag to server side, client begin the send action
    # in a thread when the the method reutrn but not ensure that server
    # has cached this val. Use 'fut.result()' to wait for server to cache it,
    # this makes send value and other action running in the same time.
    def RemoteAsync(self, val, tag):

        def send_fn(channel, msg):
            channel.send(msg)
            _ = channel.recv()

        msg = {"v": val, "tag": tag}
        fut = self.executor_.submit(send_fn, self.chann_, msg)

        return fut


class ServerChannelProxy:

    def __init__(self, port):
        self.ios_ = IOService()
        self.sess_ = Session(self.ios_, "*", port, "server")
        self.chann_ = self.sess_.addChannel()
        self.executor_ = ThreadPoolExecutor()
        self.recv_cache_ = {}
        self.stop_signal_ = False
        self.recv_loop_fut_ = None

    # Start a recv thread to receive value and cache it.
    def StartRecvLoop(self):

        def recv_loop():
            logger.info("Start recv loop.")
            while (not self.stop_signal_):
                try:
                    msg = self.chann_.recv(block=False)
                except Exception as e:
                    logger.error(e)
                    break

                if msg is None:
                    continue

                key = msg["tag"]
                value = msg["v"]
                if self.recv_cache_.get(key, None) is not None:
                    logger.warn(
                        "Hash entry for tag '{}' is not empty, replace old value"
                        .format(key))
                    del self.recv_cache_[key]

                logger.debug("Recv msg with tag '{}'.".format(key))
                self.recv_cache_[key] = value
                self.chann_.send("ok")
            logger.info("Recv loop stops.")

        self.recv_loop_fut_ = self.executor_.submit(recv_loop)

    # Stop recv thread.
    def StopRecvLoop(self):
        self.stop_signal_ = True
        self.recv_loop_fut_.result()
        logger.info("Recv loop already exit, clean cached value.")
        key_list = list(self.recv_cache_.keys())
        for key in key_list:
            del self.recv_cache_[key]
            logger.warn(
                "Remove value with tag '{}', not used until now.".format(key))
        # del self.recv_cache_
        logger.info("Release system resource!")
        self.chann_.socket.close()
        # self.chann_.term()
        # self.chann_.context.destroy()
        # self.chann_.socket.destroy()

    # Get value from cache, and the check will repeat at most 'retries' times,
    # and sleep 0.3s after each check to avoid check all the time.
    def Get(self, tag, max_time=10000, interval=0.1):
        start = time.time()
        while True:
            val = self.recv_cache_.get(tag, None)
            end = time.time()
            if val is not None:
                del self.recv_cache_[tag]
                logger.debug("Get val with tag '{}' finish.".format(tag))
                return pickle.loads(val)

            if (end - start) > max_time:
                logger.warn(
                    "Can't get value for tag '{}', timeout.".format(tag))
                break

            time.sleep(interval)

        return None


class XGB_GUEST_EN:

    def __init__(
            self,
            proxy_server=None,
            proxy_client_host=None,
            base_score=0.5,
            max_depth=3,
            n_estimators=10,
            learning_rate=0.1,
            reg_lambda=1,
            gamma=0,
            min_child_sample=1,
            # min_child_sample=100,
            min_child_weight=1,
            objective='linear',
            #  channel=None,
            sid=0,
            record=0,
            encrypted=False):
        # self.channel = channel
        self.proxy_server = proxy_server
        self.proxy_client_host = proxy_client_host

        self.base_score = base_score
        self.max_depth = max_depth
        self.n_estimators = n_estimators
        self.learning_rate = learning_rate
        self.reg_lambda = reg_lambda
        self.gamma = gamma
        self.min_child_sample = min_child_sample
        self.min_child_weight = min_child_weight
        self.objective = objective
        self.sid = sid
        self.record = record
        self.lookup_table = {}
        self.lookup_table_sum = {}
        self.pub = None
        self.tree_structure = {}
        self.host_record = 0
        self.guest_record = 0
        self.tree_structure = {}
        self.is_encrypted = encrypted

    # def get_GH(self, X, pub):

    #     bins = 13
    #     items = [x for x in X.columns if x not in ['g', 'h']]
    #     g = X['g']
    #     h = X['h']
    #     actor_nums = 50
    #     generate_add_actors = ActorPool(
    #         [ActorAdd.remote(pub) for _ in range(actor_nums)])

    #     pools = ActorPool([
    #         PallierAdd.remote(pub, actor_nums, generate_add_actors),
    #         PallierAdd.remote(pub, actor_nums, generate_add_actors),
    #         PallierAdd.remote(pub, actor_nums, generate_add_actors),
    #         PallierAdd.remote(pub, actor_nums, generate_add_actors)
    #     ])

    #     maps = [
    #         MapGH.remote(item=tmp_item,
    #                      col=X[tmp_item],
    #                      g=g,
    #                      h=h,
    #                      pub=pub,
    #                      min_child_sample=self.min_child_sample,
    #                      pools=pools) for tmp_item in items
    #     ]

    #     gh_reducer = ReduceGH.remote(maps)
    #     gh_result = ray.get(gh_reducer.reduce_gh.remote(bins=bins))
    #     GH = gh_result

    #     return GH

    def sums_of_encrypted_ghs(self,
                              X_guest,
                              encrypted_ghs,
                              cal_hist=True,
                              bins=13,
                              add_actor_num=50,
                              map_pools=50,
                              limit_add_len=3):
        if cal_hist:
            hist_0 = X_guest.apply(np.histogram, args=(bins,), axis=0)
            cut_points = hist_0.iloc[1]
            #TODO: check whether the length of 'np.unique' is less than 'np.histogram'
        else:
            cut_points = X_guest.apply(np.unique, axis=0)

        # generate add actors with paillier encryption
        paillier_add_actors = ActorPool(
            [ActorAdd.remote(self.pub) for _ in range(add_actor_num)])

        # generate actor pool for mapping
        map_pool = ActorPool([
            PallierAdd.remote(self.pub,
                              map_pools,
                              paillier_add_actors,
                              is_encrypted=self.is_encrypted)
        ])

        sum_maps = [
            MapGH.remote(item=tmp_item,
                         col=X_guest[tmp_item],
                         cut_points=cut_points[tmp_item],
                         g=encrypted_ghs['g'],
                         h=encrypted_ghs['h'],
                         pub=self.pub,
                         min_child_sample=self.min_child_sample,
                         pools=map_pool) for tmp_item in X_guest.columns
        ]
        sum_reducer = ReduceGH.remote(sum_maps)
        sum_result = ray.get(sum_reducer.reduce_gh.remote())

        return sum_result

    def guest_tree_construct(self, X_guest, encrypted_ghs, current_depth):
        m, n = X_guest.shape

        if (self.min_child_sample and
                m < self.min_child_sample) or current_depth > self.max_depth:
            return

        # calculate sums of encrypted 'g' and 'h'
        #TODO: only calculate the right ids and left ids
        encrypte_gh_sums = self.sums_of_encrypted_ghs(X_guest, encrypted_ghs)
        self.proxy_client_host.Remote(encrypte_gh_sums, 'encrypte_gh_sums')
        best_cut = self.proxy_server.Get('best_cut')

        logging.info("current best cut: {}".format(best_cut))

        host_best = best_cut['host_best']
        guest_best = best_cut['guest_best']

        guest_best_gain = None
        host_best_gain = None

        if host_best is not None:
            host_best_gain = host_best['best_gain']

        if guest_best is not None:
            guest_best_gain = guest_best['gain']

        if host_best_gain is None or guest_best_gain is None:
            return None

        guest_flag1 = (host_best_gain and
                       guest_best_gain) and (guest_best_gain > host_best_gain)
        guest_flag2 = (guest_best_gain and not host_best_gain)

        if host_best_gain or guest_best_gain:

            if guest_flag1 or guest_flag2:
                role = "guest"
                record = self.guest_record
                best_var = guest_best['var']
                best_cut = guest_best['cut']
                # calculate the left, right ids and leaf weight
                current_col = X_guest[best_var]
                less_flag = (current_col < best_cut)
                right_flag = (1 - less_flag).astype('bool')

                id_left = X_guest.loc[less_flag].index.tolist()
                id_right = X_guest.loc[right_flag].index.tolist()
                w_left = -guest_best['G_left'] / (guest_best['H_left'] +
                                                  self.reg_lambda)
                w_right = -guest_best['G_right'] / (guest_best['H_right'] +
                                                    self.reg_lambda)

                self.proxy_client_host.Remote(
                    {
                        'id_left': id_left,
                        'id_right': id_right,
                        'w_left': w_left,
                        'w_right': w_right
                    },
                    str(self.guest_record) + '_ids_w')
                # updata guest lookup table
                self.lookup_table[self.guest_record] = [best_var, best_cut]
                print("guest look_up table:", self.lookup_table)

                X_guest_left = X_guest.loc[id_left]
                print("======X_guest_left======", X_guest_left.index)
                X_guest_right = X_guest.loc[id_right]

                encrypted_ghs_left = encrypted_ghs.loc[id_left]
                encrypted_ghs_right = encrypted_ghs.loc[id_right]

                # self.guest_record += 1
                self.guest_record += 1

                self.guest_tree_construct(X_guest_left, encrypted_ghs_left,
                                          current_depth + 1)
                self.guest_tree_construct(X_guest_right, encrypted_ghs_right,
                                          current_depth + 1)

            else:
                ids_w = self.proxy_server.Get(str(self.host_record) + '_ids_w')
                print("guest record: ", self.host_record)
                role = 'host'
                record = self.host_record
                id_left = ids_w['id_left']
                id_right = ids_w['id_right']
                w_left = ids_w['w_left']
                w_right = ids_w['w_right']
                # self.host_record += 1

                print("===train guest==", X_guest.index, ids_w,
                      self.host_record)
                # tree_structure = {(role, record): {}}

                X_guest_left = X_guest.loc[id_left]
                print("======X_guest_left======", X_guest_left.index)
                X_guest_right = X_guest.loc[id_right]

                encrypted_ghs_left = encrypted_ghs.loc[id_left]
                encrypted_ghs_right = encrypted_ghs.loc[id_right]
                self.host_record += 1

                self.guest_tree_construct(X_guest_left, encrypted_ghs_left,
                                          current_depth + 1)
                self.guest_tree_construct(X_guest_right, encrypted_ghs_right,
                                          current_depth + 1)

            # tree_structure[(role,
            #                 record)][('left',
            #                           w_left)] = self.guest_tree_construct(
            #                               X_guest_left, encrypted_ghs_left,
            #                               current_depth + 1)
            # tree_structure[(role,
            #                 record)][('right',
            #                           w_right)] = self.guest_tree_construct(
            #                               X_guest_right, encrypted_ghs_right,
            #                               current_depth + 1)

    def guest_get_tree_ids(self, guest_test, current_lookup):
        while (1):
            role = self.proxy_server.Get('role')
            record_id = self.proxy_server.Get('record_id')
            print("record_id, role, current_lookup", role, record_id,
                  current_lookup)

            # if record_id is None:
            #     break
            if role == "guest":

                if record_id is None:
                    return
                tmp_lookup = current_lookup[record_id]
                var, cut = tmp_lookup[0], tmp_lookup[1]
                guest_test_left = guest_test.loc[guest_test[var] < cut]
                id_left = guest_test_left.index
                guest_test_right = guest_test.loc[guest_test[var] >= cut]
                id_right = guest_test_right.index
                self.proxy_client_host.Remote(
                    {
                        'id_left': id_left,
                        'id_right': id_right
                    },
                    str(record_id) + '_ids')

            else:
                ids = self.proxy_server.Get(str(record_id) + '_ids')
                id_left = ids['id_left']
                print("==predict guest===", guest_test.index, ids)

                guest_test_left = guest_test.loc[id_left]
                id_right = ids['id_right']
                guest_test_right = guest_test.loc[id_right]

            self.guest_get_tree_ids(guest_test_left, current_lookup)
            self.guest_get_tree_ids(guest_test_right, current_lookup)

    def predict(self, X, lookup_sum):
        for t in range(self.n_estimators):
            current_lookup = lookup_sum[t + 1]
            self.guest_get_tree_ids(X, current_lookup)


class XGB_HOST_EN:

    def __init__(
            self,
            proxy_server=None,
            proxy_client_guest=None,
            base_score=0.5,
            max_depth=3,
            n_estimators=10,
            learning_rate=0.1,
            reg_lambda=1,
            gamma=0,
            min_child_sample=1,
            # min_child_sample=100,
            min_child_weight=1,
            objective='linear',
            #  channel=None,
            random_seed=112,
            sid=0,
            record=0,
            encrpyted=True):

        # self.channel = channel
        self.proxy_server = proxy_server
        self.proxy_client_guest = proxy_client_guest
        self.base_score = base_score
        self.max_depth = max_depth
        self.n_estimators = n_estimators
        self.learning_rate = learning_rate
        self.reg_lambda = reg_lambda
        self.gamma = gamma
        self.min_child_sample = min_child_sample
        self.min_child_weight = min_child_weight
        self.objective = objective
        pub, prv = opt_paillier_keygen(random_seed)
        self.pub = pub
        self.prv = prv
        self.sid = sid
        self.record = record
        self.lookup_table = {}
        self.tree_structure = {}
        self.lookup_table_sum = {}
        self.host_record = 0
        self.guest_record = 0
        self.ratio = 10**5
        self.is_encrypted = encrpyted

    def _grad(self, y_hat, Y):

        if self.objective == 'logistic':
            y_hat = 1.0 / (1.0 + np.exp(-y_hat))
            return y_hat - Y
        elif self.objective == 'linear':
            return (y_hat - Y)
        else:
            raise KeyError('objective must be linear or logistic!')

    def _hess(self, y_hat, Y):

        if self.objective == 'logistic':
            y_hat = 1.0 / (1.0 + np.exp(-y_hat))
            return y_hat * (1.0 - y_hat)
        elif self.objective == 'linear':
            return np.array([1] * Y.shape[0])
        else:
            raise KeyError('objective must be linear or logistic!')

    def get_gh(self, y_hat, Y):
        # Calculate the g and h of each sample based on the labels of the local data
        gh = pd.DataFrame(columns=['g', 'h'])
        for i in range(0, Y.shape[0]):
            gh['g'] = self._grad(y_hat, Y)
            gh['h'] = self._hess(y_hat, Y)

        return gh

    def host_best_cut(self, X_host, cal_hist=True, plain_gh=None, bins=13):
        host_colnames = X_host.columns
        # g = plain_gh['g'].values
        g = plain_gh['g'].values
        # h = plain_gh['h'].values
        h = plain_gh['h'].values

        # best_gain = None
        best_gain = 0
        best_cut = None
        best_var = None
        G_left_best, G_right_best, H_left_best, H_right_best = None, None, None, None
        w_left, w_right = None, None

        m, n = X_host.shape
        if self.min_child_sample and m < self.min_child_sample:
            return None

        if cal_hist:
            hist_0 = X_host.apply(np.histogram, args=(bins,), axis=0)
            bin_cut_points = hist_0.iloc[1]
            uni_cut_points = X_host.apply(np.unique, axis=0)
            #TODO: check whether the length of 'np.unique' is less than 'np.histogram'
        else:
            cut_points = X_host.apply(np.unique, axis=0)

        for item in host_colnames:
            #TODO: enhance with parallelization with ray
            tmp_col = X_host[item]

            if cal_hist:
                if len(bin_cut_points[item]) < len(uni_cut_points[item]):
                    tmp_splits = bin_cut_points[item]
                else:
                    tmp_splits = uni_cut_points[item]
            else:
                tmp_splits = cut_points[item]

            if isinstance(tmp_splits, pd.Series):
                tmp_splits = tmp_splits.values
            #try:
            #    tmp_splits = cut_points[item].values
            #except:
            #    tmp_splits = cut_points[item]

            expand_splits = np.tile(tmp_splits, (len(tmp_col), 1)).T
            expand_col = np.tile(tmp_col, (len(tmp_splits), 1))
            less_flag = (expand_col < expand_splits).astype('int')
            larger_flag = 1 - less_flag

            # get the sum of g-s and h-s based on vars
            G_left = np.dot(less_flag, g)
            G_right = np.dot(larger_flag, g)
            H_left = np.dot(less_flag, h)
            H_right = np.dot(larger_flag, h)

            # cal the gain for each var-cut
            gain = G_left**2 / (H_left + self.reg_lambda) + G_right**2 / (
                H_right + self.reg_lambda) - (G_left + G_right)**2 / (
                    H_left + H_right + self.reg_lambda)

            # recorrect the gain
            gain = gain / 2 - self.gamma
            max_gain = max(gain)
            max_index = gain.tolist().index(max_gain)
            tmp_cut = tmp_splits[max_index]
            if best_gain is None or max_gain > best_gain:
                left_inds = (tmp_col < tmp_cut)
                right_inds = (1 - left_inds).astype('bool')
                if self.min_child_sample is not None:
                    if sum(left_inds) < self.min_child_sample or sum(
                            right_inds) < self.min_child_sample:
                        continue

                if self.min_child_weight is not None:
                    if H_left[max_index] < self.min_child_weight or H_right[
                            max_index] < self.min_child_weight:
                        continue

                best_gain = max_gain
                best_cut = tmp_cut
                best_var = item
                G_left_best, G_right_best, H_left_best, H_right_best = G_left[
                    max_index], G_right[max_index], H_left[max_index], H_right[
                        max_index]
        if best_var is not None:
            w_left = -G_left_best / (H_left_best + self.reg_lambda)
            w_right = -G_right_best / (H_right_best + self.reg_lambda)
            return dict({
                'w_left': w_left,
                'w_right': w_right,
                'best_var': best_var,
                'best_cut': best_cut,
                'best_gain': best_gain
            })

        else:
            return None

    def gh_sums_decrypted(self, gh_sums: pd.DataFrame, decryption_pools=50):
        decrypted_items = ['G_left', 'G_right', 'H_left', 'H_right']
        decrypted_gh_sums = gh_sums[decrypted_items]
        if self.is_encrypted:
            m, n = decrypted_gh_sums.shape
            gh_sums_flat = decrypted_gh_sums.values.flatten()

            encryption_pools = ActorPool([
                PaillierActor.remote(self.prv, self.pub)
                for _ in range(decryption_pools)
            ])

            dec_gh_sums_flat = list(
                encryption_pools.map(lambda a, v: a.pai_dec.remote(v),
                                     gh_sums_flat.tolist()))

            dec_gh_sums = np.array(dec_gh_sums_flat).reshape((m, n))
            # dec_gh_sums_df = pd.DataFrame(dec_gh_sums, columns=decrypted_items)
            dec_gh_sums_df = pd.DataFrame(dec_gh_sums,
                                          columns=decrypted_items) / self.ratio
        else:
            dec_gh_sums_df = decrypted_gh_sums

        gh_sums['G_left'] = dec_gh_sums_df['G_left']
        gh_sums['G_right'] = dec_gh_sums_df['G_right']
        gh_sums['H_left'] = dec_gh_sums_df['H_left']
        gh_sums['H_right'] = dec_gh_sums_df['H_right']

        return gh_sums

    def guest_best_cut(self, guest_gh_sums):
        best_gain = 0
        if guest_gh_sums.empty:
            return None
        guest_gh_sums['gain'] = guest_gh_sums['G_left'] ** 2 / (guest_gh_sums['H_left'] + self.reg_lambda) + \
                guest_gh_sums['G_right'] ** 2 / (guest_gh_sums['H_right'] + self.reg_lambda) - \
                (guest_gh_sums['G_left'] + guest_gh_sums['G_right']) ** 2 / (
                guest_gh_sums['H_left'] + guest_gh_sums['H_right'] + + self.reg_lambda)

        guest_gh_sums['gain'] = guest_gh_sums['gain'] / 2 - self.gamma
        print("guest_gh_sums: ", guest_gh_sums)

        max_row = guest_gh_sums['gain'].idxmax()
        max_item = guest_gh_sums.iloc[max_row, :]

        max_gain = max_item['gain']

        if max_gain > best_gain:
            return max_item

        return None

    def host_tree_construct(self,
                            X_host: pd.DataFrame,
                            f_t,
                            current_depth,
                            plain_gh=pd.DataFrame(columns=['g', 'h'])):
        m, n = X_host.shape
        print("current_depth: ", current_depth, m)

        if (self.min_child_sample and
                m < self.min_child_sample) or current_depth > self.max_depth:
            return

        # get the best cut of 'host'
        host_best = self.host_best_cut(X_host,
                                       cal_hist=True,
                                       bins=10,
                                       plain_gh=plain_gh)

        guest_gh_sums = self.proxy_server.Get(
            'encrypte_gh_sums'
        )  # the item contains {'G_left', 'G_right', 'H_left', 'H_right', 'var', 'cut'}

        # decrypted the 'guest_gh_sums' with paillier
        dec_guest_gh_sums = self.gh_sums_decrypted(guest_gh_sums)

        # get the best cut of 'guest'
        guest_best = self.guest_best_cut(dec_guest_gh_sums)
        # guest_best_gain = guest_best['gain']

        self.proxy_client_guest.Remote(
            {
                'host_best': host_best,
                'guest_best': guest_best
            }, 'best_cut')

        logging.info(
            "current depth: {}, host best var: {} and guest best var: {}".
            format(current_depth, host_best, guest_best))
        print("host and guest best", host_best, guest_best)
        host_best_gain = None
        guest_best_gain = None

        if host_best is not None:
            host_best_gain = host_best['best_gain']

        if guest_best is not None:
            guest_best_gain = guest_best['gain']

        if host_best_gain is None and guest_best_gain is None:
            return None

        flag_guest1 = (host_best_gain and
                       guest_best_gain) and (guest_best_gain > host_best_gain)
        flag_guest2 = (not host_best_gain and guest_best_gain)

        # flag_host1 = (host_best_gain and
        #               guest_best_gain) and (guest_best_gain < host_best_gain)
        # flag_host2 = (host_best_gain and not guest_best_gain)

        if (host_best_gain or guest_best_gain):
            if flag_guest1 or flag_guest2:

                role = "guest"
                record = self.guest_record
                ids_w = self.proxy_server.Get(str(self.guest_record) + '_ids_w')

                id_left = ids_w['id_left']
                id_right = ids_w['id_right']
                w_left = ids_w['w_left']
                w_right = ids_w['w_right']
                self.guest_record += 1

            else:
                role = "host"
                record = self.host_record
                # tree_structure = {(self.sid, self.record): {}}
                # self.record += 1

                current_var = host_best['best_var']
                current_col = X_host[current_var]
                less_flag = (current_col < host_best['best_cut'])
                right_flag = (1 - less_flag).astype('bool')

                id_left = X_host.loc[less_flag].index.tolist()
                id_right = X_host.loc[right_flag].index.tolist()

                w_left = host_best['w_left']
                w_right = host_best['w_right']
                ids_w = {
                    'id_left': id_left,
                    'id_right': id_right,
                    'w_left': w_left,
                    'w_right': w_right
                }
                print("===train host==", X_host.index, ids_w, self.host_record)

                self.proxy_client_guest.Remote(ids_w,
                                               str(self.host_record) + '_ids_w')

                self.lookup_table[self.host_record] = [
                    host_best['best_var'], host_best['best_cut']
                ]
                # print("self.lookup_table", self.lookup_table)

                self.host_record += 1

            tree_structure = {(role, record): {}}

            logging.info("current role: {}, current record: {}".format(
                role, record))
            print("role, record: ", role, record)

            X_host_left = X_host.loc[id_left]
            plain_gh_left = plain_gh.loc[id_left]

            X_host_right = X_host.loc[id_right]
            plain_gh_right = plain_gh.loc[id_right]

            f_t[id_left] = w_left
            f_t[id_right] = w_right
            # print("===========", (role, record, w_left, w_right))

            tree_structure[(role, record)][('left',
                                            w_left)] = self.host_tree_construct(
                                                X_host_left, f_t,
                                                current_depth + 1,
                                                plain_gh_left)

            tree_structure[(role,
                            record)][('right',
                                      w_right)] = self.host_tree_construct(
                                          X_host_right, f_t, current_depth + 1,
                                          plain_gh_right)

            return tree_structure

    def host_get_tree_node_weight(self, host_test, tree, current_lookup, w):
        if tree is not None:
            k = list(tree.keys())[0]
            role, record_id = k[0], k[1]
            self.proxy_client_guest.Remote(role, 'role')
            # print("role, record_id", role, record_id, current_lookup)
            self.proxy_client_guest.Remote(record_id, 'record_id')

            if role == 'guest':
                ids = self.proxy_server.Get(str(record_id) + '_ids')
                id_left = ids['id_left']
                id_right = ids['id_right']
                host_test_left = host_test.loc[id_left]
                host_test_right = host_test.loc[id_right]
                id_left = id_left.tolist()
                id_right = id_right.tolist()

            else:
                tmp_lookup = current_lookup
                # var, cut = tmp_lookup['feature_id'], tmp_lookup['threshold_value']
                var, cut = tmp_lookup[record_id][0], tmp_lookup[record_id][1]

                host_test_left = host_test.loc[host_test[var] < cut]
                id_left = host_test_left.index.tolist()
                # id_left = host_test_left.index
                host_test_right = host_test.loc[host_test[var] >= cut]
                id_right = host_test_right.index.tolist()
                # id_right = host_test_right.index
                self.proxy_client_guest.Remote(
                    {
                        'id_left': host_test_left.index,
                        'id_right': host_test_right.index
                    },
                    str(record_id) + '_ids')
                # print("==predict host===", host_test.index, id_left, id_right)

            for kk in tree[k].keys():
                if kk[0] == 'left':
                    tree_left = tree[k][kk]
                    w[id_left] = kk[1]
                elif kk[0] == 'right':
                    tree_right = tree[k][kk]
                    w[id_right] = kk[1]
            print("current w: ", w)
            self.host_get_tree_node_weight(host_test_left, tree_left,
                                           current_lookup, w)
            self.host_get_tree_node_weight(host_test_right, tree_right,
                                           current_lookup, w)

        self.proxy_client_guest.Remote('guest', 'role')
        self.proxy_client_guest.Remote(None, 'record_id')

    def predict_raw(self, X: pd.DataFrame, lookup):
        X = X.reset_index(drop='True')
        # Y = pd.Series([self.base_score] * X.shape[0])
        Y = np.array([self.base_score] * len(X))

        for t in range(self.n_estimators):
            tree = self.tree_structure[t + 1]
            lookup_table = lookup[t + 1]
            # y_t = pd.Series([0] * X.shape[0])
            y_t = np.zeros(len(X))
            print("befor change", y_t)
            #self._get_tree_node_w(X, tree, lookup_table, y_t, t)
            self.host_get_tree_node_weight(X, tree, lookup_table, y_t)
            print("after change", y_t)
            # Y = Y + self.learning_rate * y_t
            Y = Y + self.learning_rate * y_t

        return Y

    def predict_prob(self, X: pd.DataFrame, lookup):

        Y = self.predict_raw(X, lookup)

        # def sigmoid(x):
        #     return 1 / (1 + np.exp(-x))

        # Y = Y.apply(sigmoid)
        Y = 1 / (1 + np.exp(-Y))

        return Y

    def predict(self, X: pd.DataFrame, lookup):
        preds = self.predict_prob(X, lookup)

        return (preds >= 0.5).astype('int')


def get_logger(name):
    LOG_FORMAT = "[%(asctime)s][%(filename)s:%(lineno)d][%(levelname)s] %(message)s"
    DATE_FORMAT = "%m/%d/%Y %H:%M:%S %p"
    logging.basicConfig(level=logging.DEBUG,
                        format=LOG_FORMAT,
                        datefmt=DATE_FORMAT)
    logger = logging.getLogger(name)
    return logger


logger = get_logger("hetero_xgb")

dataset.define("guest_dataset")
dataset.define("label_dataset")

ph.context.Context.func_params_map = {
    "xgb_host_logic": ("paillier",),
    "xgb_guest_logic": ("paillier",)
}

# Number of tree to fit.
num_tree = 1
# the depth of each tree
max_depth = 2
# max_depth = 5
# is_encrypted = True
is_encrypted = False


@ph.context.function(role='host',
                     protocol='xgboost',
                     datasets=['train_hetero_xgb_host', 'test_hetero_xgb_host'],
                     port='8000',
                     task_type="classification")
def xgb_host_logic():
    # def xgb_host_logic(cry_pri="plaintext"):
    start = time.time()
    logger.info("start xgb host logic...")

    role_node_map = ph.context.Context.get_role_node_map()
    node_addr_map = ph.context.Context.get_node_addr_map()
    dataset_map = ph.context.Context.dataset_map

    logger.debug("dataset_map {}".format(dataset_map))

    logger.debug("role_nodeid_map {}".format(role_node_map))

    logger.debug("node_addr_map {}".format(node_addr_map))

    data_key = list(dataset_map.keys())[0]

    eva_type = ph.context.Context.params_map.get("taskType", None)
    if eva_type is None:
        logger.warn(
            "taskType is not specified, set to default value 'regression'.")
        eva_type = "regression"

    eva_type = eva_type.lower()
    if eva_type != "classification" and eva_type != "regression":
        logger.error(
            "Invalid value of taskType, possible value is 'regression', 'classification'."
        )
        return

    logger.info("Current task type is {}.".format(eva_type))

    # 读取注册数据
    # data = ph.dataset.read(dataset_key=data_key).df_data
    data = ph.dataset.read(dataset_key='train_hetero_xgb_host').df_data

    # y = data.pop('Class').values

    print("host data: ", data)

    if len(role_node_map["host"]) != 1:
        logger.error("Current node of host party: {}".format(
            role_node_map["host"]))
        logger.error("In hetero XGB, only dataset of host party has label, "
                     "so host party must have one, make sure it.")
        return

    host_nodes = role_node_map["host"]
    host_port = node_addr_map[host_nodes[0]].split(":")[1]

    guest_nodes = role_node_map["guest"]
    guest_ip, guest_port = node_addr_map[guest_nodes[0]].split(":")

    proxy_server = ServerChannelProxy(host_port)
    proxy_server.StartRecvLoop()

    proxy_client_guest = ClientChannelProxy(guest_ip, guest_port, "guest")

    Y = data.pop('y').values
    X_host = data.copy()

    lookup_table_sum = {}

    # if is_encrypted:
    xgb_host = XGB_HOST_EN(n_estimators=num_tree,
                           max_depth=max_depth,
                           reg_lambda=1,
                           sid=0,
                           min_child_weight=1,
                           objective='logistic',
                           proxy_server=proxy_server,
                           proxy_client_guest=proxy_client_guest,
                           encrpyted=is_encrypted)
    # channel.recv()
    # xgb_host.channel.send(xgb_host.pub)
    proxy_client_guest.Remote(xgb_host.pub, "xgb_pub")
    # proxy_client_guest.Remote(public_k, "xgb_pub")
    # print(xgb_host.channel.recv())
    y_hat = np.array([0.5] * Y.shape[0])
    # ray.init()
    # pai_actor = PaillierActor(xgb_host.prv, xgb_host.pub)
    paillier_encryptor = ActorPool(
        [PaillierActor.remote(xgb_host.prv, xgb_host.pub) for _ in range(10)])
    # actor1, actor2, actor3 = PaillierActor.remote(xgb_host.prv, xgb_host.pub
    #                                       ), PaillierActor.remote(xgb_host.prv, xgb_host.pub
    #                                                               ), PaillierActor.remote(xgb_host.prv, xgb_host.pub
    #                                                               )
    # pools = ActorPool([actor1, actor2, actor3])
    xgb_host.lookup_table = {}

    for t in range(xgb_host.n_estimators):
        print("Begin to trian tree: ", t + 1)
        f_t = pd.Series([0] * Y.shape[0])

        # host cal gradients and hessians with its own label
        gh = xgb_host.get_gh(y_hat, Y)

        # convert gradients and hessians to ints and encrypted with paillier
        # ratio = 10**3
        # gh_large = (gh * ratio).astype('int')

        if is_encrypted:
            # flat_gh = gh.values.flatten().tolist()
            flat_gh = gh.values.flatten()
            #convert ghs to 'ints' and encrypted with paillier
            flat_gh = flat_gh * xgb_host.ratio
            flat_gh.astype('int')

            start_enc = time.time()
            enc_flat_gh = list(
                paillier_encryptor.map(lambda a, v: a.pai_enc.remote(v),
                                       flat_gh))

            end_enc = time.time()

            enc_gh = np.array(enc_flat_gh).reshape((-1, 2))
            enc_gh_df = pd.DataFrame(enc_gh, columns=['g', 'h'])

            # send all encrypted gradients and hessians to 'guest'
            proxy_client_guest.Remote(enc_gh_df, "gh_en")

            end_send_gh = time.time()
            print("Encrypt finish.")

        else:
            proxy_client_guest.Remote(gh, "gh_en")

        # start construct boosting trees
        # lp = LineProfiler(xgb_host.host_tree_construct)
        # lp.run(
        #     "xgb_host.tree_structure[t + 1] = xgb_host.host_tree_construct(X_host.copy(), f_t, 0, gh)"
        # )
        # lp.print_stats()

        xgb_host.tree_structure[t + 1] = xgb_host.host_tree_construct(
            X_host.copy(), f_t, 0, gh)

        end_build_tree = time.time()

        lookup_table_sum[t + 1] = xgb_host.lookup_table
        y_hat = y_hat + xgb_host.learning_rate * f_t

        logger.info("Finish to trian tree {}.".format(t + 1))
        check_time = [
            end_enc - start_enc, end_send_gh - end_enc,
            end_build_tree - end_send_gh
        ]
        print("build time ", check_time)

        end = time.time()
        # logger.info("lasting time for xgb %s".format(end-start))
        print("train encrypted time for xgb: ", (end - start))

        predict_file_path = ph.context.Context.get_predict_file_path()
        indicator_file_path = ph.context.Context.get_indicator_file_path()
        model_file_path = ph.context.Context.get_model_file_path()
        lookup_file_path = ph.context.Context.get_host_lookup_file_path()

        with open(model_file_path, 'wb') as fm:
            pickle.dump(xgb_host.tree_structure, fm)
        with open(lookup_file_path, 'wb') as fl:
            pickle.dump(xgb_host.lookup_table_sum, fl)
        end = time.time()
        # logger.info("lasting time for xgb %s".format(end-start))
        print("train plaintext time for xgb: ", (end - start))

    train_pred = xgb_host.predict(X_host.copy(), lookup_table_sum)
    print("train_pred, Y: ", train_pred, Y)
    train_acc = metrics.accuracy_score(train_pred, Y)
    print("train_acc: ", train_acc)

    # validate for test
    test_host = ph.dataset.read(dataset_key='test_hetero_xgb_host').df_data
    print("test_host: ", test_host.shape)

    test_y = test_host.pop('y')
    test_data = test_host.copy()
    test_pred = xgb_host.predict(test_data, lookup_table_sum)

    test_acc = metrics.accuracy_score(test_pred, test_y.values)
    print("train and test validate acc: ", {
        'train_acc': train_acc,
        'test_acc': test_acc
    })
    proxy_server.StopRecvLoop()
    # host_log.close()


@ph.context.function(
    role='guest',
    protocol='xgboost',
    datasets=['train_hetero_xgb_guest', 'test_hetero_xgb_guest'],
    port='9000',
    task_type="classification")
def xgb_guest_logic():
    # def xgb_guest_logic(cry_pri="plaintext"):
    print("start xgb guest logic...")
    # ios = IOService()
    role_node_map = ph.context.Context.get_role_node_map()
    node_addr_map = ph.context.Context.get_node_addr_map()
    dataset_map = ph.context.Context.dataset_map

    logger.debug("dataset_map {}".format(dataset_map))

    logger.debug("role_nodeid_map {}".format(role_node_map))

    logger.debug("node_addr_map {}".format(node_addr_map))

    data_key = list(dataset_map.keys())[0]

    eva_type = ph.context.Context.params_map.get("taskType", None)
    if eva_type is None:
        logger.warn(
            "taskType is not specified, set to default value 'regression'.")
        eva_type = "regression"

    eva_type = eva_type.lower()
    if eva_type != "classification" and eva_type != "regression":
        logger.error(
            "Invalid value of taskType, possible value is 'regression', 'classification'."
        )
        return

    logger.info("Current task type is {}.".format(eva_type))

    if len(role_node_map["host"]) != 1:
        logger.error("Current node of host party: {}".format(
            role_node_map["host"]))
        logger.error("In hetero XGB, only dataset of host party has label,"
                     "so host party must have one, make sure it.")
        return

    guest_nodes = role_node_map["guest"]
    guest_port = node_addr_map[guest_nodes[0]].split(":")[1]
    proxy_server = ServerChannelProxy(guest_port)
    proxy_server.StartRecvLoop()
    logger.debug("Create server proxy for guest, port {}.".format(guest_port))

    host_nodes = role_node_map["host"]
    host_ip, host_port = node_addr_map[host_nodes[0]].split(":")

    proxy_client_host = ClientChannelProxy(host_ip, host_port, "host")
    # data = ph.dataset.read(dataset_key=data_key).df_data
    data = ph.dataset.read(dataset_key='train_hetero_xgb_guest').df_data
    X_guest = data
    guest_log = open('/app/guest_log', 'w+')

    # if is_encrypted:
    xgb_guest = XGB_GUEST_EN(n_estimators=num_tree,
                             max_depth=max_depth,
                             reg_lambda=1,
                             min_child_weight=1,
                             objective='logistic',
                             sid=1,
                             proxy_server=proxy_server,
                             proxy_client_host=proxy_client_host,
                             encrypted=is_encrypted)  # noqa

    # channel.send(b'guest ready')
    # pub = xgb_guest.channel.recv()
    pub = proxy_server.Get('xgb_pub')
    # xgb_guest.channel.send(b'recved pub')
    lookup_table_sum = {}
    xgb_guest.lookup_table = {}

    for t in range(xgb_guest.n_estimators):
        xgb_guest.record = 0
        # gh_host = xgb_guest.channel.recv()
        gh_en = proxy_server.Get('gh_en')
        xgb_guest.pub = pub
        xgb_guest.tree_structure[t + 1] = xgb_guest.guest_tree_construct(
            X_guest.copy(), gh_en, 0)

        # stat construct boosting trees

        lookup_table_sum[t + 1] = xgb_guest.lookup_table

    lookup_file_path = ph.context.Context.get_guest_lookup_file_path()

    with open(lookup_file_path, 'wb') as fl:
        pickle.dump(xgb_guest.lookup_table_sum, fl)

    xgb_guest.predict(X_guest.copy(), lookup_table_sum)

    # validate for test
    test_guest = ph.dataset.read(dataset_key='test_hetero_xgb_guest').df_data
    print("test_guest: ", test_guest.shape)
    xgb_guest.predict(test_guest, lookup_table_sum)

    # xgb_guest.predict(X_guest)
    proxy_server.StopRecvLoop()
    guest_log.close()
