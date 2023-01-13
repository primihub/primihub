import primihub as ph
from primihub import dataset, context
from multiprocessing import cpu_count
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
import pathos
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

    def __init__(self, merge_gh) -> None:
        self.merge_gh = merge_gh

    def groupby(self, internal_groups):
        df_list = []
        if self.merge_gh:
            for tmp_col in internal_groups:
                tmp_sum = tmp_col.sum(on=['g']).to_pandas()
                tmp_cnt = tmp_col.count().to_pandas()
                tmp_df = pd.merge(tmp_sum, tmp_cnt)
                df_list.append(tmp_df)
        else:
            for tmp_col in internal_groups:
                tmp_sum = tmp_col.sum(on=['g', 'h']).to_pandas()
                tmp_sum.rename(columns={
                    'sum(g)': 'g',
                    'sum(h)': 'h'
                },
                               inplace=True)
                df_list.append(tmp_sum)

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
            top_ratio=0.2,
            other_ratio=0.2,
            sample_type='goss',
            limit_size=20000,
            host_iter=0):

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
        self.ratio = 10**10
        self.const = 2
        self.g_ratio = 10**8
        self.encrypted = encrypted
        self.global_transfer_time = 0
        self.gloabl_construct_time = 0
        self.top_ratio = top_ratio
        self.other_ratio = other_ratio
        self.sample_type = sample_type
        self.limit_size = limit_size
        self.host_iter = host_iter

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
        best_gain = 0.001
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

    def get_sum_on_ids(self, df_buckest: pd.DataFrame, plain_gh: pd.DataFrame):
        if 'g' in df_buckest.columns:
            df_buckest.pop('g')

        if 'h' in df_buckest.columns:
            df_buckest.pop('h')

        m, n = df_buckest.shape
        print("=========", plain_gh)

        df_buckest['g'] = plain_gh['g']
        df_buckest['h'] = plain_gh['h']
        iter_cols = df_buckest.columns.difference(['g', 'h'])
        ray_ds = ray.data.from_pandas(df_buckest)
        plain_gh_sums = plain_gh.sum(axis=0)

        G_lefts = []
        G_rights = []
        H_lefts = []
        H_rights = []
        cuts = []
        vars = []

        for current_col in iter_cols:
            current_ds = ray_ds.select_columns(cols=[current_col, 'g', 'h'])
            # tmp_cnt = current_ds.count()
            current_gpdata = current_ds.groupby(current_col)
            gh_sums = current_gpdata.sum(on=['g', 'h']).to_pandas()

            gh_sums.rename(columns={'sum(g)': 'g', 'sum(h)': 'h'}, inplace=True)

            sorted_gh_sums = gh_sums.sort_values(by=current_col, ascending=True)
            cumsum_sorted_gh_sums = sorted_gh_sums.cumsum()
            current_m, current_n = cumsum_sorted_gh_sums.shape

            # tmp_g_lefts = cumsum_sorted_gh_sums['g']
            tmp_g_lefts = cumsum_sorted_gh_sums['g']
            # tmp_h_lefts = cumsum_sorted_gh_sums['h']
            tmp_h_lefts = cumsum_sorted_gh_sums['h']

            tmp_var = [current_col] * current_m
            tmp_cut = sorted_gh_sums[current_col]

            G_lefts += tmp_g_lefts.values.tolist()
            H_lefts += tmp_h_lefts.values.tolist()
            vars += tmp_var
            cuts += tmp_cut.values.tolist()

            print("=======", tmp_g_lefts, tmp_h_lefts, tmp_var, tmp_cut)

        GHS = pd.DataFrame({
            'G_left': G_lefts,
            'H_left': H_lefts,
            'var': vars,
            'cut': cuts
        })

        GHS['G_right'] = plain_gh_sums['g'] - GHS['G_left']
        GHS['H_right'] = plain_gh_sums['h'] - GHS['H_left']

        return GHS

    def assign_or_append(self, obj, buffer):
        if obj is None:
            return buffer

        return obj + buffer

    def sum_bygroup(self, select_group):
        select_res = []
        for tmp_group in select_group:
            bucket_ghs_sum = tmp_group.sum(on=['g', 'h']).to_pandas()
            bucket_ghs_sum.rename(columns={
                'sum(g)': 'g',
                'sum(h)': 'h'
            },
                                  inplace=True)

            values = bucket_ghs_sum.values
            col_names = bucket_ghs_sum.columns
            select_res.append((values, col_names))

        return select_res

    def gh_sums_bucket_guest(self, bucket_guest: pd.DataFrame, plain_gh):
        print("current bucket_guest: ", bucket_guest)

        # merge_df = pd.concat([bucket_guest, plain_gh], axis=1)
        # plain_gh = self.gh.iloc[bucket_guest.index]

        bucket_guest['g'] = plain_gh['g']
        bucket_guest['h'] = plain_gh['h']
        if self.merge_gh:
            bucket_guest['g'] = self.G['g']

        ray_ds = ray.data.from_pandas(bucket_guest)

        plain_gh_sums = plain_gh.sum(axis=0)

        gh_cols = ['g', 'h']

        cols = bucket_guest.columns.difference(['g', 'h'])

        G1 = None
        G2 = None
        H1 = None
        H2 = None
        Cuts = None
        Vars = None

        internal_groups = []

        for tmp_col in cols:
            select_cols = [tmp_col] + gh_cols
            select_ds = ray_ds.select_columns(cols=select_cols)
            select_group = select_ds.groupby(tmp_col)
            internal_groups.append(select_group)

        self.batch_size = 8
        groups = [
            internal_groups[x:x + self.batch_size]
            for x in range(0, len(internal_groups), self.batch_size)
        ]
        res = list(
            self.group_pools.map(lambda a, v: a.groupby.remote(v), groups))

        plat_res = []
        for tmp_res in res:
            plat_res += tmp_res

        # pool = pathos.multiprocessing.Pool()
        # res = pool.map_async(self.sum_bygroup, groups).get()
        # for cur_res in res:
        #     for internal in cur_res:
        #         values = internal[0]
        #         col_names = internal[1]
        #         inter_df = pd.DataFrame(values, columns=col_names)
        #         plat_res.append(inter_df)
        if self.merge_gh:
            for tmp_col, bucket_ghs_sum in zip(cols, plat_res):
                ascending_ghs_sum = bucket_ghs_sum.sort_values(by=tmp_col,
                                                               ascending=True)
                ascending_ghs_sum[
                    'g'] = ascending_ghs_sum['sum(g)'] // self.ratio
                ascending_ghs_sum['h'] = ascending_ghs_sum[
                    'sum(g)'] - ascending_ghs_sum['g'] * self.ratio

                ascending_ghs_sum['g'] = ascending_ghs_sum[
                    'g'] - self.const * ascending_ghs_sum['count()']
                cum_ghs_sum = ascending_ghs_sum.cumsum()
                g_lefts = cum_ghs_sum['g'].values.tolist()
                h_lefts = cum_ghs_sum['h'].values.tolist()
                g_rights = (plain_gh_sums['g'] -
                            cum_ghs_sum['g']).values.tolist()
                h_rights = (plain_gh_sums['h'] -
                            cum_ghs_sum['h']).values.tolist()
                var_name = [tmp_col] * len(g_lefts)
                vat_cuts = bucket_ghs_sum[tmp_col].values.tolist()

                G1 = self.assign_or_append(G1, g_lefts)
                G2 = self.assign_or_append(G2, g_rights)
                H1 = self.assign_or_append(H1, h_lefts)
                H2 = self.assign_or_append(H2, h_rights)
                Vars = self.assign_or_append(Vars, var_name)
                Cuts = self.assign_or_append(Cuts, vat_cuts)

        else:
            for tmp_col, bucket_ghs_sum in zip(cols, plat_res):
                ascending_ghs_sum = bucket_ghs_sum.sort_values(by=tmp_col,
                                                               ascending=True)
                cum_ghs_sum = ascending_ghs_sum.cumsum()
                g_lefts = cum_ghs_sum['g'].values.tolist()
                h_lefts = cum_ghs_sum['h'].values.tolist()
                g_rights = (plain_gh_sums['g'] -
                            cum_ghs_sum['g']).values.tolist()
                h_rights = (plain_gh_sums['h'] -
                            cum_ghs_sum['h']).values.tolist()
                var_name = [tmp_col] * len(g_lefts)
                vat_cuts = bucket_ghs_sum[tmp_col].values.tolist()

                G1 = self.assign_or_append(G1, g_lefts)
                G2 = self.assign_or_append(G2, g_rights)
                H1 = self.assign_or_append(H1, h_lefts)
                H2 = self.assign_or_append(H2, h_rights)
                Vars = self.assign_or_append(Vars, var_name)
                Cuts = self.assign_or_append(Cuts, vat_cuts)

        return pd.DataFrame({
            'G_left': G1,
            'G_right': G2,
            'H_left': H1,
            'H_right': H2,
            'var': Vars,
            'cut': Cuts
        })

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
                if self.merge_gh:
                    m = len(val)
                    col = "sum(g)"
                    tmp_var = [key] * m
                    tmp_cut = val[key].values.tolist()
                    val[col] = [
                        opt_paillier_decrypt_crt(self.pub, self.prv, item)
                        for item in val[col]
                    ]
                    val = val.sort_values(by=key, ascending=True)
                    val['g'] = val['sum(g)'] // self.ratio
                    val['h'] = val['sum(g)'] - val['g'] * self.ratio
                    val['g'] = val['g'] / 10**4 - self.const * val['count()']
                    val['h'] = val['h'] / 10**4

                    tmp_g_lefts = val['g'].cumsum()
                    tmp_h_lefts = val['h'].cumsum()

                    # val['sum(h)'] = val['sum(g)'] % (self.ratio * self.ratio)
                    # val['sum(g)'] = val['sum(g)'] // (self.ratio * self.ratio)

                    # # substract const
                    # val['sum(g)'] = val[
                    #     'sum(g)'] / self.ratio - self.const * val['count()']
                    # val['sum(h)'] = val['sum(h)'] / self.ratio
                    # tmp_g_lefts = val['sum(g)'].cumsum(
                    # )  #/ self.ratio - self.const * val['count']
                    # tmp_h_lefts = val['sum(h)'].cumsum()  #/ self.ratio

                    G_lefts += tmp_g_lefts.values.tolist()
                    H_lefts += tmp_h_lefts.values.tolist()
                    vars += tmp_var
                    cuts += tmp_cut

                else:
                    m, n = val.shape
                    tmp_var = [key] * m
                    tmp_cut = val[key].values.tolist()
                    for col in sum_col:
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
        # print("current gh_sums and plain_gh_sums: ", gh_sums, plain_gh_sums)

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
        best_gain = 0.001
        if guest_gh_sums.empty:
            return None
        guest_gh_sums['gain'] = guest_gh_sums['G_left'] ** 2 / (guest_gh_sums['H_left'] + self.reg_lambda) + \
                guest_gh_sums['G_right'] ** 2 / (guest_gh_sums['H_right'] + self.reg_lambda) - \
                (guest_gh_sums['G_left'] + guest_gh_sums['G_right']) ** 2 / (
                guest_gh_sums['H_left'] + guest_gh_sums['H_right'] + + self.reg_lambda)

        guest_gh_sums['gain'] = guest_gh_sums['gain'] / 2 - self.gamma
        # print("guest_gh_sums: ", guest_gh_sums)

        max_row = guest_gh_sums['gain'].idxmax()
        fl_console_log.info("max_row: {}".format(max_row))
        # print("max_row: ", max_row)
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
        self.host_iter += 1
        print("current_depth: ", current_depth, self.host_iter, X_host,
              plain_gh)

        if (self.min_child_sample and
                m < self.min_child_sample) or current_depth > self.max_depth:
            return

        # get the best cut of 'host'
        host_best = self.host_best_cut(X_host,
                                       cal_hist=True,
                                       bins=10,
                                       plain_gh=plain_gh)

        plain_gh_sums = plain_gh.sum(axis=0)

        guest_gh_sums = self.proxy_server.Get(
            'encrypte_gh_sums' + str(self.host_iter)
        )  # the item contains {'G_left', 'G_right', 'H_left', 'H_right', 'var', 'cut'}

        # decrypted the 'guest_gh_sums' with paillier
        if trans_guest_buckets:
            # dec_guest_gh_sums = self.get_sum_on_ids(guest_gh_sums, plain_gh)
            # self.get_sum_on_ids(guest_gh_sums, )
            dec_guest_gh_sums = self.gh_sums_bucket_guest(
                guest_gh_sums, plain_gh)

        else:
            if ray_group:
                dec_guest_gh_sums = self.gh_sums_decrypted_with_ray(
                    guest_gh_sums, plain_gh_sums=plain_gh_sums)
            else:
                dec_guest_gh_sums = self.gh_sums_decrypted(
                    guest_gh_sums, plain_gh_sums=plain_gh_sums)

        # get the best cut of 'guest'
        guest_best = self.guest_best_cut(dec_guest_gh_sums)
        # guest_best_gain = guest_best['gain']

        self.proxy_client_guest.Remote(
            {
                'host_best': host_best,
                'guest_best': guest_best
            }, 'best_cut')

        fl_console_log.info(
            "current depth: {}, host best var: {} and guest best var: {}".
            format(current_depth, host_best, guest_best))

        # logging.info(
        #     "current depth: {}, host best var: {} and guest best var: {}".
        #     format(current_depth, host_best, guest_best))
        fl_console_log.info("Best host is {} and best guest is {}".format(
            host_best, guest_best))
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
                ids_w = self.proxy_server.Get('ids_w')

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
                self.proxy_client_guest.Remote(
                    {
                        'id_left': id_left,
                        'id_right': id_right,
                        'w_left': w_left,
                        'w_right': w_right
                    }, 'ids_w')

                self.lookup_table[self.host_record] = [
                    host_best['best_var'], host_best['best_cut']
                ]
                # print("self.lookup_table", self.lookup_table)

                self.host_record += 1

            tree_structure = {(role, record): {}}
            fl_console_log.info("current role: {}, current record: {}".format(
                role, record))

            X_host_left = X_host.loc[id_left]
            plain_gh_left = plain_gh.loc[id_left]

            X_host_right = X_host.loc[id_right]
            plain_gh_right = plain_gh.loc[id_right]

            f_t[id_left] = w_left
            f_t[id_right] = w_right

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
            fl_console_log.info("Begin to trian tree {}".format(t + 1))
            # print("Begin to trian tree: ", t + 1)
            f_t = pd.Series([0] * Y.shape[0])

            # host cal gradients and hessians with its own label
            gh = pd.DataFrame({
                'g': self._grad(y_hat, Y.flatten()),
                'h': self._hess(y_hat, Y.flatten())
            })
            self.gh = gh
            if self.sample_type == 'goss' and len(X_host) > self.limit_size:
                top_ids, low_ids = goss_sample(gh,
                                               top_rate=self.top_ratio,
                                               other_rate=self.other_ratio)

                amply_rate = (1 - self.top_ratio) / self.other_ratio

                # amplify the selected smaller gradients
                gh['g'][low_ids] *= amply_rate

                sample_ids = top_ids + low_ids

            elif self.sample_type == 'random' and len(X_host) > self.limit_size:
                sample_ids = random_sample(gh,
                                           top_rate=self.top_ratio,
                                           other_rate=self.other_ratio)

                # TODO: Amplify the sample gradients
                # X_host = X_host.iloc[sample_ids].copy()
                # Y = Y[sample_ids]
                # ghs = ghs.iloc[sample_ids].copy()

            else:
                sample_ids = None

            self.proxy_client_guest.Remote(sample_ids, "sample_ids")
            if sample_ids is not None:
                # select from 'X_host', Y and ghs
                # print("sample_ids: ", sample_ids)
                current_x = X_host.iloc[sample_ids].copy()
                # current_y = Y[sample_ids]
                current_ghs = gh.iloc[sample_ids].copy()
                current_y_hat = y_hat[sample_ids]
                current_f_t = f_t.iloc[sample_ids].copy()
            else:
                current_x = X_host.copy()
                # current_y = Y.copy()
                current_ghs = gh.copy()
                current_y_hat = y_hat.copy()
                current_f_t = f_t.copy()

            # else:
            #     sample_ids
            #     raise ValueError("The {self.sample_type} was not defined!")

            # convert gradients and hessians to ints and encrypted with paillier
            # ratio = 10**3
            # gh_large = (gh * ratio).astype('int')
            if self.encrypted:
                if self.merge_gh:
                    # cp_gh = gh.copy()
                    cp_gh = current_ghs.copy()
                    cp_gh['g'] = cp_gh['g'] + self.const
                    cp_gh = np.round(cp_gh, 4)
                    cp_gh = (cp_gh * 10**4).astype('int')
                    # cp_gh = cp_gh * self.ratio
                    # make 'g' positive
                    # gh['g'] = gh['g'] + self.const
                    # gh *= self.ratio
                    # cp_gh_int = (cp_gh * self.ratio).astype('int')

                    merge_gh = (cp_gh['g'] * self.ratio + cp_gh['h'])
                    # print("merge_gh: ", merge_gh.values)
                    start_enc = time.time()
                    enc_merge_gh = list(
                        paillier_encryptor.map(lambda a, v: a.pai_enc.remote(v),
                                               merge_gh.values.tolist()))

                    enc_gh_df = pd.DataFrame({
                        'g': enc_merge_gh,
                        'id': cp_gh.index.tolist()
                    })
                    # cp_gh['g'] = enc_merge_gh
                    # enc_gh_df = cp_gh['g']
                    enc_gh_df.set_index('id', inplace=True)
                    # print("enc_gh_df: ", enc_gh_df)
                    end_enc = time.time()

                    # merge_gh = (gh['g'] * self.g_ratio +
                    #             gh['h']).values.atype('int')
                    # enc_gh_df = pd.DataFrame({'merge_gh': merge_gh})

                else:
                    # flat_gh = gh.values.flatten()
                    flat_gh = current_ghs.values.flatten()
                    flat_gh *= self.ratio

                    flat_gh = flat_gh.astype('int')

                    start_enc = time.time()
                    enc_flat_gh = list(
                        paillier_encryptor.map(lambda a, v: a.pai_enc.remote(v),
                                               flat_gh.tolist()))

                    end_enc = time.time()

                    enc_gh = np.array(enc_flat_gh).reshape((-1, 2))
                    # enc_gh_df = pd.DataFrame(enc_gh, )
                    enc_gh_df = pd.DataFrame(enc_gh, columns=['g', 'h'])
                    enc_gh_df['id'] = current_ghs.index.tolist()
                    enc_gh_df.set_index('id', inplace=True)

                # send all encrypted gradients and hessians to 'guest'
                self.proxy_client_guest.Remote(enc_gh_df, "gh_en")

                end_send_gh = time.time()
                fl_console_log.info(
                    "The encrypted time is {} and the transfer time is {}".
                    format((end_enc - start_enc), (end_send_gh - end_enc)))
                # print("Time for encryption and transfer: ",
                #       (end_enc - start_enc), (end_send_gh - end_enc))
                fl_console_log.info("Encrypt finish.")
                # print("Encrypt finish.")

            else:
                if self.merge_gh:
                    cp_gh = current_ghs.copy()
                    cp_gh['g'] = cp_gh['g'] + self.const
                    cp_gh = np.round(cp_gh, 5)

                    merge_gh = (cp_gh['g'] * self.ratio + cp_gh['h'])

                    self.G = pd.DataFrame({'g': merge_gh})
                    # current_ghs['g'] = merge_gh

                    self.proxy_client_guest.Remote(
                        pd.DataFrame({'g': merge_gh}), "gh_en")
                else:
                    self.proxy_client_guest.Remote(current_ghs, "gh_en")

            # self.tree_structure[t + 1] = self.host_tree_construct(
            #     X_host.copy(), f_t, 0, gh)
            self.tree_structure[t + 1] = self.host_tree_construct(
                current_x.copy(), current_f_t, 0, current_ghs)
            # y_hat = y_hat + xgb_host.learning_rate * f_t

            end_build_tree = time.time()

            lookup_table_sum[t + 1] = self.lookup_table
            # y_hat = y_hat + self.learning_rate * f_t
            if sample_ids is None:
                y_hat = y_hat + self.learning_rate * current_f_t

            else:
                y_hat[sample_ids] = y_hat[
                    sample_ids] + self.learning_rate * current_f_t

            # current_y_hat = current_y_hat + self.learning_rate * current_f_t
            fl_console_log.info("Finish to trian tree {}.".format(t + 1))

            # logger.info("Finish to trian tree {}.".format(t + 1))

            current_loss = self.log_loss(Y, 1 / (1 + np.exp(-y_hat)))
            # current_loss = self.log_loss(current_y,
            #                              1 / (1 + np.exp(-current_y_hat)))
            train_losses.append(current_loss)

            fl_console_log.info("train_losses are {}.".format(train_losses))

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


# Number of tree to fit.
num_tree = 10
# the depth of each tree
max_depth = 3
# whether encrypted or not
is_encrypted = False
merge_gh = False

ray_group = True

min_child_weight = 5

sample_type = "random"

feature_sample = True

trans_guest_buckets = True
cpu_number = cpu_count()

if __name__ == "__main__":

    # @ph.context.function(
    #     role='host',
    #     protocol='xgboost',
    #     datasets=['train_hetero_xgb_host'
    #              ],  # ['train_hetero_xgb_host'],  #, 'test_hetero_xgb_host'],
    #     port='8000',
    #     task_type="classification")
    # def xgb_host_logic(cry_pri="paillier"):
    # fl_console_log.info("start xgb host logic...")
    logger.info("start xgb host logic...")
    fl_file_log.debug("xgb host logic file")
    fl_file_log.info("xgb host logic file")
    # ray.init(address='ray://172.21.3.16:10001')

    # # role_node_map = ph.context.Context.get_role_node_map()
    # # node_addr_map = ph.context.Context.get_node_addr_map()
    # # dataset_map = ph.context.Context.dataset_map
    # taskId = ph.context.Context.params_map['taskid']
    # jobId = ph.context.Context.params_map['jobid']
    taskId = 100
    jobId = 200

    host_log_console = FLConsoleHandler(jb_id=jobId,
                                        task_id=taskId,
                                        log_level='info',
                                        format=FORMAT)
    fl_console_log = host_log_console.set_format()

    # logger.debug("dataset_map {}".format(dataset_map))
    # fl_console_log.debug("dataset_map {}".format(dataset_map))

    # logger.debug("role_nodeid_map {}".format(role_node_map))
    # fl_console_log.debug("role_nodeid_map {}".format(role_node_map))

    # logger.debug("node_addr_map {}".format(no de_addr_map))
    # fl_console_log.debug("node_addr_map {}".format(node_addr_map))
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

    # logger.info("Current task type is {}.".format(eva_type))

    # 读取注册数据
    # data = ph.dataset.read(dataset_key=data_key).df_data
    # data = pd.read_csv("data/FL/hetero_xgb/train/train_breast_cancer_host.csv")
    # data = ph.dataset.read(dataset_key='train_hetero_xgb_host').df_data
    data = pd.read_csv('/home/xusong/data/epsilon_normalized.host',
                       header=0).iloc[:, 550:]

    # # samples-50000, cols-450
    # data = data.iloc[:50000, 550:]
    # data = data.iloc[:, 550:]

    # y = data.pop('Class').values

    # print("host data: ", data)

    # if len(role_node_map["host"]) != 1:
    #     fl_console_log.error("Current node of host party: {}".format(
    #         role_node_map["host"]))

    #     fl_console_log.error(
    #         "In hetero XGB, only dataset of host party has label, "
    #         "so host party must have one, make sure it.")
    # logger.error("Current node of host party: {}".format(
    #     role_node_map["host"]))
    # logger.error("In hetero XGB, only dataset of host party has label, "
    #              "so host party must have one, make sure it.")
    # return

    # host_nodes = role_node_map["host"]
    # host_port = node_addr_map[host_nodes[0]].split(":")[1]

    # guest_nodes = role_node_map["guest"]
    # guest_ip, guest_port = node_addr_map[guest_nodes[0]].split(":")
    host_port = 8000
    guest_ip = "172.21.3.126"
    guest_port = 9000

    proxy_server = ServerChannelProxy(host_port)
    proxy_server.StartRecvLoop()

    proxy_client_guest = ClientChannelProxy(guest_ip, guest_port, "guest")

    if 'id' in data.columns:
        data.pop('id')

    Y = data.pop('y').values
    X_host = data.copy()

    lookup_table_sum = {}

    # if is_encrypted:
    xgb_host = XGB_HOST_EN(n_estimators=num_tree,
                           max_depth=max_depth,
                           reg_lambda=1,
                           sid=0,
                           min_child_weight=min_child_weight,
                           objective='logistic',
                           proxy_server=proxy_server,
                           proxy_client_guest=proxy_client_guest,
                           encrypted=is_encrypted,
                           top_ratio=0.2,
                           other_ratio=0.1)

    xgb_host.merge_gh = merge_gh
    xgb_host.fl_console_log = fl_console_log
    group_pools = ActorPool(
        [GroupPool.remote(merge_gh) for _ in range(cpu_number - 3)])

    proxy_client_guest.Remote(xgb_host.pub, "xgb_pub")
    xgb_host.sample_type = sample_type
    xgb_host.group_pools = group_pools

    paillier_encryptor = ActorPool([
        PaillierActor.remote(xgb_host.prv, xgb_host.pub)
        for _ in range(cpu_number - 3)
    ])

    xgb_host.lookup_table = {}
    start = time.time()
    xgb_host.fit(X_host, Y, paillier_encryptor, lookup_table_sum)

    # lp = LineProfiler()
    # lp.add_function(xgb_host.host_tree_construct)
    # lp.add_function(xgb_host.gh_sums_decrypted)
    # lp_wrapper = lp(xgb_host.fit)
    # lp_wrapper(X_host=X_host,
    #            Y=Y,
    #            paillier_encryptor=paillier_encryptor,
    #            lookup_table_sum=lookup_table_sum)
    # lp.print_stats()

    end = time.time()

    fl_console_log.info("train time for is {}".format(end - start))

    # print("train time for xgboost: ", (end - start))
    y_hat = xgb_host.predict_prob(X_host, lookup=lookup_table_sum)

    ks, auc = evaluate_ks_and_roc_auc(y_real=Y, y_proba=y_hat)
    xgb_host.ks = ks
    xgb_host.auc = auc
    train_acc = metrics.accuracy_score((y_hat >= 0.5).astype('int'), Y)
    # acc = sum((y_hat >= 0.5).astype(int) == Y) / len(y_hat)
    xgb_host.acc = train_acc
    fpr, tpr, threshold = metrics.roc_curve(Y, y_hat)
    xgb_host.fpr = fpr.tolist()
    xgb_host.tpr = tpr.tolist()

    # model_file_path = ph.context.Context.get_model_file_path()
    # lookup_file_path = ph.context.Context.get_host_lookup_file_path()

    # save host-part model
    # model_file_path = ph.context.Context.get_model_file_path() + ".host"

    with open('hostModel', 'wb') as hostModel:
        pickle.dump(
            {
                'tree_struct': xgb_host.tree_structure,
                'lr': xgb_host.learning_rate
            }, hostModel)

    # save host-part table
    # lookup_file_path = ph.context.Context.get_host_lookup_file_path() + ".host"
    with open('hostTable', 'wb') as hostTable:
        pickle.dump(lookup_table_sum, hostTable)

    # indicator_file_path = ph.context.Context.get_indicator_file_path()

    trainMetrics = {
        "train_acc": xgb_host.acc,
        "train_auc": xgb_host.auc,
        "train_ks": xgb_host.ks,
        "train_fpr": xgb_host.fpr,
        "train_tpr": xgb_host.tpr
    }

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
    trainMetricsBuff = json.dumps(trainMetrics)
    with open('metrics.json', 'w') as filePath:
        filePath.write(trainMetricsBuff)

        # pickle.dump(trainMetrics, filePath)

    proxy_server.StopRecvLoop()
    # host_log.close()
