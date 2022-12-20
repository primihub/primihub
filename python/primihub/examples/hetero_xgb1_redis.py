import primihub as ph
from primihub import dataset, context
from phe import paillier
from sklearn import metrics
from primihub.primitive.opt_paillier_c2py_warpper import *
import pandas as pd
import numpy as np
import redis
import logging
import pickle
from typing import (
    List,
    Optional,
    Union,
    TypeVar,
)
import pandas
import pyarrow
import yaml
from pathlib import Path

from ray.data.block import KeyFn, _validate_key_fn
from primihub.primitive.opt_paillier_c2py_warpper import *
import time
import pandas as pd
import numpy as np
import copy
import logging
import time
from concurrent.futures import ThreadPoolExecutor
from primihub.channel.zmq_channel import IOService, Session
from ray.data._internal.pandas_block import PandasBlockAccessor
from ray.data._internal.util import _check_pyarrow_version
from typing import Callable, Optional
from ray.data.block import BlockAccessor
import functools
import ray
from ray.util import ActorPool
from line_profiler import LineProfiler
from ray.data.aggregate import _AggregateOnKeyBase
from ray.data._internal.null_aggregate import (_null_wrap_init,
                                               _null_wrap_merge,
                                               _null_wrap_accumulate_block,
                                               _null_wrap_finalize)
# import matplotlib.pyplot as plt
T = TypeVar("T", contravariant=True)
U = TypeVar("U", covariant=True)

Block = Union[List[T], "pyarrow.Table", "pandas.DataFrame", bytes]

_pandas = None
from primihub.utils.logger_util import logger


def lazy_import_pandas():
    global _pandas
    if _pandas is None:
        import pandas
        _pandas = pandas
    return _pandas


LOG_FORMAT = "[%(asctime)s][%(filename)s:%(lineno)d][%(levelname)s] %(message)s"
DATE_FORMAT = "%m/%d/%Y %H:%M:%S %p"
logging.basicConfig(level=logging.DEBUG, format=LOG_FORMAT, datefmt=DATE_FORMAT)
logger = logging.getLogger("proxy")


def search_best_splits(X: pd.DataFrame,
                       g,
                       h,
                       hist=True,
                       bins=None,
                       eps=0.001,
                       reg_lambda=1,
                       gamma=0,
                       min_child_sample=None,
                       min_child_weight=None):
    X = X.copy()

    n = len(X)
    if bins is None:
        bins = max(int(np.ceil(np.log(n) / np.log(4))), 2)  # 4 is the base bite

    if isinstance(g, pd.Series):
        g = g.values

    if isinstance(h, pd.Series):
        h = h.values

    best_gain = None
    best_cut = None
    best_var = None
    G_left_best, G_right_best, H_left_best, H_right_best = None, None, None, None
    w_left, w_right = None, None

    m, n = X.shape
    # x = X.values
    vars = X.columns

    if hist:
        if bins is not None:
            hist_0 = X.apply(np.histogram, args=(bins,), axis=0)
        else:
            hist_0 = X.apply(np.histogram, axis=0)
        split_points = hist_0.iloc[1]

        # bin_cut_points = hist_0.iloc[1]
        uni_cut_points = X.apply(np.unique, axis=0)

    else:
        split_points = X.apply(np.unique, axis=0)

    for item in vars:
        tmp_var = item
        if hist:
            try:
                if len(split_points[item].flatten()) < len(
                        uni_cut_points[item].flatten()):

                    tmp_splits = split_points[item]
                else:
                    tmp_splits = uni_cut_points[item]
            except:
                tmp_splits = split_points[item][1:]

        else:
            tmp_splits = split_points[item][1:]

        tmp_col = X[item]
        # print("+++++++++")
        # print(tmp_splits, tmp_splits[0])
        if len(tmp_splits) < 1:
            print("current item ", item, split_points)
            continue
        # print("-------", type(tmp_splits), tmp_splits.shape, split_points)
        tmp_splits[0] = tmp_splits[0] - eps
        tmp_splits[-1] = tmp_splits[-1] + eps

        exp_splits = np.tile(tmp_splits, (len(tmp_col), 1)).T
        exp_col = np.tile(tmp_col, (len(tmp_splits), 1))
        less_flag = (exp_col < exp_splits).astype('int')
        larger_flag = 1 - less_flag
        G_left = np.dot(less_flag, g)
        G_right = np.dot(larger_flag, g)
        H_left = np.dot(less_flag, h)
        H_right = np.dot(larger_flag, h)
        gain = G_left**2 / (H_left + reg_lambda) + G_right**2 / (
            H_right + reg_lambda) - (G_left + G_right)**2 / (H_left + H_right +
                                                             reg_lambda)

        gain = gain / 2 - gamma

        max_gain = max(gain)
        max_index = gain.tolist().index(max_gain)
        tmp_cut = tmp_splits[max_index]
        if best_gain is None or max_gain > best_gain:
            left_inds = (tmp_col < tmp_cut)
            right_inds = (1 - left_inds).astype('bool')
            if min_child_sample is not None:
                if sum(left_inds) < min_child_sample or sum(
                        right_inds) < min_child_sample:
                    continue

            if min_child_weight is not None:
                if H_left[max_index] < min_child_weight or H_right[
                        max_index] < min_child_weight:
                    continue

            best_gain = max_gain
            best_cut = tmp_cut
            best_var = tmp_var
            G_left_best, G_right_best, H_left_best, H_right_best = G_left[
                max_index], G_right[max_index], H_left[max_index], H_right[
                    max_index]

    if best_var is not None:
        w_left = -G_left_best / (H_left_best + reg_lambda)
        w_right = -G_right_best / (H_right_best + reg_lambda)

    return w_left, w_right, best_var, best_cut, best_gain


def opt_paillier_decrypt_crt(pub, prv, cipher_text):

    if not isinstance(cipher_text, Opt_paillier_ciphertext):
        print(f"{cipher_text} should be type of Opt_paillier_ciphertext()")
        return

    decrypt_text = opt_paillier_c2py.opt_paillier_decrypt_crt_warpper(
        pub, prv, cipher_text)

    decrypt_text_num = int(decrypt_text)

    return decrypt_text_num


def atom_paillier_sum(items, pub_key, add_actors, limit=15):
    # less 'limit' will create more parallels
    # nums = items * limit
    if isinstance(items, pd.Series):
        items = items.values
    if len(items) < limit:
        return functools.reduce(lambda x, y: opt_paillier_add(pub_key, x, y),
                                items)
    N = int(len(items) / limit)
    items_list = []
    inter_results = []
    for i in range(N):
        tmp_val = items[i * limit:(i + 1) * limit]
        # tmp_add_actor = self.add_actors[i]
        if i == (N - 1):
            tmp_val = items[i * limit:]
        items_list.append(tmp_val)
    inter_results = list(
        add_actors.map(lambda a, v: a.add.remote(v), items_list))
    #     tmp_g_left, tmp_g_right, tmp_h_left,tmp_h_right  = list(
    #         self.pools.map(lambda a, v: a.pai_add.remote(v), [G_left_g, G_right_g, H_left_h, H_right_h]))
    # # inter_results = [ActorAdd.remote(self.pub, items[i*N:(i+1)*N]).add.remote() for i in range(nums)]
    # final_result = ray.get(inter_results)
    final_result = functools.reduce(
        lambda x, y: opt_paillier_add(pub_key, x, y), inter_results)
    return final_result


class MyPandasBlockAccessor(PandasBlockAccessor):

    def sum(self, on: KeyFn, ignore_nulls: bool, encrypted: bool, pub_key: None,
            add_actors) -> Optional[U]:
        pd = lazy_import_pandas()
        if on is not None and not isinstance(on, str):
            raise ValueError(
                "on must be a string or None when aggregating on Pandas blocks, but "
                f"got: {type(on)}.")
        if self.num_rows() == 0:
            return None
        col = self._table[on]
        print("=====", self._table, col)
        # if col.isnull().all():
        #     # Short-circuit on an all-null column, returning None. This is required for
        #     # sum() since it will otherwise return 0 when summing on an all-null column,
        #     # which is not what we want.
        #     return None
        if not encrypted:
            val = col.sum(skipna=ignore_nulls)
        else:
            val = atom_paillier_sum(col, pub_key, add_actors)
            # tmp_val = {}
            # for tmp_col in on:
            #     tmp_val[tmp_col] = atom_paillier_sum(col[tmp_col], pub_key, add_actors)
            # val = pd.DataFrame(tmp_val)
            # pass
        if pd.isnull(val):
            return None
        return val


class MyBlockAccessor(BlockAccessor):
    """Provides accessor methods for a specific block.

    Ideally, we wouldn't need a separate accessor classes for blocks. However,
    this is needed if we want to support storing ``pyarrow.Table`` directly
    as a top-level Ray object, without a wrapping class (issue #17186).

    There are three types of block accessors: ``SimpleBlockAccessor``, which
    operates over a plain Python list, ``ArrowBlockAccessor`` for
    ``pyarrow.Table`` type blocks, ``PandasBlockAccessor`` for ``pandas.DataFrame``
    type blocks.
    """

    @staticmethod
    def for_block(block: Block) -> "BlockAccessor[T]":
        """Create a block accessor for the given block."""
        _check_pyarrow_version()
        import pandas
        import pyarrow
        # if isinstance(block, pyarrow.Table):
        #     from ray.data._internal.arrow_block import ArrowBlockAccessor
        #     return ArrowBlockAccessor(block)
        if isinstance(block, pandas.DataFrame):
            # from ray.data._internal.pandas_block import PandasBlockAccessor
            return MyPandasBlockAccessor(block)
        # elif isinstance(block, bytes):
        #     from ray.data._internal.arrow_block import ArrowBlockAccessor
        #     return ArrowBlockAccessor.from_bytes(block)
        # elif isinstance(block, list):
        #     from ray.data._internal.simple_block import SimpleBlockAccessor
        #     return SimpleBlockAccessor(block)
        else:
            raise TypeError("Not a block type: {} ({})".format(
                block, type(block)))


class PallierSum(_AggregateOnKeyBase):
    """Define sum of encrypted items."""

    def __init__(self,
                 on: Optional[KeyFn] = None,
                 ignore_nulls: bool = True,
                 pub_key=None,
                 add_actors=None):
        self._set_key_fn(on)
        # null_merge = _null_wrap_merge(ignore_nulls, lambda a1, a2: a1 + a2)
        null_merge = _null_wrap_merge(
            ignore_nulls, lambda a1, a2: opt_paillier_add(pub_key, a1, a2))
        super().__init__(
            init=_null_wrap_init(lambda k: 0),
            merge=null_merge,
            accumulate_block=_null_wrap_accumulate_block(
                ignore_nulls,
                lambda block: MyBlockAccessor.for_block(block).sum(
                    on,
                    ignore_nulls,
                    encrypted=True,
                    pub_key=pub_key,
                    add_actors=add_actors),
                null_merge,
            ),
            finalize=_null_wrap_finalize(lambda a: a),
            name=(f"sum({str(on)})"),
        )


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


# def atom_paillier_sum(items, pub_key, add_actors, limit=3):
#     nums = items * limit
#     if len(items) < nums:
#         return functools.reduce(lambda x, y: opt_paillier_add(pub_key, x, y),
#                                 items)
#     N = int(len(items) / nums)
#     items_list = []

#     inter_results = []
#     for i in range(nums):
#         tmp_val = items[i * N:(i + 1) * N]
#         # tmp_add_actor = self.add_actors[i]
#         if i == (nums - 1):
#             tmp_val = items[i * N:]
#         items_list.append(tmp_val)

#     inter_results = list(
#         add_actors.map(lambda a, v: a.add.remote(v), items_list))

#     #     tmp_g_left, tmp_g_right, tmp_h_left,tmp_h_right  = list(
#     #         self.pools.map(lambda a, v: a.pai_add.remote(v), [G_left_g, G_right_g, H_left_h, H_right_h]))
#     # # inter_results = [ActorAdd.remote(self.pub, items[i*N:(i+1)*N]).add.remote() for i in range(nums)]
#     # final_result = ray.get(inter_results)
#     final_result = functools.reduce(
#         lambda x, y: opt_paillier_add(pub_key, x, y), inter_results)

#     return final_result


@ray.remote
class PallierAdd(object):

    def __init__(self, pub, nums, add_actors, encrypted):
        self.pub = pub
        self.nums = nums
        self.add_actors = add_actors
        self.encrypted = encrypted

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
            # print("not encrypted for add")
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
            # G_right_g = self.g[(1 - flag).astype('bool')].tolist()
            H_left_h = self.h[flag].tolist()
            # H_right_h = self.h[(1 - flag).astype('bool')].tolist()
            # print("++++++++++", len(G_left_g), len(G_right_g), len(H_left_h),
            #       len(H_right_h))

            print("++++++++++", len(G_left_g), len(H_left_h))

            # tmp_g_left, tmp_g_right, tmp_h_left, tmp_h_right = list(
            #     self.pools.map(lambda a, v: a.pai_add.remote(v),
            #                    [G_left_g, G_right_g, H_left_h, H_right_h]))

            tmp_g_left, tmp_h_left = list(
                self.pools.map(lambda a, v: a.pai_add.remote(v),
                               [G_left_g, H_left_h]))
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

            # Add 0s to to 'G_rights'
            G_rights.append(0)

            # G_rights.append(tmp_g_right)
            H_lefts.append(tmp_h_left)
            # H_rights.append(tmp_h_right)

            # Aadd 0s to 'H_rights'
            H_rights.append(0)
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


@ray.remote
class GroupPool:

    def __init__(self, add_actors, pub) -> None:
        self.add_actors = add_actors
        self.pub = pub
        pass

    def groupby(self, group_col):
        tmp_sum = group_col._aggregate_on(PallierSum,
                                          on=['g', 'h'],
                                          ignore_nulls=True,
                                          pub_key=self.pub,
                                          add_actors=self.add_actors)

        return tmp_sum.to_pandas()


@ray.remote
class GroupSum(object):

    def __init__(self, groupdata, pub, add_actors) -> None:
        self.groupdata = groupdata
        self.pub = pub
        self.add_actors = add_actors

    def groupsum(self):
        self.groupsum = self.groupdata._aggregate_on(PallierSum,
                                                     on=['g', 'h'],
                                                     ignore_nulls=True,
                                                     pub_key=self.pub,
                                                     add_actors=self.add_actors)

    def getsum(self):
        return self.groupsum.to_pandas()
        # return groupsum.to_pandas()


class BatchGHSum:

    def __init__(self, split_cuts) -> None:
        self.split_cuts = split_cuts

    def __call__(self, batch: pd.DataFrame):

        pass


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
            is_encrypted=None,
            guest_redis=None):
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
        self.encrypted = is_encrypted
        self.chops = 20
        self.guest_redis = guest_redis

    def sums_of_encrypted_ghs_with_ray(self,
                                       X_guest,
                                       encrypted_ghs,
                                       cal_hist=True,
                                       bins=None,
                                       add_actor_num=20,
                                       map_pools=50,
                                       limit_add_len=3):
        n = len(X_guest)
        if bins is None:
            bins = max(int(np.ceil(np.log(n) / np.log(4))), 2)

        X_guest['g'] = encrypted_ghs['g']
        X_guest['h'] = encrypted_ghs['h']
        cols = X_guest.columns.difference(['g', 'h'])

        set_items = X_guest[cols].apply(np.unique, axis=0)
        X_guest_max0 = X_guest[cols].max(axis=0) + 0.005
        X_guest_min0 = X_guest[cols].min(axis=0)
        X_guest_width = (X_guest_max0 - X_guest_min0) / bins

        set_cols = [col for col in cols if len(set_items[col]) < bins]

        ray_x_guest = ray.data.from_pandas(X_guest)

        def hist_bin_transform(df: pd.DataFrame):

            def assign_bucket(s: pd.Series):

                # check if current 's' in 'set_cols'
                if s.name in set_cols:
                    return s

                # s_max = X_guest_max0[s.name] + 0.005
                s_min = X_guest_min0[s.name]
                s_width = X_guest_width[s.name]
                s_bin = np.ceil((s - s_min) // s_width)

                s_split = s_min + s_bin * s_width

                return round(s_split, 3)

                # return int((s - s_min) // s_width)

            df.loc[:, cols] = df.loc[:, cols].transform(assign_bucket)

            return df

        buckets_x_guest = ray_x_guest.map_batches(hist_bin_transform,
                                                  batch_format="pandas")

        # add 'g' and 'h' to bucket guest
        # buckets_x_guest = buckets_x_guest.add_column(
        #     "g", lambda df: encrypted_ghs['g'])

        # buckets_x_guest = buckets_x_guest.add_column(
        #     "h", lambda df: encrypted_ghs['h'])

        print("current x-guset and buckets_x_guest", X_guest,
              buckets_x_guest.to_pandas(), encrypted_ghs,
              buckets_x_guest.to_pandas().shape, encrypted_ghs.shape)

        total_left_ghs = {}
        # paillier_add_actors = ActorPool(
        #     [ActorAdd.remote(self.pub) for _ in range(add_actor_num)])

        # grouppools = ActorPool([
        #     GroupPool.remote(paillier_add_actors, self.pub) for _ in range(20)
        # ])

        groups = []
        for tmp_col in cols:
            tmp_group = buckets_x_guest.groupby(tmp_col)
            #     groups.append(tmp_group)

            # if self.encrypted:
            #     groupsums = [
            #         GroupSum.remote(tmp_group, self.pub, paillier_add_actors)
            #         for tmp_group in groups
            #     ]

            #     [tmp_groupsum.groupsum.remote() for tmp_groupsum in groupsums]

            #     res = ray.get(
            #         [tmp_groupsum.getsum.remote() for tmp_groupsum in groupsums])

            #     # res = list(grouppools.map(lambda a, v: a.groupby.remote(v), groups))
            # else:
            #     res = [
            #         tmp_group.sum(on=['g', 'h']).to_pandas() for tmp_group in groups
            #     ]

            # for key, val in zip(cols, res):
            #     total_left_ghs[key] = val

            if self.encrypted:
                tmp_sum = tmp_group._aggregate_on(
                    PallierSum,
                    on=['g', 'h'],
                    ignore_nulls=True,
                    pub_key=self.pub,
                    add_actors=self.paillier_add_actors)
            else:
                tmp_sum = tmp_group.sum(on=['g', 'h'])
            # total_left_ghs[tmp_col] = tmp_sum.to_pandas().sort_values(
            #     by=tmp_col, ascending=True)
            total_left_ghs[tmp_col] = tmp_sum.to_pandas()

        print("current total_left_ghs: ", total_left_ghs)

        return total_left_ghs

    def sums_of_encrypted_ghs(self,
                              X_guest,
                              encrypted_ghs,
                              cal_hist=True,
                              bins=None,
                              add_actor_num=50,
                              map_pools=50,
                              limit_add_len=3):
        n = len(X_guest)
        if bins is None:
            bins = max(int(np.ceil(np.log(n) / np.log(4))), 2)
        if cal_hist:
            hist_0 = X_guest.apply(np.histogram, args=(bins,), axis=0)
            hist_points = hist_0.iloc[1]
            #TODO: check whether the length of 'np.unique' is less than 'np.histogram'
        uniq_points = X_guest.apply(np.unique, axis=0)

        def select(hist_points, uniq_points, item):
            if len(hist_points[item]) < len(uniq_points[item]):
                return hist_points[item]

            return uniq_points[item]

        # generate add actors with paillier encryption
        paillier_add_actors = ActorPool(
            [ActorAdd.remote(self.pub) for _ in range(add_actor_num)])

        # generate actor pool for mapping
        map_pool = ActorPool([
            PallierAdd.remote(self.pub, map_pools, paillier_add_actors,
                              self.encrypted)
        ])

        sum_maps = [
            MapGH.remote(item=tmp_item,
                         col=X_guest[tmp_item],
                         cut_points=select(hist_points, uniq_points, tmp_item),
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
        if ray_group:
            encrypte_gh_sums = self.sums_of_encrypted_ghs_with_ray(
                X_guest, encrypted_ghs)
        else:
            encrypte_gh_sums = self.sums_of_encrypted_ghs(
                X_guest, encrypted_ghs)
        # self.proxy_client_host.Remote(encrypte_gh_sums, 'encrypte_gh_sums')
        self.guest_redis.lpush('encrypte_gh_sums', encrypte_gh_sums)
        # best_cut = self.proxy_server.Get('best_cut')
        best_cut = self.guest_redis.rpop('best_cut')

        logging.info("current best cut: {}".format(best_cut))

        host_best = best_cut['host_best']
        guest_best = best_cut['guest_best']

        guest_best_gain = None
        host_best_gain = None

        if host_best is not None:
            host_best_gain = host_best['best_gain']

        if guest_best is not None:
            guest_best_gain = guest_best['gain']

        if host_best_gain is None and guest_best_gain is None:
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

                # self.proxy_client_host.Remote(
                # {
                #     'id_left': id_left,
                #     'id_right': id_right,
                #     'w_left': w_left,
                #     'w_right': w_right
                # }, 'ids_w')
                self.guest_redis.lpush(
                    'ids_w', {
                        'id_left': id_left,
                        'id_right': id_right,
                        'w_left': w_left,
                        'w_right': w_right
                    })
                # updata guest lookup table
                self.lookup_table[self.guest_record] = [best_var, best_cut]
                print("guest look_up table:", self.lookup_table)

                # self.guest_record += 1
                self.guest_record += 1

            else:
                # ids_w = self.proxy_server.Get('ids_w')
                ids_w = self.guest_redis.rpop('ids_w')
                role = 'host'
                record = self.host_record
                id_left = ids_w['id_left']
                id_right = ids_w['id_right']
                w_left = ids_w['w_left']
                w_right = ids_w['w_right']
                self.host_record += 1

            # print("===train==", X_guest.index, ids_w)
            tree_structure = {(role, record): {}}

            X_guest_left = X_guest.loc[id_left]
            X_guest_right = X_guest.loc[id_right]

            encrypted_ghs_left = encrypted_ghs.loc[id_left]
            encrypted_ghs_right = encrypted_ghs.loc[id_right]

            # self.guest_tree_construct(X_guest_left, encrypted_ghs_left,
            #                           current_depth + 1)
            # self.guest_tree_construct(X_guest_right, encrypted_ghs_right,
            #                           current_depth + 1)

            tree_structure[(role,
                            record)][('left',
                                      w_left)] = self.guest_tree_construct(
                                          X_guest_left, encrypted_ghs_left,
                                          current_depth + 1)
            tree_structure[(role,
                            record)][('right',
                                      w_right)] = self.guest_tree_construct(
                                          X_guest_right, encrypted_ghs_right,
                                          current_depth + 1)

            return tree_structure

    def fit(self, X_guest, lookup_table_sum):
        for t in range(self.n_estimators):
            self.record = 0
            # gh_host = xgb_guest.channel.recv()
            # gh_en = self.proxy_server.Get('gh_en')
            gh_en = self.guest_redis.rpop('gh_en')
            self.tree_structure[t + 1] = self.guest_tree_construct(
                X_guest.copy(), gh_en, 0)

            # stat construct boosting trees

            lookup_table_sum[t + 1] = self.lookup_table

    def guest_get_tree_ids(self, guest_test, tree, current_lookup):
        if tree is not None:
            k = list(tree.keys())[0]
            role, record_id = k[0], k[1]
            # role = self.proxy_server.Get('role')
            # record_id = self.proxy_server.Get('record_id')
            # print("record_id, role, current_lookup", role, record_id,
            #       current_lookup)

            # if record_id is None:
            #     break
            if role == "guest":
                # if record_id is None:
                #     return
                tmp_lookup = current_lookup[record_id]
                var, cut = tmp_lookup[0], tmp_lookup[1]
                guest_test_left = guest_test.loc[guest_test[var] < cut]
                id_left = guest_test_left.index
                guest_test_right = guest_test.loc[guest_test[var] >= cut]
                id_right = guest_test_right.index
                self.guest_redis.lpush(
                    str(record_id) + '_ids', {
                        'id_left': id_left,
                        'id_right': id_right
                    })

                # self.proxy_client_host.Remote(
                #     {
                #         'id_left': id_left,
                #         'id_right': id_right
                #     },
                #     str(record_id) + '_ids')

            else:
                ids = self.guest_redis.rpop(str(record_id) + '_ids')
                # ids = self.proxy_server.Get(str(record_id) + '_ids')
                id_left = ids['id_left']
                print("==predict guest===", guest_test.index, ids)

                guest_test_left = guest_test.loc[id_left]
                id_right = ids['id_right']
                guest_test_right = guest_test.loc[id_right]

            for kk in tree[k].keys():
                if kk[0] == 'left':
                    tree_left = tree[k][kk]
                elif kk[0] == 'right':
                    tree_right = tree[k][kk]

            self.guest_get_tree_ids(guest_test_left, tree_left, current_lookup)
            self.guest_get_tree_ids(guest_test_right, tree_right,
                                    current_lookup)

    def predict(self, X, lookup_sum):
        for t in range(self.n_estimators):
            tree = self.tree_structure[t + 1]
            current_lookup = lookup_sum[t + 1]
            self.guest_get_tree_ids(X, tree, current_lookup)


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
            encrypted=None,
            host_redis=None):

        # self.channel = channel
        # self.proxy_server = proxy_server
        # self.proxy_client_guest = proxy_client_guest
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
        self.encrypted = encrypted
        self.global_transfer_time = 0
        self.gloabl_construct_time = 0
        self.host_redis = host_redis

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

    # def map_batch_best(self, ray_data):
    #     pass

    def host_best_cut(self, X_host, cal_hist=True, plain_gh=None, bins=10):
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

        w_left, w_right, best_var, best_cut, best_gain = search_best_splits(
            X_host,
            g,
            h,
            reg_lambda=self.reg_lambda,
            gamma=self.gamma,
            min_child_sample=self.min_child_sample,
            min_child_weight=self.min_child_weight)

        if best_var is not None:
            return dict({
                'w_left': w_left,
                'w_right': w_right,
                'best_var': best_var,
                'best_cut': best_cut,
                'best_gain': best_gain
            })

        else:
            return None

    def gh_sums_decrypted_with_ray(self,
                                   gh_sums_dict,
                                   plain_gh_sums,
                                   decryption_pools=5):
        sum_col = ['sum(g)', 'sum(h)']
        G_lefts = []
        G_rights = []
        H_lefts = []
        H_rights = []
        cuts = []
        vars = []
        if self.encrypted:
            # encryption_pools = ActorPool([
            #     PaillierActor.remote(self.prv, self.pub)
            #     for _ in range(decryption_pools)
            # ])
            for key, val in gh_sums_dict.items():
                m, n = val.shape
                tmp_var = [key] * m
                tmp_cut = val[key].values.tolist()
                for col in sum_col:
                    print("val[col]: ", val[col])
                    val[col] = [
                        opt_paillier_decrypt_crt(self.pub, self.prv, item)
                        for item in val[col]
                    ]
                val = val.sort_values(by=key, ascending=True)

                cumsum_val = val.cumsum() / self.ratio
                tmp_g_lefts = cumsum_val['sum(g)']
                tmp_h_lefts = cumsum_val['sum(h)']

                G_lefts += tmp_g_lefts.values.tolist()
                H_lefts += tmp_h_lefts.values.tolist()
                vars += tmp_var
                cuts += tmp_cut

                # encrypted_sums = val[sum_col]
                # m, n = encrypted_sums.shape
                # encrypted_sums_flat = encrypted_sums.values.flatten()
        else:
            for key, val in gh_sums_dict.items():
                m, n = val.shape

                # sorted by col
                val = val.sort_values(by=key, ascending=True)
                tmp_var = [key] * m
                tmp_cut = val[key].values.tolist()
                # for col in sum_col:
                #     val[col] = [
                #         opt_paillier_decrypt_crt(self.pub, self.prv, item)
                #         for item in val[col]
                #     ]

                cumsum_val = val.cumsum()
                tmp_g_lefts = cumsum_val['sum(g)']
                tmp_h_lefts = cumsum_val['sum(h)']

                G_lefts += tmp_g_lefts.values.tolist()
                H_lefts += tmp_h_lefts.values.tolist()
                vars += tmp_var
                cuts += tmp_cut

        gh_sums = pd.DataFrame({
            'G_left': G_lefts,
            'H_left': H_lefts,
            'var': vars,
            'cut': cuts
        })
        print("current gh_sums and plain_gh_sums: ", gh_sums, plain_gh_sums)

        gh_sums['G_right'] = plain_gh_sums['g'] - gh_sums['G_left']
        gh_sums['H_right'] = plain_gh_sums['h'] - gh_sums['H_left']

        return gh_sums

    def gh_sums_decrypted(self,
                          gh_sums: pd.DataFrame,
                          decryption_pools=50,
                          plain_gh_sums=None):
        # decrypted_items = ['G_left', 'G_right', 'H_left', 'H_right']

        # just decrypt 'G_left' and 'H_left'
        decrypted_items = ['G_left', 'H_left']
        if self.encrypted:
            decrypted_gh_sums = gh_sums[decrypted_items]
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
            dec_gh_sums_df = gh_sums[decrypted_items]

        gh_sums['G_left'] = dec_gh_sums_df['G_left']
        # gh_sums['G_right'] = dec_gh_sums_df['G_right']
        gh_sums['G_right'] = plain_gh_sums['g'] - dec_gh_sums_df['G_left']
        gh_sums['H_left'] = dec_gh_sums_df['H_left']
        # gh_sums['H_right'] = dec_gh_sums_df['H_right']
        gh_sums['H_right'] = plain_gh_sums['h'] - dec_gh_sums_df['H_left']

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
        print("max_row: ", max_row)
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

        plain_gh_sums = plain_gh.sum(axis=0)

        # guest_gh_sums = self.proxy_server.Get(
        #     'encrypte_gh_sums'
        # )  # the item contains {'G_left', 'G_right', 'H_left', 'H_right', 'var', 'cut'}
        guest_gh_sums = self.host_redis.rpop('encrypte_gh_sums')

        # decrypted the 'guest_gh_sums' with paillier
        if ray_group:
            dec_guest_gh_sums = self.gh_sums_decrypted_with_ray(
                guest_gh_sums, plain_gh_sums=plain_gh_sums)
        else:
            dec_guest_gh_sums = self.gh_sums_decrypted(
                guest_gh_sums, plain_gh_sums=plain_gh_sums)

        # get the best cut of 'guest'
        guest_best = self.guest_best_cut(dec_guest_gh_sums)
        # guest_best_gain = guest_best['gain']
        self.host_redis.lpush('best_cut', {
            'host_best': host_best,
            'guest_best': guest_best
        })

        # self.proxy_client_guest.Remote(
        #     {
        #         'host_best': host_best,
        #         'guest_best': guest_best
        #     }, 'best_cut')

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
                # ids_w = self.proxy_server.Get('ids_w')
                ids_w = self.host_redis.rpop('ids_w')

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
                self.host_redis.lpush(
                    'ids_w', {
                        'id_left': id_left,
                        'id_right': id_right,
                        'w_left': w_left,
                        'w_right': w_right
                    })
                # self.proxy_client_guest.Remote(
                #     {
                #         'id_left': id_left,
                #         'id_right': id_right,
                #         'w_left': w_left,
                #         'w_right': w_right
                #     }, 'ids_w')

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
            # self.proxy_client_guest.Remote(role, 'role')
            # # print("role, record_id", role, record_id, current_lookup)
            # self.proxy_client_guest.Remote(record_id, 'record_id')

            if role == 'guest':
                # ids = self.proxy_server.Get(str(record_id) + '_ids')
                ids = self.host_redis.rpop(str(record_id) + '_ids')
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
                self.host_redis.lpush(
                    str(record_id) + '_ids', {
                        'id_left': host_test_left.index,
                        'id_right': host_test_right.index
                    })
                # self.proxy_client_guest.Remote(
                #     {
                #         'id_left': host_test_left.index,
                #         'id_right': host_test_right.index
                #     },
                #     str(record_id) + '_ids')
                # print("==predict host===", host_test.index, id_left, id_right)

            for kk in tree[k].keys():
                if kk[0] == 'left':
                    tree_left = tree[k][kk]
                    w[id_left] = kk[1]
                elif kk[0] == 'right':
                    tree_right = tree[k][kk]
                    w[id_right] = kk[1]
            # print("current w: ", w)
            self.host_get_tree_node_weight(host_test_left, tree_left,
                                           current_lookup, w)
            self.host_get_tree_node_weight(host_test_right, tree_right,
                                           current_lookup, w)

        # self.proxy_client_guest.Remote('guest', 'role')
        # self.proxy_client_guest.Remote(None, 'record_id')

    def fit(self, X_host, Y, paillier_encryptor, lookup_table_sum):
        y_hat = np.array([self.base_score] * len(Y))
        train_losses = []

        start = time.time()
        for t in range(self.n_estimators):
            print("Begin to trian tree: ", t + 1)
            f_t = pd.Series([0] * Y.shape[0])

            # host cal gradients and hessians with its own label
            gh = pd.DataFrame({
                'g': self._grad(y_hat, Y.flatten()),
                'h': self._hess(y_hat, Y.flatten())
            })

            # convert gradients and hessians to ints and encrypted with paillier
            # ratio = 10**3
            # gh_large = (gh * ratio).astype('int')
            if self.encrypted:
                flat_gh = gh.values.flatten()
                flat_gh *= self.ratio

                flat_gh = flat_gh.astype('int')

                start_enc = time.time()
                enc_flat_gh = list(
                    paillier_encryptor.map(lambda a, v: a.pai_enc.remote(v),
                                           flat_gh.tolist()))

                end_enc = time.time()

                enc_gh = np.array(enc_flat_gh).reshape((-1, 2))
                enc_gh_df = pd.DataFrame(enc_gh, columns=['g', 'h'])

                # send all encrypted gradients and hessians to 'guest'
                # self.proxy_client_guest.Remote(enc_gh_df, "gh_en")
                self.host_redis.lpush('gh_en', enc_gh_df)

                end_send_gh = time.time()
                print("Time for encryption and transfer: ",
                      (end_enc - start_enc), (end_send_gh - end_enc))
                print("Encrypt finish.")

            else:
                # self.proxy_client_guest.Remote(gh, "gh_en")
                self.host_redis.lpush('gh_en', gh)

            # start construct boosting trees
            # lp = LineProfiler(xgb_host.host_tree_construct)
            # lp.run(
            #     "xgb_host.tree_structure[t + 1] = xgb_host.host_tree_construct(X_host.copy(), f_t, 0, gh)"
            # )
            # lp.print_stats()

            self.tree_structure[t + 1] = self.host_tree_construct(
                X_host.copy(), f_t, 0, gh)
            # y_hat = y_hat + xgb_host.learning_rate * f_t

            end_build_tree = time.time()

            lookup_table_sum[t + 1] = self.lookup_table
            y_hat = y_hat + self.learning_rate * f_t

            logger.info("Finish to trian tree {}.".format(t + 1))

            current_loss = self.log_loss(Y, 1 / (1 + np.exp(-y_hat)))
            train_losses.append(current_loss)

            print("train_losses ", train_losses)

    def predict_raw(self, X: pd.DataFrame, lookup):
        X = X.reset_index(drop='True')
        # Y = pd.Series([self.base_score] * X.shape[0])
        Y = np.array([self.base_score] * len(X))

        for t in range(self.n_estimators):
            tree = self.tree_structure[t + 1]
            lookup_table = lookup[t + 1]
            # y_t = pd.Series([0] * X.shape[0])
            y_t = np.zeros(len(X))
            #self._get_tree_node_w(X, tree, lookup_table, y_t, t)
            self.host_get_tree_node_weight(X, tree, lookup_table, y_t)
            Y = Y + self.learning_rate * y_t

        return Y

    def predict_prob(self, X: pd.DataFrame, lookup):

        Y = self.predict_raw(X, lookup)

        Y = 1 / (1 + np.exp(-Y))

        return Y

    def predict(self, X: pd.DataFrame, lookup):
        preds = self.predict_prob(X, lookup)

        return (preds >= 0.5).astype('int')

    def log_loss(self, actual, predict_prob):

        return metrics.log_loss(actual, predict_prob)


def get_logger(name):
    LOG_FORMAT = "[%(asctime)s][%(filename)s:%(lineno)d][%(levelname)s] %(message)s"
    DATE_FORMAT = "%m/%d/%Y %H:%M:%S %p"
    logging.basicConfig(level=logging.DEBUG,
                        format=LOG_FORMAT,
                        datefmt=DATE_FORMAT)
    logger = logging.getLogger(name)
    return logger


logger = get_logger("hetero_xgb")

ph.context.Context.func_params_map = {
    "xgb_host_logic": ("paillier",),
    "xgb_guest_logic": ("paillier",)
}

# Number of tree to fit.
num_tree = 5
# the depth of each tree
max_depth = 5
# whether encrypted or not
is_encrypted = True

ray_group = True

min_child_weight = 5


class RedisProxy:

    def __init__(self,
                 host,
                 port,
                 db=0,
                 password='primihub',
                 topic='hetero_xgb') -> None:
        # self.host = host
        # self.port = port
        # self.db = db
        # self.password = password

        self.connection = redis.Redis(host=host,
                                      port=port,
                                      db=db,
                                      password=password)

    def set(self, key, val):
        # flag = False
        # while not flag:
        try:
            self.connection.set(key, pickle.dumps(val))
        except Exception as e:
            raise KeyError("Redis set exception is ", str(e))

        logger.info("Redis set successful")

    def get(self, key, max_time=300, interval=0.1):
        start = time.time()
        res = None
        while True:
            try:
                res = pickle.loads(self.connection.get(key))
            except TypeError as e:
                logger.error("Current key error {}".format(e))
            end = time.time()

            if res is not None:
                self.connection.delete(key)
                return res

            if (end - start) > max_time:
                raise KeyError(
                    "Can't get value for key {}, timeout.".format(key))

            time.sleep(interval)

    def lpush(self, key, val):
        try:
            flag = self.connection.lpush(key, pickle.dumps(val))
        except Exception as e:
            logger.error("Redis set exception is ", str(e))

        logger.info("Redis lpush successful")

    def rpop(self, key, max_time=10000, interval=0.1):
        start = time.time()
        res = None

        while True:
            try:
                res = pickle.loads(self.connection.rpop(key))
            except TypeError as e:
                logger.error("Current key error {}".format(e))
            end = time.time()

            if res is not None:
                # self.redis.delete(key)
                return res

            if (end - start) > max_time:
                raise KeyError(
                    "Can't get value for key {}, timeout.".format(key))

            time.sleep(interval)

    def delete(self, key):
        try:
            flag = self.connection.delete(key)
        except Exception as e:
            logger.error("Redis set exception is ", str(e))

    def stop(self):
        try:
            self.connection.shutdown()
        except Exception as e:
            logger.error("Redis set exception is ", str(e))


yaml_file = "/app/config/primihub_node0.yaml"
yaml_dict = yaml.safe_load(Path(yaml_file).read_text())
redis_meta = yaml_dict['redis_meta_service']

redis_password = redis_meta['redis_password']
redis_addr = redis_meta['redis_addr']
redis_ip = redis_addr.split(":")[0]
redis_port = int(redis_addr.split(":")[1])


@ph.context.function(
    role='host',
    protocol='xgboost',
    datasets=['train_hetero_xgb_host'
             ],  # ['train_hetero_xgb_host'],  #, 'test_hetero_xgb_host'],
    port='8000',
    task_type="classification")
def xgb_host_logic(cry_pri="paillier"):
    logger.info("start xgb host logic...")

    role_node_map = ph.context.Context.get_role_node_map()
    node_addr_map = ph.context.Context.get_node_addr_map()
    dataset_map = ph.context.Context.dataset_map

    logger.debug("dataset_map {}".format(dataset_map))

    logger.debug("role_nodeid_map {}".format(role_node_map))

    logger.debug("node_addr_map {}".format(node_addr_map))

    data_key = list(dataset_map.keys())[0]

    # host_redis = RedisProxy(host='172.21.3.108',
    #                         port=15550,
    #                         db=0,
    #                         password='primihub')
    host_redis = RedisProxy(host=redis_ip,
                            port=redis_port,
                            db=0,
                            password=redis_password)

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
    data = ph.dataset.read(dataset_key=data_key).df_data
    # data = ph.dataset.read(dataset_key='train_hetero_xgb_host').df_data
    # data = pd.read_csv(
    #     '/primihub/data/FL/hetero_xgb/train/epsilon_normalized.t.host',
    #     header=0)
    # data = data.iloc[:, 550:]

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

    # proxy_server = ServerChannelProxy(host_port)
    # proxy_server.StartRecvLoop()

    # proxy_client_guest = ClientChannelProxy(guest_ip, guest_port, "guest")

    Y = data.pop('y').values
    X_host = data.copy()

    lookup_table_sum = {}

    # if is_encrypted:
    xgb_host = XGB_HOST_EN(
        n_estimators=num_tree,
        max_depth=max_depth,
        reg_lambda=1,
        sid=0,
        min_child_weight=min_child_weight,
        objective='logistic',
        #    proxy_server=proxy_server,
        #    proxy_client_guest=proxy_client_guest,
        encrypted=is_encrypted,
        host_redis=host_redis)
    # channel.recv()
    # xgb_host.channel.send(xgb_host.pub)
    # proxy_client_guest.Remote(xgb_host.pub, "xgb_pub")
    # host_redis.set("xgb_pub", pickle.dumps(xgb_host.pub))
    host_redis.set("PUB", xgb_host.pub)
    # proxy_client_guest.Remote(public_k, "xgb_pub")
    # print(xgb_host.channel.recv())
    # y_hat = np.array([0.5] * Y.shape[0])
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
    y_hat = np.array([xgb_host.base_score] * len(Y))
    train_losses = []

    start = time.time()
    # xgb_host.fit(X_host, Y, paillier_encryptor, lookup_table_sum)

    lp = LineProfiler()
    lp.add_function(xgb_host.host_tree_construct)
    lp.add_function(xgb_host.gh_sums_decrypted)
    lp_wrapper = lp(xgb_host.fit)
    lp_wrapper(X_host=X_host,
               Y=Y,
               paillier_encryptor=paillier_encryptor,
               lookup_table_sum=lookup_table_sum)
    lp.print_stats()

    end = time.time()
    # logger.info("lasting time for xgb %s".format(end-start))
    print("train time for xgboost: ", (end - start))

    model_file_path = ph.context.Context.get_model_file_path()
    lookup_file_path = ph.context.Context.get_host_lookup_file_path()

    # print("host structure: ", xgb_host.tree_structure)

    # train_pred = xgb_host.predict(X_host.copy(), lookup_table_sum)
    # train_acc = metrics.accuracy_score(train_pred, Y)
    # print("train_acc: ", train_acc)

    # save host-part model
    model_file_path = ph.context.Context.get_model_file_path() + ".host"

    with open(model_file_path, 'wb') as hostModel:
        pickle.dump(
            {
                'tree_struct': xgb_host.tree_structure,
                'lr': xgb_host.learning_rate
            }, hostModel)

    # save host-part table
    lookup_file_path = ph.context.Context.get_host_lookup_file_path() + ".host"
    with open(lookup_file_path, 'wb') as hostTable:
        pickle.dump(lookup_table_sum, hostTable)

    predict_file_path = ph.context.Context.get_predict_file_path()
    indicator_file_path = ph.context.Context.get_indicator_file_path() + '.png'

    # save results to png
    # current_pred = xgb_host.predict_prob(X_host.copy(), lookup_table_sum)
    # plt.figure()
    # fpr, tpr, threshold = metrics.roc_curve(Y, current_pred)
    # plt.plot(fpr, tpr)
    # plt.title("train_acc={}".format(train_acc))
    # plt.savefig(indicator_file_path)

    # save pred_y to file
    # preds = pd.DataFrame({'prob': current_pred, "binary_pred": train_pred})
    # preds.to_csv(predict_file_path, index=False, sep='\t')
    # with open(indicator_file_path, 'wb') as trainMetrics:
    #     pickle.dump(train_metrics, trainMetrics)

    # proxy_server.StopRecvLoop()
    # host_log.close()


@ph.context.function(
    role='guest',
    protocol='xgboost',
    datasets=[
        'train_hetero_xgb_guest'  #'five_thous_guest'
    ],  #['train_hetero_xgb_guest'],  #, 'test_hetero_xgb_guest'],
    port='9000',
    task_type="classification")
def xgb_guest_logic(cry_pri="paillier"):
    # def xgb_guest_logic(cry_pri="plaintext"):
    print("start xgb guest logic...")
    # ios = IOService()
    role_node_map = ph.context.Context.get_role_node_map()
    node_addr_map = ph.context.Context.get_node_addr_map()
    dataset_map = ph.context.Context.dataset_map

    # guest_redis = RedisProxy(host='172.21.3.108',
    #                          port=15550,
    #                          db=0,
    #                          password='primihub')
    guest_redis = RedisProxy(host=redis_ip,
                             port=redis_port,
                             db=0,
                             password=redis_password)

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
    # proxy_server = ServerChannelProxy(guest_port)
    # proxy_server.StartRecvLoop()
    logger.debug("Create server proxy for guest, port {}.".format(guest_port))

    host_nodes = role_node_map["host"]
    host_ip, host_port = node_addr_map[host_nodes[0]].split(":")

    # proxy_client_host = ClientChannelProxy(host_ip, host_port, "host")
    data = ph.dataset.read(dataset_key=data_key).df_data
    # data = ph.dataset.read(dataset_key='train_hetero_xgb_guest').df_data
    # data = pd.read_csv(
    #     '/primihub/data/FL/hetero_xgb/train/epsilon_normalized.t.guest',
    #     header=0)
    # data = data.iloc[:, :10]

    X_guest = data
    # guest_log = open('/app/guest_log', 'w+')
    add_actor_num = 50
    # if is_encrypted:
    xgb_guest = XGB_GUEST_EN(
        n_estimators=num_tree,
        max_depth=max_depth,
        reg_lambda=1,
        min_child_weight=min_child_weight,
        objective='logistic',
        sid=1,
        #  proxy_server=proxy_server,
        #  proxy_client_host=proxy_client_host,
        is_encrypted=is_encrypted,
        guest_redis=guest_redis)  # noqa

    # channel.send(b'guest ready')
    # pub = xgb_guest.channel.recv()
    # pub = proxy_server.Get('xgb_pub')
    # pub = pickle.loads(guest_redis.get('xgb_pub'))
    # pub = guest_redis.get('xgb_pub2')
    pub = guest_redis.get('PUB')
    guest_redis.delete('PUB')
    # pub = guesproxy_server.Get('xgb_pub')

    xgb_guest.pub = pub
    paillier_add_actors = ActorPool(
        [ActorAdd.remote(xgb_guest.pub) for _ in range(add_actor_num)])

    xgb_guest.paillier_add_actors = paillier_add_actors

    # xgb_guest.channel.send(b'recved pub')
    lookup_table_sum = {}
    xgb_guest.lookup_table = {}

    lp = LineProfiler()
    lp.add_function(xgb_guest.guest_tree_construct)
    lp.add_function(xgb_guest.sums_of_encrypted_ghs)

    lp_wrapper = lp(xgb_guest.fit)
    lp_wrapper(X_guest.copy(), lookup_table_sum)
    lp.print_stats()

    # xgb_guest.fit(X_guest, lookup_table_sum)

    # for t in range(xgb_guest.n_estimators):
    #     xgb_guest.record = 0
    #     # gh_host = xgb_guest.channel.recv()
    #     gh_en = proxy_server.Get('gh_en')
    #     xgb_guest.tree_structure[t + 1] = xgb_guest.guest_tree_construct(
    #         X_guest.copy(), gh_en, 0)

    #     # stat construct boosting trees

    #     lookup_table_sum[t + 1] = xgb_guest.lookup_table
    # xgb_guest.predict(X_guest.copy(), lookup_table_sum)

    # predict_file_path = ph.context.Context.get_predict_file_path()
    # indicator_file_path = ph.context.Context.get_indicator_file_path()
    # guest_model_path = ph.context.Context.get_guest_model_path()
    lookup_file_path = ph.context.Context.get_guest_lookup_file_path(
    ) + ".guest"
    guest_model_path = ph.context.Context.get_model_file_path() + ".guest"

    # save guest part model
    with open(guest_model_path, 'wb') as guestModel:
        pickle.dump(
            {
                'tree_struct': xgb_guest.tree_structure,
                'lr': xgb_guest.learning_rate
            }, guestModel)

    # save guest part table
    with open(lookup_file_path, 'wb') as guestTable:
        pickle.dump(lookup_table_sum, guestTable)

    # xgb_guest.predict(X_guest.copy(), lookup_table_sum)

    # validate for test
    # test_guest = ph.dataset.read(dataset_key='test_hetero_xgb_guest').df_data
    # print("test_guest: ", test_guest.shape)
    # xgb_guest.predict(test_guest, lookup_table_sum)
    # xgb_guest.predict(X_guest.copy(), lookup_table_sum)

    # xgb_guest.predict(X_guest)
    # proxy_server.StopRecvLoop()
    # guest_log.close()
