import functools
import numpy as np
import pandas as pd
from sklearn.preprocessing import LabelEncoder
from primihub.FL.algorithm.utils.net_work import GrpcClient
from primihub.primitive.opt_paillier_c2py_warpper import opt_paillier_add, opt_paillier_keygen, opt_paillier_encrypt_crt, opt_paillier_decrypt_crt
from primihub.FL.algorithm.utils.base import BaseModel
from primihub.FL.algorithm.utils.dataset import read_csv
from primihub.utils.logger_util import logger


class IVBase(BaseModel):

    def __init__(self, **kwargs):
        super().__init__(**kwargs)
        self.set_inputs()

    def get_summary(self):
        """
        """
        return {}

    def set_inputs(self):
        # set common parameters
        self.model = self.common_params['model']
        self.task_name = self.common_params['task_name']
        self.threshold = self.common_params['threshold']
        self.bin_num = self.common_params['bin_num']
        self.bin_type = self.common_params['bin_type']
        self.security_length = self.common_params['security_length']

        # set role special parameters
        self.data_set = self.role_params['data_set']
        self.id = self.role_params['id']
        self.label = self.role_params['label']
        self.continuous_variables = self.role_params['continuous_variables']
        self.bin_dict = self.role_params['bin_dict']

        # read from data path
        data_path = self.role_params['data']['data_path']
        self.data = read_csv(data_path, selected_column=None, id=None)

        # set current channel
        remote_party = self.roles[self.role_params['others_role']][0]
        self.channel = GrpcClient(local_party=self.role_params['self_name'],
                                    remote_party=remote_party,
                                    node_info=self.node_info,
                                    task_info=self.task_info)

    def run(self):
        pass

    def get_outputs(self):
        return dict()

    def get_status(self):
        return {}

    def preprocess(self):
        if self.id is not None:
            self.data.pop(self.id)

        if self.label is not None:
            self.y = self.data.pop(self.label)
            enc = LabelEncoder()
            self.y = enc.fit_transform(self.y)

            self.data['label_0'] = (self.y == 0).astype('int')
            self.data['label_1'] = (self.y == 1).astype('int')

        # set category columns
        all_columns = self.data.columns
        self.category_feature = []
        for tmp_col in all_columns:
            if tmp_col in self.continuous_variables + ["label_0", "label_1"]:
                continue
            else:
                self.category_feature.append(tmp_col)

    def binning(self):
        self.bucket_col = []

        for cur_feature in self.continuous_variables:
            if cur_feature in self.data.columns:
                cur_feature_bins = self.bin_dict.get(cur_feature, None)
                if cur_feature_bins is not None:

                    # bucket-binning according to pre-set bins

                    if self.bin_type == "equal_size":
                        bin_bucket = pd.qcut(self.data[cur_feature],
                                             cur_feature_bins,
                                             labels=False,
                                             duplicates='drop')

                    else:
                        bin_bucket = pd.cut(self.data[cur_feature],
                                            bins=cur_feature_bins,
                                            labels=np.arange(
                                                len(cur_feature_bins)))

                    self.data[cur_feature + "_cate"] = bin_bucket

                else:
                    # bucket-binning according to cutting numbers

                    if self.bin_type == "equal_size":
                        bin_bucket = pd.qcut(self.data[cur_feature],
                                             self.bin_num,
                                             labels=False,
                                             duplicates='drop')
                    else:
                        bin_bucket = pd.cut(self.data[cur_feature],
                                            bins=self.bin_num,
                                            labels=np.arange(self.bin_num))
                    self.data[cur_feature + "_cate"] = bin_bucket

                self.bucket_col.append(cur_feature + "_cate")
            else:
                continue

    def filter_features(self, iv_dict=None):
        filtered_features = []

        if iv_dict is not None:
            for key, val in iv_dict.items():
                if val < self.threshold:
                    continue
                else:
                    key = key.replace("_cate", "")
                    filtered_features.append(key)
        return filtered_features


class IVHost(IVBase):

    def __init__(self, **kwargs):
        super().__init__(**kwargs)
        self.out_file = self.role_params['out_file']
        self.binning()
        self.preprocess()
        # map continuous features into discrete buckets
        self.public_key, self.private_key = opt_paillier_keygen(
            self.security_length)

        self.channel.send("pub_key", self.public_key)

    def cal_iv(self):
        all_cates = self.category_feature + self.bucket_col

        iv_dict = dict()

        for dim in all_cates:
            woe_iv_df_0 = self.data.groupby(dim).agg({
                'label_0': 'sum'
            }).reset_index()
            woe_iv_df_1 = self.data.groupby(dim).agg({
                'label_1': 'sum'
            }).reset_index()

            woe_iv_df = pd.merge(woe_iv_df_0, woe_iv_df_1, how="left", on=dim)

            woe_iv_df['label_0'] += 1
            woe_iv_df['label_1'] += 1

            woe_iv_df[
                'ratio_0'] = woe_iv_df['label_0'] / woe_iv_df['label_0'].sum()
            woe_iv_df[
                'ratio_1'] = woe_iv_df['label_1'] / woe_iv_df['label_1'].sum()

            woe_iv_df['woe'] = np.log(woe_iv_df['ratio_0'] /
                                      woe_iv_df['ratio_1'])

            woe_iv_df['iv'] = (woe_iv_df['ratio_0'] -
                               woe_iv_df['ratio_1']) * woe_iv_df['woe']
            iv = round(woe_iv_df['iv'].sum(), 4)
            iv_dict[dim] = iv

        return iv_dict

    def run(self):
        iv_dict = self.cal_iv()
        print("host iv_dict: ", iv_dict)
        filter_features = self.filter_features(iv_dict)

        if self.out_file is not None:
            filtered_data = self.data[filter_features]
            filtered_data[self.label] = self.y
            filtered_data.to_csv(self.out_file, index=False, sep='\t')

        enc_label_0 = opt_paillier_encrypt_crt(self.public_key,
                                               self.private_key, 0)
        enc_label_1 = opt_paillier_encrypt_crt(self.public_key,
                                               self.private_key, 1)

        enc_label_dict = {0: enc_label_0, 1: enc_label_1}

        enc_label = filtered_data[self.label].apply(
            lambda x: enc_label_dict[int(x)])
        self.channel.send("enc_label", enc_label)

        # receiving guest 'iv_dict'
        iv_dict_counter = self.channel.recv("iv_dict_counter")

        guest_ivs = {}

        for key, val in iv_dict_counter.items():
            table_count = None
            table_names = []
            label_0 = []
            label_1 = []

            for item, count in val.items():
                table_names.append(item)

                decrypt_label_1 = opt_paillier_decrypt_crt(
                    self.public_key, self.private_key, count['positive_count'])
                label_1.append(decrypt_label_1)
                label_0.append(count['total_count'] - decrypt_label_1)

            table_count = pd.DataFrame(data=None,
                                       columns=[key, "label_0", "label_1"])
            table_count[key] = table_names
            table_count["label_0"] = label_0
            table_count['label_1'] = label_1
            table_count["label_0"] += 1
            table_count["label_1"] += 1

            # table_count['cnt'] = table_count["label_0"] + table_count["label_1"]

            table_count['ratio_0'] = table_count["label_0"] / table_count[
                "label_0"].sum()
            table_count['ratio_1'] = table_count["label_1"] / table_count[
                "label_1"].sum()

            table_count['woe'] = np.log(table_count['ratio_0'] /
                                        table_count['ratio_1'])
            table_count['iv'] = (table_count['ratio_0'] -
                                 table_count['ratio_1']) * table_count['woe']

            iv = round(table_count['iv'].sum(), 4)
            guest_ivs[key] = iv

        self.channel.send("guest_ivs", guest_ivs)


class IVGuest(IVBase):

    def __init__(self, **kwargs):
        super().__init__(**kwargs)
        self.out_file = self.role_params['out_file']
        self.binning()

        self.preprocess()
        # map continuous features into discrete buckets
        self.pub_key = self.channel.recv("pub_key")

    def cal_iv(self):
        all_cates = self.category_feature + self.bucket_col
        iv_dict_counter = dict()
        encrypted_label = self.channel.recv("enc_label")

        if encrypted_label is None:
            raise ValueError(f"The {encrypted_label} should not be None!")
        else:
            for dim in all_cates:
                cur_col = self.data[dim]
                items = np.unique(cur_col)
                tmp_item_counter = {}

                for tmp_item in items:
                    tmp_label = encrypted_label[cur_col == tmp_item]
                    tmp_total_count = len(tmp_label)
                    tmp_positive_count = functools.reduce(
                        lambda x, y: opt_paillier_add(self.pub_key, x, y),
                        tmp_label)
                    tmp_item_counter[tmp_item] = {}
                    tmp_item_counter[tmp_item]["total_count"] = tmp_total_count
                    tmp_item_counter[tmp_item][
                        "positive_count"] = tmp_positive_count  # encrypted number

                iv_dict_counter[dim] = tmp_item_counter
                print("guest tmp_item_counter", tmp_item_counter)

        return iv_dict_counter

    def run(self):
        iv_dict_counter = self.cal_iv()
        self.channel.send("iv_dict_counter", iv_dict_counter)

        guest_ivs = self.channel.recv("guest_ivs")
        print("guest_ivs: ", guest_ivs)
        filtered_features = self.filter_features(guest_ivs)

        if self.out_file is not None:
            filtered_data = self.data[filtered_features]
            filtered_data.to_csv(self.out_file, index=False, sep='\t')
