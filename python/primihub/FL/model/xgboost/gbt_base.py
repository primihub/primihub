# basic packages
import pyarrow
import random
import pickle
import pandas
import functools
import logging
import pandas as pd
import numpy as np
from typing import (
    List,
    Optional,
    Union,
    TypeVar,
)

# open source packages
import json
from sklearn import metrics
import logging
import ray
from ray.util import ActorPool
from primihub.new_FL.algorithm.utils.base_xus import BaseModel
from sklearn.preprocessing import StandardScaler, MinMaxScaler
from primihub.new_FL.algorithm.utils.net_work import GrpcServer, GrpcServers
from primihub.utils.evaluation import evaluate_ks_and_roc_auc, plot_lift_and_gain, eval_acc

from primihub.primitive.opt_paillier_c2py_warpper import opt_paillier_decrypt_crt, opt_paillier_encrypt_crt, opt_paillier_add, opt_paillier_keygen
from primihub.utils.logger_util import FLConsoleHandler, FORMAT
from ray.data.block import KeyFn

T = TypeVar("T", contravariant=True)
U = TypeVar("U", covariant=True)

Block = Union[List[T], "pyarrow.Table", "pandas.DataFrame", bytes]

_pandas = None

ray.init(ignore_reinit_error=True)


def lazy_import_pandas():
    global _pandas
    if _pandas is None:
        import pandas
        _pandas = pandas
    return _pandas


from ray.data._internal.pandas_block import PandasBlockAccessor
from ray.data._internal.util import _check_pyarrow_version
from ray.data.aggregate import _AggregateOnKeyBase
from ray.data._internal.null_aggregate import (_null_wrap_init,
                                               _null_wrap_merge,
                                               _null_wrap_accumulate_block,
                                               _null_wrap_finalize)
from typing import Optional
from ray.data.block import BlockAccessor


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


def atom_paillier_sum(items, pub_key, add_actors, limit=15):
    # less 'limit' will create more parallels
    # nums = items * limit
    if isinstance(items, pd.Series):
        items = items.values
    if len(items) < limit:
        return functools.reduce(lambda x, y: opt_paillier_add(pub_key, x, y),
                                items)

    items_list = [items[x:x + limit] for x in range(0, len(items), limit)]

    inter_results = list(
        add_actors.map(lambda a, v: a.add.remote(v), items_list))

    final_result = functools.reduce(
        lambda x, y: opt_paillier_add(pub_key, x, y), inter_results)
    return final_result


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

        if not encrypted:
            val = col.sum(skipna=ignore_nulls)
        else:
            val = atom_paillier_sum(col, pub_key, add_actors, limit=limit)
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
        if isinstance(block, pandas.DataFrame):
            return MyPandasBlockAccessor(block)

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


def search_best_splits(X: pd.DataFrame,
                       g,
                       h,
                       best_gain=0,
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

    best_gain = best_gain
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
            logging.info("current item is {} and split_point is {}".format(
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


class VGBTBase(BaseModel):

    def __init__(self, **kwargs):
        super().__init__(**kwargs)
        self.set_inputs()

    def loss(self, y_hat, y_true):
        if self.loss_type == 'log':
            y_prob = self.sigmoid(y_hat)

            return metrics.log_loss(y_true, y_prob)

        elif self.loss == "squarederror":
            return metrics.mean_squared_error(
                y_true, y_hat)  # mse don't activate inputs
        else:
            raise KeyError('The type is not implemented!')

    def get_summary(self):
        """
        """
        return {}

    def set_inputs(self):
        # set common parameters
        self.model = self.kwargs['common_params']['model']
        self.task_name = self.kwargs['common_params']['task_name']
        self.num_tree = self.kwargs['common_params']['num_tree']
        self.max_depth = self.kwargs['common_params']['max_depth']
        self.reg_lambda = self.kwargs['common_params']['reg_lambda']
        self.merge_gh = self.kwargs['common_params']['merge_gh']
        self.ray_group = self.kwargs['common_params']['ray_group']
        self.sample_type = self.kwargs['common_params']['sample_type']
        self.feature_sample = self.kwargs['common_params']['feature_sample']
        self.learning_rate = self.kwargs['common_params']['learning_rate']
        self.gamma = self.kwargs['common_params']['gamma']
        self.min_child_weight = self.kwargs['common_params']['min_child_weight']
        self.min_child_sample = self.kwargs['common_params']['min_child_sample']
        self.record = self.kwargs['common_params']['record']
        self.encrypted_proto = self.kwargs['common_params']['encrypted_proto']
        self.estimators = self.kwargs['common_params']['num_tree']
        self.samples_clip_size = self.kwargs['common_params'][
            'samples_clip_size']
        self.large_grads_ratio = self.kwargs['common_params'][
            'large_grads_ratio']
        self.small_grads_ratio = self.kwargs['common_params'][
            'small_grads_ratio']
        self.actors = int(self.kwargs['common_params']['actors'])

        cpus = int(ray.available_resources()['CPU']) - 1
        self.actors = min(self.actors, cpus)

        self.metric_path = self.kwargs['common_params']['metric_path']
        self.model_pred = self.kwargs['common_params']['model_pred']

        # set role special parameters
        self.role_params = self.kwargs['role_params']
        self.data_set = self.role_params['data_set']

        # set party node information
        self.node_info = self.kwargs['node_info']

        # set other parameters
        self.other_params = self.kwargs['other_params']

        # read from data path
        value = eval(self.other_params.party_datasets[
            self.other_params.party_name].data['data_set'])

        data_path = value['data_path']
        self.data = pd.read_csv(data_path)

        self.tree_structure = {}
        self.lookup_table_sum = {}
        self.lookup_table = {}
        self.host_record = 0
        self.guest_record = 0

    def run(self):
        pass

    def get_outputs(self):
        return dict()

    def get_status(self):
        return {}

    def preprocess(self):
        if self.selected_column is not None:
            self.data = self.data[self.selected_column]
        else:
            self.data = self.data

        if self.id is not None:
            self.data.pop(self.id)

        if self.label is not None:
            self.y = self.data.pop(self.label)
        else:
            self.y = (np.random.random(self.data.shape[0]) > 0.5).astype('int')


class VGBTHost(VGBTBase):

    def __init__(self, **kwargs):
        super().__init__(**kwargs)
        self.channel = GrpcServers(local_role=self.other_params.party_name,
                                   remote_roles=self.role_params['neighbors'],
                                   party_info=self.node_info,
                                   task_info=self.other_params.task_info)
        self.id = self.role_params['id']
        self.selected_column = self.role_params['selected_column']
        self.objective = self.role_params['objective']
        self.label = self.role_params['label']
        self.base_score = self.role_params['base_score']
        self.model_path = self.role_params['model_path']
        self.lookup_table_path = self.role_params['lookup_table']
        self.secure_bits = self.role_params['secure_bits']
        self.amplify_ratio = self.role_params['amplify_ratio']
        self.const = 2
        self.pub, self.prv = opt_paillier_keygen(self.secure_bits)

        self.channel.sender("pub", self.pub)

        self.encrypt_pool = ActorPool([
            PaillierActor.remote(self.prv, self.pub) for _ in range(self.actors)
        ])

    def gh_sums_decrypted_with_ray(self, gh_sums_dict, plain_gh_sums):
        sum_col = ['sum(g)', 'sum(h)']
        G_lefts = []
        G_rights = []
        H_lefts = []
        H_rights = []
        cuts = []
        vars = []
        if self.encrypt_pool is not None:
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
                    val['g'] = val['sum(g)'] // 10**self.amplify_ratio
                    val['h'] = val['sum(g)'] - val['g'] * 10**self.amplify_ratio
                    val['g'] = val['g'] / 10**4 - self.const * val['count()']
                    val['h'] = val['h'] / 10**4

                    tmp_g_lefts = val['g'].cumsum()
                    tmp_h_lefts = val['h'].cumsum()

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

                    cumsum_val = val.cumsum() / 10**self.amplify_ratio
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
        if self.encrypted_proto is not None:
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

            dec_gh_sums_df = pd.DataFrame(
                dec_gh_sums, columns=decrypted_items) / 10**self.amplify_ratio

        else:
            dec_gh_sums_df = gh_sums[decrypted_items]

        gh_sums['G_left'] = dec_gh_sums_df['G_left']
        # gh_sums['G_right'] = dec_gh_sums_df['G_right']
        gh_sums['G_right'] = plain_gh_sums['g'] - dec_gh_sums_df['G_left']
        gh_sums['H_left'] = dec_gh_sums_df['H_left']
        # gh_sums['H_right'] = dec_gh_sums_df['H_right']
        gh_sums['H_right'] = plain_gh_sums['h'] - dec_gh_sums_df['H_left']

        return gh_sums

    def predict_raw(self, X: pd.DataFrame, lookup):
        X = X.reset_index(drop='True')
        # Y = pd.Series([self.base_score] * X.shape[0])
        Y = np.array([self.base_score] * len(X))

        for t in range(self.estimators):
            tree = self.tree_structure[t + 1]
            lookup_table = lookup[t + 1]
            # y_t = pd.Series([0] * X.shape[0])
            y_t = np.zeros(len(X))
            #self._get_tree_node_w(X, tree, lookup_table, y_t, t)
            self.host_get_tree_node_weight(X, tree, lookup_table, y_t)
            Y = Y + self.learning_rate * y_t

        return Y

    def run(self):
        self.preprocess()
        self.fit()

    def train_metrics(self, y_true, y_hat):
        ks, auc = evaluate_ks_and_roc_auc(y_real=y_true, y_proba=y_hat)
        acc = metrics.accuracy_score(y_true, (y_hat >= 0.5).astype('int'))
        fpr, tpr, threshold = metrics.roc_curve(y_true, y_hat)
        recall = eval_acc(y_true, (y_hat >= 0.5).astype('int'))
        lifts, gains = plot_lift_and_gain(y_true, y_hat)
        trainMetrics = {
            "acc": acc,
            "auc": auc,
            "ks": ks,
            "fpr": fpr.tolist(),
            "tpr": tpr.tolist(),
            "lift_x": lifts['axis_x'].tolist(),
            "lift_y": lifts['axis_y'],
            "gain_x": gains['axis_x'].tolist(),
            "gain_y": gains['axis_y'],
            "recall": recall
        }
        print("ks, auc and acc", ks, auc, acc)
        return trainMetrics

    def predict_raw(self, X: pd.DataFrame, lookup):
        X = X.reset_index(drop='True')
        # Y = pd.Series([self.base_score] * X.shape[0])
        Y = np.array([self.base_score] * len(X))

        for t in range(self.estimators):
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

    def grad_and_hess(self, y_hat, y_true):
        if self.objective == "linear":
            grad = y_hat - y_true
            hess = np.array([1] * y_true.shape[0])
        elif self.objective == "logistic":
            y_hat = 1.0 / (1.0 + np.exp(-y_hat))
            grad = y_hat - y_true
            hess = y_hat * (1.0 - y_hat)
        else:
            raise ValueError('objective must be linear or logistic!')

        return grad, hess

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

    def gh_sampling(self, gh):
        m, n = gh.shape
        if m < self.samples_clip_size:
            return gh
        else:
            if self.sample_type == "random":
                sample_ids = random_sample(gh,
                                           top_rate=self.large_grads_ratio,
                                           other_rate=self.small_grads_ratio)
                return gh.iloc[sample_ids]
            elif self.sample_type == "goss":
                top_ids, low_ids = goss_sample(
                    gh,
                    top_rate=self.large_grads_ratio,
                    other_rate=self.small_grads_ratio)

                amply_rate = (1 -
                              self.large_grads_ratio) / self.small_grads_ratio

                gh['g'][low_ids] *= amply_rate
                sample_ids = top_ids + low_ids

                return gh.iloc[sample_ids]

            else:
                return gh

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
        # print("max_row: ", max_row)
        max_item = guest_gh_sums.iloc[max_row, :]

        max_gain = max_item['gain']

        if max_gain > best_gain:
            return max_item

        return None

    def host_tree_construct(self, X_host, f_t, current_depth, gh):
        m, n = X_host.shape

        if m < self.min_child_sample or current_depth > self.max_depth:
            return

        host_best = self.host_best_cut(X_host,
                                       cal_hist=True,
                                       bins=10,
                                       plain_gh=gh)

        plain_gh_sums = gh.sum(axis=0)

        guest_gh_sums = self.channel.recv('encrypte_gh_sums')[
            self.role_params['neighbors'][0]]

        if self.ray_group:
            dec_guest_gh_sums = self.gh_sums_decrypted_with_ray(
                guest_gh_sums, plain_gh_sums=plain_gh_sums)
        else:
            dec_guest_gh_sums = self.gh_sums_decrypted(guest_gh_sums,
                                                       plain_gh_sums)

        guest_best = self.guest_best_cut(dec_guest_gh_sums)

        self.channel.sender('best_cut', {
            'host_best': host_best,
            'guest_best': guest_best
        })

        logging.info(
            "current depth: {}, host best var: {} and guest best var: {}".
            format(current_depth, host_best, guest_best))

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

        if (host_best_gain or guest_best_gain):
            if flag_guest1 or flag_guest2:

                role = "guest"
                record = self.guest_record
                # ids_w = self.proxy_server.Get('ids_w')
                ids_w = self.channel.recv('ids_w')[self.role_params['neighbors']
                                                   [0]]

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
                # self.proxy_client_guest.Remote(
                #     {
                #         'id_left': id_left,
                #         'id_right': id_right,
                #         'w_left': w_left,
                #         'w_right': w_right
                #     }, 'ids_w')
                self.channel.sender(
                    'ids_w', {
                        'id_left': id_left,
                        'id_right': id_right,
                        'w_left': w_left,
                        'w_right': w_right
                    })

                self.lookup_table[self.host_record] = [
                    host_best['best_var'], host_best['best_cut']
                ]
                # print("self.lookup_table", self.lookup_table)

                self.host_record += 1

            tree_structure = {(role, record): {}}
            logging.info("current role: {}, current record: {}".format(
                role, record))

            print(
                "current role: {}, current record: {}, host_best_gain: {}, guest_best_gain: {}"
                .format(role, record, host_best_gain, guest_best_gain))

            X_host_left = X_host.loc[id_left]
            plain_gh_left = gh.loc[id_left]

            X_host_right = X_host.loc[id_right]
            plain_gh_right = gh.loc[id_right]

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
                # ids = self.proxy_server.Get(str(record_id) + '_ids')
                ids = self.channel.recv(str(record_id) + '_ids')[
                    self.role_params['neighbors'][0]]
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

                self.channel.sender(
                    str(record_id) + '_ids', {
                        'id_left': host_test_left.index,
                        'id_right': host_test_right.index
                    })
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

    def fit(self):
        y_hat = np.array([self.base_score] * len(self.y))
        losses = []
        self.channel.sender("xgb_pub", self.pub)

        for iter in range(self.estimators):
            f_t = pd.Series([0] * self.y.shape[0])

            # calculate grad and hess
            grad, hess = self.grad_and_hess(y_hat, self.y)

            grad_and_hess = pd.DataFrame({'g': grad, 'h': hess})

            # random sampling gh, data, y_hat, f_t
            sample_gh = self.gh_sampling(grad_and_hess)
            sample_inds = sample_gh.index.tolist()

            self.channel.sender("sample_ids", sample_inds)

            sample_data = self.data.iloc[sample_inds].copy()
            sample_y_hat = y_hat[sample_inds].copy()
            sample_f_t = f_t.iloc[sample_inds].copy()

            # choose the encoding type
            # if self.encrypted_proto == "paillier":
            if self.encrypted_proto is not None:
                # whether to merge grads and hess before encrypting
                if self.merge_gh:
                    cp_gh = sample_gh.copy()
                    cp_gh['g'] = cp_gh['g'] + self.const
                    cp_gh = np.round(cp_gh, 4)
                    cp_gh = (cp_gh * 10**4).astype('int')
                    merged_gh = cp_gh['g'] * 10**self.amplify_ratio + cp_gh['h']

                    encode_merged_gh = list(
                        self.encrypt_pool.map(lambda a, v: a.pai_enc.remote(v),
                                              merged_gh.values.tolist()))

                    enc_gh_df = pd.DataFrame({
                        'g': encode_merged_gh,
                        "id": sample_gh.index.tolist()
                    })

                    enc_gh_df.set_index('id', inplace=True)

                else:
                    flat_gh = sample_gh.values.flatten()
                    flat_gh *= 10**self.amplify_ratio
                    sample_gh_flat_int = flat_gh.astype('int')

                    encode_sample_gh_flat = list(
                        self.encrypt_pool.map(lambda a, v: a.pai_enc.remote(v),
                                              sample_gh_flat_int.tolist()))

                    enc_gh = np.array(encode_sample_gh_flat).reshape(-1, 2)
                    enc_gh_df = pd.DataFrame(enc_gh, columns=['g', 'h'])
                    enc_gh_df['id'] = sample_gh.index.tolist()
                    enc_gh_df.set_index('id', inplace=True)

                self.channel.sender("gh_en", enc_gh_df)

            else:
                self.channel.sender("gh_en", sample_gh)

            self.tree_structure[iter + 1] = self.host_tree_construct(
                sample_data, sample_f_t, 0, sample_gh)

            self.lookup_table_sum[iter + 1] = self.lookup_table

            y_hat[sample_inds] = y_hat[
                sample_inds] + self.learning_rate * sample_f_t

            current_loss = self.log_loss(self.y, 1 / (1 + np.exp(-y_hat)))
            losses.append(current_loss)
            print("current loss", current_loss)
            logging.info("Finish to trian tree {}.".format(iter + 1))

        # saving train metrics
        y_hat = self.predict_prob(self.data, lookup=self.lookup_table_sum)
        train_metrics = self.train_metrics(y_true=self.y, y_hat=y_hat)
        train_metrics_buff = json.dumps(train_metrics)
        with open(self.metric_path, 'w') as filePath:
            filePath.write(train_metrics_buff)

        with open(self.lookup_table_path, 'wb') as hostTable:
            pickle.dump(self.lookup_table_sum, hostTable)

        with open(self.model_path, 'wb') as hostModel:
            pickle.dump(
                {
                    'tree_struct': self.tree_structure,
                    'lr': self.learning_rate
                }, hostModel)


class VGBTGuest(VGBTBase):

    def __init__(self, **kwargs):
        super().__init__(**kwargs)
        self.channel = GrpcServers(local_role=self.other_params.party_name,
                                   remote_roles=self.role_params['neighbors'],
                                   party_info=self.node_info,
                                   task_info=self.other_params.task_info)
        self.id = self.role_params['id']
        self.selected_column = self.role_params['selected_column']
        self.label = self.role_params['label']
        self.model_path = self.role_params['model_path']
        self.lookup_table_path = self.role_params['lookup_table']
        self.batch_size = self.role_params['batch_size']
        self.pub = self.channel.recv("pub")[self.role_params['neighbors'][0]]
        self.add_pool = ActorPool(
            [ActorAdd.remote(self.pub) for _ in range(self.actors)])

    def run(self):
        self.preprocess()
        self.fit()

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

        if self.encrypted_proto is not None:

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
        internal_groups = []
        for tmp_col in cols:
            # print("=====tmp_col======", tmp_col)
            if self.merge_gh:
                tmp_group = buckets_x_guest.select_columns(
                    cols=[tmp_col, "g"]).groupby(tmp_col)
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

        if self.encrypted_proto is not None:
            internal_res = list(
                self.grouppools.map(lambda a, v: a.groupby.remote(v), groups))

            res = []
            for tmp_res in internal_res:
                res += tmp_res

        else:
            res = [
                tmp_group.sum(on=['g', 'h']).to_pandas()
                for tmp_group in internal_groups
            ]

        for key, tmp_task in zip(cols, res):
            # total_left_ghs[key] = tmp_task.get()
            total_left_ghs[key] = tmp_task

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

        if m < self.min_child_sample or current_depth > self.max_depth:
            return

        if self.ray_group:
            encrypte_gh_sums = self.sums_of_encrypted_ghs_with_ray(
                X_guest, encrypted_ghs)

        else:
            encrypte_gh_sums = self.sums_of_encrypted_ghs(
                X_guest, encrypted_ghs)

        self.channel.sender('encrypte_gh_sums', encrypte_gh_sums)
        best_cut = self.channel.recv('best_cut')[self.role_params['neighbors']
                                                 [0]]

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

                self.channel.sender(
                    'ids_w', {
                        'id_left': id_left,
                        'id_right': id_right,
                        'w_left': w_left,
                        'w_right': w_right
                    })
                # updata guest lookup table
                self.lookup_table[self.guest_record] = [best_var, best_cut]
                # self.guest_record += 1
                self.guest_record += 1

            else:
                # ids_w = self.proxy_server.Get('ids_w')
                ids_w = self.channel.recv('ids_w')[self.role_params['neighbors']
                                                   [0]]
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

    def fit(self):

        # receving public key from host
        self.pub = self.channel.recv("xgb_pub")[self.role_params['neighbors']
                                                [0]]

        # initialize adding actors on paillier encryption
        paillier_add_actors = ActorPool(
            [ActorAdd.remote(self.pub) for _ in range(self.actors)])

        if self.merge_gh:
            self.grouppools = ActorPool([
                GroupPool.remote(paillier_add_actors, self.pub, on_cols=['g'])
                for _ in range(self.actors)
            ])
        else:
            self.grouppools = ActorPool([
                GroupPool.remote(paillier_add_actors,
                                 self.pub,
                                 on_cols=['g', 'h']) for _ in range(self.actors)
            ])

        for t in range(self.estimators):
            sample_ids = self.channel.recv('sample_ids')[
                self.role_params['neighbors'][0]]
            sample_guest = self.data.iloc[sample_ids]
            gh_en = self.channel.recv('gh_en')[self.role_params['neighbors'][0]]

            self.tree_structure[t + 1] = self.guest_tree_construct(
                sample_guest, gh_en, 0)

            self.lookup_table_sum[t + 1] = self.lookup_table

        # save guest part model
        with open(self.model_path, 'wb') as guestModel:
            pickle.dump(
                {
                    'tree_struct': self.tree_structure,
                    'lr': self.learning_rate
                }, guestModel)

        # save guest part table
        with open(self.lookup_table_path, 'wb') as guestTable:
            pickle.dump(self.lookup_table_sum, guestTable)

        self.predict(self.data, self.lookup_table_sum)

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
                # self.proxy_client_host.Remote(
                #     {
                #         'id_left': id_left,
                #         'id_right': id_right
                #     },
                #     str(record_id) + '_ids')
                self.channel.sender(
                    str(record_id) + '_ids', {
                        'id_left': id_left,
                        'id_right': id_right
                    })

            else:
                # ids = self.proxy_server.Get(str(record_id) + '_ids')
                ids = self.channel.recv(str(record_id) + '_ids')[
                    self.role_params['neighbors'][0]]
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
        for t in range(self.estimators):
            tree = self.tree_structure[t + 1]
            current_lookup = lookup_sum[t + 1]
            self.guest_get_tree_ids(X, tree, current_lookup)
