import primihub as ph
from primihub import dataset, context
from phe import paillier
from sklearn import metrics
from primihub.primitive.opt_paillier_c2py_warpper import *
import pandas as pd
import numpy as np
import random
from scipy.stats import ks_2samp
from sklearn.metrics import roc_auc_score
import pickle
import json
from typing import (
    List,
    Optional,
    Union,
    TypeVar,
)
from multiprocessing import Process, Pool
import pandas
import pyarrow

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

from primihub.utils.logger_util import FLFileHandler, FLConsoleHandler, FORMAT

# import matplotlib.pyplot as plt
T = TypeVar("T", contravariant=True)
U = TypeVar("U", covariant=True)

Block = Union[List[T], "pyarrow.Table", "pandas.DataFrame", bytes]

_pandas = None


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

# ray.init(address='ray://172.21.3.108:10001')
console_handler = FLConsoleHandler(jb_id=1,
                                   task_id=1,
                                   log_level='info',
                                   format=FORMAT)
fl_console_log = console_handler.set_format()

file_handler = FLFileHandler(jb_id=1,
                             task_id=1,
                             log_file='fl_log_info.txt',
                             log_level='INFO',
                             format=FORMAT)
fl_file_log = file_handler.set_format()

# ray.init("ray://172.21.3.108:10001")


def goss_sample(df_g, top_rate=0.2, other_rate=0.2):
    df_g_cp = abs(df_g.copy())
    g_arr = df_g_cp['g'].values
    if top_rate < 0 or top_rate > 100:
        raise ValueError("The ratio should be between 0 and 100.")
    elif top_rate > 0 and top_rate < 1:
        top_rate *= 100

    top_rate = int(top_rate)
    top_clip = np.percentile(g_arr, 100 - top_rate)
    top_ids = df_g_cp[df_g_cp['g'] >= top_clip].index.tolist()
    other_ids = df_g_cp[df_g_cp['g'] < top_clip].index.tolist()

    assert other_rate > 0 and other_rate <= 1.0
    other_num = int(len(other_ids) * other_rate)

    low_ids = random.sample(other_ids, other_num)

    return top_ids, low_ids


def random_sample(df_g, top_rate=0.2, other_rate=0.2):
    all_ids = df_g.index.tolist()
    sample_rate = top_rate + other_rate

    assert sample_rate > 0 and sample_rate <= 1.0
    sample_num = int(len(all_ids) * sample_rate)

    sample_ids = random.sample(all_ids, sample_num)

    return sample_ids


def col_sample(feature_list, sample_ratio=0.3, threshold=30):
    if len(feature_list) < threshold:
        return feature_list

    sample_num = int(len(feature_list) * sample_ratio)

    sample_features = random.sample(feature_list, sample_num)

    return sample_features


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

        if len(tmp_splits) < 1:
            fl_console_log.info(
                "current item is {} and split_point is {}".format(
                    item, split_points))
            # print("current item ", item, split_points)
            continue

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
        fl_console_log.error(
            "The input should be type of Opt_paillier_ciphertext but {}".format(
                type(cipher_text)))
        # print(f"{cipher_text} should be type of Opt_paillier_ciphertext()")
        return

    decrypt_text = opt_paillier_c2py.opt_paillier_decrypt_crt_warpper(
        pub, prv, cipher_text)

    decrypt_text_num = int(decrypt_text)

    return decrypt_text_num


@ray.remote
def map(obj, f):
    return f(obj)


@ray.remote
def batch_paillier_sum(items, pub_key, limit_size=50):
    if isinstance(items, pd.Series):
        items = items.values

    n = len(items)

    if n < limit_size:
        return functools.reduce(lambda x, y: opt_paillier_add(pub_key, x, y),
                                items)

    mid = int(n / 2)
    left = items[:mid]
    right = items[mid:]

    left_sum = batch_paillier_sum.remote(left, pub_key)
    right_sum = batch_paillier_sum.remote(right, pub_key)
    return opt_paillier_add(pub_key, ray.get(left_sum), ray.get(right_sum))


def atom_paillier_sum(items, pub_key, add_actors, limit=15):
    # less 'limit' will create more parallels
    # nums = items * limit
    if isinstance(items, pd.Series):
        items = items.values
    if len(items) < limit:
        return functools.reduce(lambda x, y: opt_paillier_add(pub_key, x, y),
                                items)

    items_list = [items[x:x + limit] for x in range(0, len(items), limit)]
    # N = int(len(items) / limit)
    # items_list = []
    # inter_results = []
    # for i in range(N):
    #     tmp_val = items[i * limit:(i + 1) * limit]
    #     # tmp_add_actor = self.add_actors[i]
    #     if i == (N - 1):
    #         tmp_val = items[i * limit:]
    #     items_list.append(tmp_val)
    inter_results = list(
        add_actors.map(lambda a, v: a.add.remote(v), items_list))

    final_result = functools.reduce(
        lambda x, y: opt_paillier_add(pub_key, x, y), inter_results)
    return final_result


class MyPandasBlockAccessor(PandasBlockAccessor):

    def sum(self,
            on: KeyFn,
            ignore_nulls: bool,
            encrypted: bool,
            pub_key: None,
            add_actors,
            limit=30) -> Optional[U]:
        pd = lazy_import_pandas()
        if on is not None and not isinstance(on, str):
            raise ValueError(
                "on must be a string or None when aggregating on Pandas blocks, but "
                f"got: {type(on)}.")
        if self.num_rows() == 0:
            return None
        col = self._table[on]
        # print("=====", self._table, col)
        # if col.isnull().all():
        #     # Short-circuit on an all-null column, returning None. This is required for
        #     # sum() since it will otherwise return 0 when summing on an all-null column,
        #     # which is not what we want.
        #     return None
        if not encrypted:
            val = col.sum(skipna=ignore_nulls)
        else:
            val = atom_paillier_sum(col, pub_key, add_actors, limit=limit)
            # val = ray.get(batch_paillier_sum.remote(col, pub_key))
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

            # print("++++++++++", len(G_left_g), len(H_left_h))

            tmp_g_left, tmp_h_left = list(
                self.pools.map(lambda a, v: a.pai_add.remote(v),
                               [G_left_g, H_left_h]))

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
def groupby_sum(group_col, pub, on_cols, add_actors):
    df_list = []
    for tmp_col in group_col:
        tmp_sum = tmp_col._aggregate_on(
            PallierSum,
            on=on_cols,
            #   on=['g', 'h'],
            ignore_nulls=True,
            pub_key=pub,
            add_actors=add_actors).to_pandas()

        tmp_count = tmp_col.count().to_pandas()
        tmp_df = pd.merge(tmp_sum, tmp_count)
        df_list.append(tmp_df)

    return df_list


@ray.remote
class GroupPool:

    def __init__(self, add_actors, pub, on_cols) -> None:
        self.add_actors = add_actors
        self.pub = pub
        self.on_cols = on_cols

    def groupby(self, group_col):
        df_list = []
        for tmp_col in group_col:
            tmp_sum = tmp_col._aggregate_on(
                PallierSum,
                on=self.on_cols,
                #   on=['g', 'h'],
                ignore_nulls=True,
                pub_key=self.pub,
                add_actors=self.add_actors).to_pandas()

            tmp_count = tmp_col.count().to_pandas()
            tmp_df = pd.merge(tmp_sum, tmp_count)

            df_list.append(tmp_df)

        # return tmp_sum.to_pandas()
        # return pd.merge(tmp_sum, tmp_count)
        return df_list


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
        fl_console_log.debug("Send value with tag '{}' to {} finish".format(
            tag, self.dest_role))

        # logger.debug("Send value with tag '{}' to {} finish".format(
        #     tag, self.dest_role))

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
            fl_console_log.info("Start recv loop.")
            # logger.info("Start recv loop.")
            while (not self.stop_signal_):
                try:
                    msg = self.chann_.recv(block=False)
                except Exception as e:
                    fl_console_log.error(e)
                    # logger.error(e)
                    break

                if msg is None:
                    continue

                key = msg["tag"]
                value = msg["v"]
                if self.recv_cache_.get(key, None) is not None:
                    fl_console_log.warn(
                        "Hash entry for tag '{}' is not empty, replace old value"
                        .format(key))
                    # logger.warn(
                    #     "Hash entry for tag '{}' is not empty, replace old value"
                    #     .format(key))
                    del self.recv_cache_[key]

                fl_console_log.debug("Recv msg with tag '{}'.".format(key))
                # logger.debug("Recv msg with tag '{}'.".format(key))
                self.recv_cache_[key] = value
                self.chann_.send("ok")
            fl_console_log.info("Recv loop stops.")
            # logger.info("Recv loop stops.")

        self.recv_loop_fut_ = self.executor_.submit(recv_loop)

    # Stop recv thread.
    def StopRecvLoop(self):
        self.stop_signal_ = True
        self.recv_loop_fut_.result()
        fl_console_log.info("Recv loop already exit, clean cached value.")
        # logger.info("Recv loop already exit, clean cached value.")
        key_list = list(self.recv_cache_.keys())
        for key in key_list:
            del self.recv_cache_[key]
            fl_console_log.warn(
                "Remove value with tag '{}', not used until now.".format(key))
            # logger.warn(
            #     "Remove value with tag '{}', not used until now.".format(key))
        # logger.info("Release system resource!")
        fl_console_log.info("Release system resource!")
        self.chann_.socket.close()

    # Get value from cache, and the check will repeat at most 'retries' times,
    # and sleep 0.3s after each check to avoid check all the time.
    def Get(self, tag, max_time=10000, interval=0.1):
        start = time.time()
        while True:
            val = self.recv_cache_.get(tag, None)
            end = time.time()
            if val is not None:
                del self.recv_cache_[tag]
                fl_console_log.debug(
                    "Get val with tag '{}' finish.".format(tag))
                # logger.debug("Get val with tag '{}' finish.".format(tag))
                return pickle.loads(val)

            if (end - start) > max_time:
                fl_console_log.warn(
                    "Can't get value for tag '{}', timeout.".format(tag))
                break

            time.sleep(interval)

        return None


def evaluate_ks_and_roc_auc(y_real, y_proba):
    # Unite both visions to be able to filter
    df = pd.DataFrame()
    df['real'] = y_real
    # df['proba'] = y_proba[:, 1]
    df['proba'] = y_proba

    # Recover each class
    class0 = df[df['real'] == 0]
    class1 = df[df['real'] == 1]

    ks = ks_2samp(class0['proba'], class1['proba'])
    roc_auc = roc_auc_score(df['real'], df['proba'])

    print(f"KS: {ks.statistic:.4f} (p-value: {ks.pvalue:.3e})")
    print(f"ROC AUC: {roc_auc:.4f}")
    return ks.statistic, roc_auc


def sum_job(tmp_group, encrypted, pub, paillier_add_actors):
    if encrypted:
        tmp_sum = tmp_group._aggregate_on(PallierSum,
                                          on=['g', 'h'],
                                          ignore_nulls=True,
                                          pub_key=pub,
                                          add_actors=paillier_add_actors)
    else:
        tmp_group = tmp_group.sum(on=['g', 'h'])

    return tmp_sum.to_pandas()


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
            sample_ratio=0.3,
            batch_size=8):
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
        self.feature_ratio = sample_ratio
        self.batch_size = batch_size

    def sum_job(self, tmp_group):
        if self.encrypted:
            tmp_sum = tmp_group._aggregate_on(
                PallierSum,
                on=['g', 'h'],
                ignore_nulls=True,
                pub_key=self.pub,
                add_actors=self.paillier_add_actors)
        else:
            tmp_group = tmp_group.sum(on=['g', 'h'])

        return tmp_sum.to_pandas()

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

        if self.encrypted:
            if self.merge_gh:
                X_guest['g'] = encrypted_ghs['g']
                cols = X_guest.columns.difference(['g', 'h'])
            else:
                X_guest['g'] = encrypted_ghs['g']
                X_guest['h'] = encrypted_ghs['h']
                cols = X_guest.columns.difference(['g', 'h'])
        else:
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

        # print("current x-guset and buckets_x_guest", X_guest,
        #       buckets_x_guest.to_pandas(), encrypted_ghs,
        #       buckets_x_guest.to_pandas().shape, encrypted_ghs.shape)

        total_left_ghs = {}

        groups = []
        batch_size = 5
        internal_groups = []
        for tmp_col in cols:
            # print("=====tmp_col======", tmp_col)
            if self.encrypted:
                if self.merge_gh:
                    tmp_group = buckets_x_guest.select_columns(
                        cols=[tmp_col, "g"]).groupby(tmp_col)
                else:
                    tmp_group = buckets_x_guest.select_columns(
                        cols=[tmp_col, "g", "h"]).groupby(tmp_col)
            else:
                tmp_group = buckets_x_guest.select_columns(
                    cols=[tmp_col, "g", "h"]).groupby(tmp_col)

            internal_groups.append(tmp_group)

        # each_bin = int(len(internal_groups) / self.batch_size)

        groups = [
            internal_groups[x:x + self.batch_size]
            for x in range(0, len(internal_groups), self.batch_size)
        ]
        print("==============", cols, groups, len(groups))

        if self.encrypted:
            # internal_res = list(
            #     self.grouppools.map(lambda a, v: a.groupby.remote(v), groups))

            if self.merge_gh:
                internal_res = ray.get(
                    groupby_sum.remote(group_col=groups,
                                       pub=self.pub,
                                       on_cols=['g'],
                                       add_actors=self.grouppools))
            else:
                internal_res = ray.get(
                    groupby_sum.remote(group_col=groups,
                                       pub=self.pub,
                                       on_cols=['g', 'h'],
                                       add_actors=self.grouppools))

            res = []
            for tmp_res in internal_res:
                res += tmp_res

            # if len(groups) > 20:
            #     mid = int(len(groups) / 2)
            #     groups1 = groups[:mid]
            #     groups2 = groups[mid:]

            #     res1 = list(
            #         self.grouppools.map(lambda a, v: a.groupby.remote(v),
            #                             groups1))

            #     with self.cli1:
            #         if self.merge_gh:
            #             res2 = [
            #                 tmp_group._aggregate_on(
            #                     PallierSum,
            #                     on=['g'],
            #                     ignore_nulls=True,
            #                     pub_key=self.pub,
            #                     add_actors=self.paillier_add_actors).to_pandas(
            #                     ) for tmp_group in groups2
            #             ]

            #         else:
            #             res2 = [
            #                 tmp_group._aggregate_on(
            #                     PallierSum,
            #                     on=['g', 'h'],
            #                     ignore_nulls=True,
            #                     pub_key=self.pub,
            #                     add_actors=self.paillier_add_actors).to_pandas(
            #                     ) for tmp_group in groups2
            #             ]

            #     res = res1 + res2
            # else:

            # if len(groups) > 20:
            #     mid = int(len(groups) / 2)
            #     groups1 = groups[:mid]
            #     groups2 = groups[mid:]

            #     res1 = list(
            #         self.grouppools.map(lambda a, v: a.groupby.remote(v),
            #                             groups1))

            #     with self.cli1:
            #         if self.merge_gh:
            #             res2 = [
            #                 tmp_group._aggregate_on(
            #                     PallierSum,
            #                     on=['g'],
            #                     ignore_nulls=True,
            #                     pub_key=self.pub,
            #                     add_actors=self.paillier_add_actors).to_pandas(
            #                     ) for tmp_group in groups2
            #             ]

            #         else:
            #             res2 = [
            #                 tmp_group._aggregate_on(
            #                     PallierSum,
            #                     on=['g', 'h'],
            #                     ignore_nulls=True,
            #                     pub_key=self.pub,
            #                     add_actors=self.paillier_add_actors).to_pandas(
            #                     ) for tmp_group in groups2
            #             ]

            #     res = res1 + res2
            # if self.merge_gh:
            #     grouppools2 = ActorPool([
            #         GroupPool.remote(self.paillier_add_actors,
            #                          self.pub,
            #                          on_cols=['g']) for _ in range(10)
            #     ])
            # else:
            #     grouppools2 = ActorPool([
            #         GroupPool.remote(self.paillier_add_actors,
            #                          self.pub,
            #                          on_cols=['g', 'h'])
            #         for _ in range(10)
            #     ])

            # res2 = list(
            #     self.grouppools.map(lambda a, v: a.groupby.remote(v),
            #                         groups1))

            # pass

            # else:
            # res = list(
            #     self.grouppools.map(lambda a, v: a.groupby.remote(v), groups))
        else:
            res = [
                tmp_group.sum(on=['g', 'h']).to_pandas()
                for tmp_group in internal_groups
            ]

        for key, tmp_task in zip(cols, res):
            # total_left_ghs[key] = tmp_task.get()
            total_left_ghs[key] = tmp_task

            # if self.encrypted:
            #     tmp_sum = tmp_group._aggregate_on(
            #         PallierSum,
            #         on=['g', 'h'],
            #         ignore_nulls=True,
            #         pub_key=self.pub,
            #         add_actors=self.paillier_add_actors)
            # else:
            #     tmp_sum = tmp_group.sum(on=['g', 'h'])
            # total_left_ghs[tmp_col] = tmp_sum.to_pandas()

            # total_left_ghs[tmp_col] = tmp_sum.to_pandas().sort_values(
            #     by=tmp_col, ascending=True)

        # print("current total_left_ghs: ", total_left_ghs)

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
        self.proxy_client_host.Remote(encrypte_gh_sums, 'encrypte_gh_sums')
        best_cut = self.proxy_server.Get('best_cut')

        # logging.info("current best cut: {}".format(best_cut))
        fl_console_log.info("current best cut: {}".format(best_cut))

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

                self.proxy_client_host.Remote(
                    {
                        'id_left': id_left,
                        'id_right': id_right,
                        'w_left': w_left,
                        'w_right': w_right
                    }, 'ids_w')
                # updata guest lookup table
                self.lookup_table[self.guest_record] = [best_var, best_cut]
                fl_console_log.info("guest look_up table is {}".format(
                    self.lookup_table))
                # print("guest look_up table:", self.lookup_table)

                # self.guest_record += 1
                self.guest_record += 1

            else:
                ids_w = self.proxy_server.Get('ids_w')
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
            sample_ids = self.proxy_server.Get('sample_ids')
            if sample_ids is None:
                current_x = X_guest.copy()
            else:
                current_x = X_guest.iloc[sample_ids].copy()
            # gh_host = xgb_guest.channel.recv()
            gh_en = self.proxy_server.Get('gh_en')
            # print("gh_en: ", gh_en)
            self.tree_structure[t + 1] = self.guest_tree_construct(
                current_x, gh_en, 0)

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
                self.proxy_client_host.Remote(
                    {
                        'id_left': id_left,
                        'id_right': id_right
                    },
                    str(record_id) + '_ids')

            else:
                ids = self.proxy_server.Get(str(record_id) + '_ids')
                id_left = ids['id_left']

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


# Number of tree to fit.
num_tree = 1
# the depth of each tree
max_depth = 3
# whether encrypted or not
is_encrypted = False
merge_gh = True

ray_group = True

min_child_weight = 5

sample_type = "random"

feature_sample = True

if __name__ == "__main__":

    # @ph.context.function(
    #     role='guest',
    #     protocol='xgboost',
    #     datasets=[
    #         'train_hetero_xgb_guest'  #'five_thous_guest'
    #     ],  #['train_hetero_xgb_guest'],  #, 'test_hetero_xgb_guest'],
    #     port='9000',
    #     task_type="classification")
    # def xgb_guest_logic(cry_pri="paillier"):
    # def xgb_guest_logic(cry_pri="plaintext"):
    # fl_console_log.info("start xgb guest logic...")

    # ios = IOService()
    # role_node_map = ph.context.Context.get_role_node_map()
    # node_addr_map = ph.context.Context.get_node_addr_map()
    # dataset_map = ph.context.Context.dataset_map
    # taskId = ph.context.Context.params_map['taskid']
    # jobId = ph.context.Context.params_map['jobid']
    taskId = 100
    jobId = 200

    guest_log_console = FLConsoleHandler(jb_id=jobId,
                                         task_id=taskId,
                                         log_level='info',
                                         format=FORMAT)
    fl_console_log = guest_log_console.set_format()
    # fl_console_log.debug("dataset_map {}".format(dataset_map))

    # logger.debug("dataset_map {}".format(dataset_map))
    # fl_console_log.debug("role_nodeid_map {}".format(role_node_map))

    # logger.debug("role_nodeid_map {}".format(role_node_map))
    # fl_console_log.debug("node_addr_map {}".format(node_addr_map))

    # logger.debug("node_addr_map {}".format(node_addr_map))

    # data_key = list(dataset_map.keys())[0]

    # eva_type = ph.context.Context.params_map.get("taskType", None)
    # if eva_type is None:
    #     fl_console_log.warn(
    #         "taskType is not specified, set to default value 'regression'.")
    #     # logger.warn(
    #     #     "taskType is not specified, set to default value 'regression'.")
    #     eva_type = "regression"

    # eva_type = eva_type.lower()
    # if eva_type != "classification" and eva_type != "regression":
    #     fl_console_log.error(
    #         "Invalid value of taskType, possible value is 'regression', 'classification'."
    #     )
    #     # logger.error(
    #     #     "Invalid value of taskType, possible value is 'regression', 'classification'."
    #     # )
    #     return

    # fl_console_log.info("Current task type is {}.".format(eva_type))

    # if len(role_node_map["host"]) != 1:
    #     fl_console_log.error("Current node of host party: {}".format(
    #         role_node_map["host"]))
    #     # logger.error("Current node of host party: {}".format(
    #     #     role_node_map["host"]))
    #     fl_console_log.error(
    #         "In hetero XGB, only dataset of host party has label,"
    #         "so host party must have one, make sure it.")
    #     # logger.error("In hetero XGB, only dataset of host party has label,"
    #     #              "so host party must have one, make sure it.")
    #     return

    # guest_nodes = role_node_map["guest"]
    # guest_port = node_addr_map[guest_nodes[0]].split(":")[1]
    guest_port = 9000
    host_ip = "172.21.3.108"
    host_port = 8000

    proxy_server = ServerChannelProxy(guest_port)
    proxy_server.StartRecvLoop()
    # fl_console_log.debug(
    #     "Create server proxy for guest, port {}.".format(guest_port))

    # host_nodes = role_node_map["host"]
    # host_ip, host_port = node_addr_map[host_nodes[0]].split(":")

    proxy_client_host = ClientChannelProxy(host_ip, host_port, "host")
    # data = ph.dataset.read(dataset_key=data_key).df_data
    data = pd.read_csv("data/FL/hetero_xgb/train/train_breast_cancer_guest.csv")

    # data = ph.dataset.read(dataset_key='train_heter_xxgb_guest').df_data
    # data = pd.read_csv('/home/primihub/xusong/data/epsilon_normalized.t.guest',
    #                    header=0)

    # # samples-50000, cols-450
    # data = data.iloc[:50000, :450]
    # data = data.iloc[:, :450]

    if 'id' in data.columns:
        data.pop('id')
    X_guest = data
    # guest_log = open('/app/guest_log', 'w+')
    # if is_encrypted:
    xgb_guest = XGB_GUEST_EN(n_estimators=num_tree,
                             max_depth=max_depth,
                             reg_lambda=1,
                             min_child_weight=min_child_weight,
                             objective='logistic',
                             sid=1,
                             proxy_server=proxy_server,
                             proxy_client_host=proxy_client_host,
                             is_encrypted=is_encrypted,
                             sample_ratio=0.3)  # noqa

    if feature_sample:
        guest_cols = X_guest.columns.tolist()
        selected_features = col_sample(guest_cols,
                                       sample_ratio=xgb_guest.feature_ratio)
        X_guest = X_guest[selected_features]

    pub = proxy_server.Get('xgb_pub')
    xgb_guest.pub = pub
    xgb_guest.merge_gh = merge_gh
    xgb_guest.batch_size = 10

    add_actor_num = 20
    paillier_add_actors = ActorPool(
        [ActorAdd.remote(xgb_guest.pub) for _ in range(add_actor_num)])

    # ray.init(address='ray://172.21.3.16:10001')

    if xgb_guest.merge_gh:
        grouppools = ActorPool([
            GroupPool.remote(paillier_add_actors, xgb_guest.pub, on_cols=['g'])
            for _ in range(10)
        ])
    else:
        grouppools = ActorPool([
            GroupPool.remote(paillier_add_actors,
                             xgb_guest.pub,
                             on_cols=['g', 'h']) for _ in range(10)
        ])
    xgb_guest.paillier_add_actors = paillier_add_actors
    xgb_guest.grouppools = grouppools

    # cli1 = ray.init("ray://172.21.3.126:10001", allow_multiple=True)

    # xgb_guest.channel.send(b'recved pub')
    lookup_table_sum = {}
    xgb_guest.lookup_table = {}
    # xgb_guest.cli1 = cli1

    xgb_guest.fit(X_guest, lookup_table_sum)

    # predict_file_path = ph.context.Context.get_predict_file_path()
    # indicator_file_path = ph.context.Context.get_indicator_file_path()
    # guest_model_path = ph.context.Context.get_guest_model_path()
    # lookup_file_path = ph.context.Context.get_guest_lookup_file_path(
    # ) + ".guest"
    # guest_model_path = ph.context.Context.get_model_file_path() + ".guest"

    # save guest part model
    with open("guestModel", 'wb') as guestModel:
        pickle.dump(
            {
                'tree_struct': xgb_guest.tree_structure,
                'lr': xgb_guest.learning_rate
            }, guestModel)

    # save guest part table
    with open("guestTable", 'wb') as guestTable:
        pickle.dump(lookup_table_sum, guestTable)

    xgb_guest.predict(X_guest.copy(), lookup_table_sum)

    # xgb_guest.predict(X_guest)
    proxy_server.StopRecvLoop()
    # guest_log.close()
