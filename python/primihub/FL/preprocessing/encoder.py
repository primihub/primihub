import numpy as np
import numbers
from sklearn.preprocessing import OneHotEncoder as SKL_OneHotEncoder
from sklearn.preprocessing import OrdinalEncoder as SKL_OrdinalEncoder
from sklearn.utils import is_scalar_nan
from sklearn.preprocessing._encoders import _BaseEncoder as _SKL_BaseEncoder
from .base import PreprocessBase
from .util import unique, RealNotInt


class _BaseEncoder(PreprocessBase, _SKL_BaseEncoder):

    def __init__(self,
                 categories="auto",
                 min_frequency=None,
                 max_categories=None,
                 FL_type=None,
                 role=None,
                 channel=None):
        super().__init__(FL_type, role, channel)
        if FL_type == 'H':
            self.categories = categories
            self.min_frequency = min_frequency
            self.max_categories = max_categories

    def _Hfit(self, X, ignore_missing_for_infrequent=False):
        infrequent_enabled = \
            check_infrequent_enabled(self.min_frequency, self.max_categories)
        self.module._infrequent_enabled = infrequent_enabled

        if self.role == 'client':
            if infrequent_enabled:
                return_counts = True
                min_frequency = self.min_frequency
                max_categories = self.max_categories
                self.min_frequency = None
                self.max_categories = None
            else:
                return_counts = False

            fit_results = self._fit(
                X,
                handle_unknown=self.module.handle_unknown,
                force_all_finite="allow-nan",
                return_counts=return_counts,
            )
            self.module.n_features_in_ = self.n_features_in_
            self.module.feature_names_in_ = self.feature_names_in_

            categories = self.categories_
            local_categories = categories
            if self.categories == 'auto':
                self.channel.send('categories', categories)
                categories = self.channel.recv('categories')
                self.categories_ = categories
            self.module.categories_ = categories

            missing_indices = {}
            if ignore_missing_for_infrequent:
                missing_indices = compute_missing_indices(categories)
                self.module._missing_indices = missing_indices

            if infrequent_enabled:
                if min_frequency is not None and isinstance(min_frequency, RealNotInt):
                    self.channel.send('n_samples', fit_results['n_samples'])
                
                if max_categories is None:
                    if isinstance(min_frequency, RealNotInt):
                        min_frequency = self.channel.recv('min_frequency')
                    min_freq_indices_counts = \
                        get_min_freq_indices_counts(min_frequency,
                                                    local_categories,
                                                    fit_results['category_counts'],
                                                    ignore_missing_for_infrequent=True)
                    self.channel.send('min_freq_indices_counts', min_freq_indices_counts)
                else:
                    self.channel.send('category_counts', fit_results['category_counts'])
                
                infrequent_indices = self.channel.recv('infrequent_indices')
                self._infrequent_indices = infrequent_indices
                self.module._infrequent_indices = infrequent_indices
                self.module._default_to_infrequent_mappings = \
                    compute_default_to_infrequent_mappings(categories, 
                                                           infrequent_indices, 
                                                           missing_indices)
        
        elif self.role == 'server':
            if self.categories == 'auto':
                client_categories = self.channel.recv_all('categories')
                categories = compute_category_union(client_categories)
                self.channel.send_all('categories', categories)
            else:
                categories = self.categories
            self.categories_ = categories
            self.module.categories_ = categories
            self.module.n_features_in_ = len(categories)

            missing_indices = {}
            if ignore_missing_for_infrequent:
                missing_indices = compute_missing_indices(categories)
                self.module._missing_indices = missing_indices
            
            if infrequent_enabled:
                if self.min_frequency is not None and isinstance(self.min_frequency, RealNotInt):
                    n_samples = self.channel.recv_all('n_samples')
                    n_samples = sum(n_samples)
                else:
                    n_samples = None
                
                if self.max_categories is None:
                    if isinstance(self.min_frequency, RealNotInt):
                        self.channel.send_all('min_frequency', self.min_frequency * n_samples)
                    min_freq_indices_counts = self.channel.recv_all('min_freq_indices_counts')
                    infrequent_indices = \
                        infreq_indices_for_min_freq(self.min_frequency,
                                                    categories,
                                                    client_categories,
                                                    min_freq_indices_counts)
                    self._infrequent_indices = infrequent_indices
                    self.module._default_to_infrequent_mappings = \
                        compute_default_to_infrequent_mappings(categories, 
                                                               infrequent_indices, 
                                                               missing_indices)
                else:
                    client_category_counts = self.channel.recv_all('category_counts')

                    if self.categories == 'auto':
                        category_counts = \
                            compute_count_histogram(categories,
                                                    client_categories,
                                                    client_category_counts)
                    else:
                        category_counts = np.sum(client_category_counts, axis=0)

                    self._fit_infrequent_category_mapping(
                        n_samples,
                        category_counts,
                        missing_indices,
                    )
                    infrequent_indices = self._infrequent_indices
                    self.module._default_to_infrequent_mappings = self._default_to_infrequent_mappings
                
                self.channel.send_all('infrequent_indices', infrequent_indices)
                self.module._infrequent_indices = infrequent_indices
        return missing_indices


class OneHotEncoder(_BaseEncoder):

    def __init__(self,
                 categories="auto",
                 drop=None,
                 sparse_output=True,
                 dtype=np.float64,
                 handle_unknown="error",
                 min_frequency=None,
                 max_categories=None,
                 feature_name_combiner="concat",
                 FL_type=None,
                 role=None,
                 channel=None):
        super().__init__(categories=categories,
                         min_frequency=min_frequency,
                         max_categories=max_categories,
                         FL_type=FL_type,
                         role=role,
                         channel=channel)
        self.module = SKL_OneHotEncoder(categories=categories,
                                        drop=drop,
                                        sparse_output=sparse_output,
                                        dtype=dtype,
                                        handle_unknown=handle_unknown,
                                        min_frequency=min_frequency,
                                        max_categories=max_categories,
                                        feature_name_combiner=feature_name_combiner)

    def Hfit(self, X):
        self._Hfit(X, ignore_missing_for_infrequent=False)
        self.module._set_drop_idx()
        self.module._n_features_outs = self.module._compute_n_features_outs()

        return self


class OrdinalEncoder(_BaseEncoder):

    def __init__(self,
                 categories="auto",
                 dtype=np.float64,
                 handle_unknown="error",
                 unknown_value=None,
                 encoded_missing_value=np.nan,
                 min_frequency=None,
                 max_categories=None,
                 FL_type=None,
                 role=None,
                 channel=None):
        super().__init__(categories=categories,
                         min_frequency=min_frequency,
                         max_categories=max_categories,
                         FL_type=FL_type,
                         role=role,
                         channel=channel)
        self.module = SKL_OrdinalEncoder(categories=categories,
                                         dtype=dtype,
                                         handle_unknown=handle_unknown,
                                         unknown_value=unknown_value,
                                         encoded_missing_value=encoded_missing_value,
                                         min_frequency=min_frequency,
                                         max_categories=max_categories)

    def Hfit(self, X):
        if self.module.handle_unknown == "use_encoded_value":
            if is_scalar_nan(self.module.unknown_value):
                if np.dtype(self.module.dtype).kind != "f":
                    raise ValueError(
                        "When unknown_value is np.nan, the dtype "
                        "parameter should be "
                        f"a float dtype. Got {self.module.dtype}."
                    )
            elif not isinstance(self.module.unknown_value, numbers.Integral):
                raise TypeError(
                    "unknown_value should be an integer or "
                    "np.nan when "
                    "handle_unknown is 'use_encoded_value', "
                    f"got {self.module.unknown_value}."
                )
        elif self.module.unknown_value is not None:
            raise TypeError(
                "unknown_value should only be set when "
                "handle_unknown is 'use_encoded_value', "
                f"got {self.module.unknown_value}."
            )
        
        missing_indices = self._Hfit(X, ignore_missing_for_infrequent=True)
            
        cardinalities = [len(cats) for cats in self.module.categories_]
        if self.module._infrequent_enabled:
            # Cardinality decreases because the infrequent categories are grouped
            # together
            for feature_idx, infrequent in enumerate(self.infrequent_categories_):
                if infrequent is not None:
                    cardinalities[feature_idx] -= len(infrequent)

        # missing values are not considered part of the cardinality
        # when considering unknown categories or encoded_missing_value
        for cat_idx, categories_for_idx in enumerate(self.module.categories_):
            for cat in categories_for_idx:
                if is_scalar_nan(cat):
                    cardinalities[cat_idx] -= 1
                    continue
        
        if self.module.handle_unknown == "use_encoded_value":
            for cardinality in cardinalities:
                if 0 <= self.module.unknown_value < cardinality:
                    raise ValueError(
                        "The used value for unknown_value "
                        f"{self.module.unknown_value} is one of the "
                        "values already used for encoding the "
                        "seen categories."
                    )

        if missing_indices:
            if np.dtype(self.module.dtype).kind != "f" and is_scalar_nan(
                self.module.encoded_missing_value
            ):
                raise ValueError(
                    "There are missing values in features "
                    f"{list(missing_indices)}. For OrdinalEncoder to "
                    f"encode missing values with dtype: {self.module.dtype}, set "
                    "encoded_missing_value to a non-nan value, or "
                    "set dtype to a float"
                )

            if not is_scalar_nan(self.module.encoded_missing_value):
                # Features are invalid when they contain a missing category
                # and encoded_missing_value was already used to encode a
                # known category
                invalid_features = [
                    cat_idx
                    for cat_idx, cardinality in enumerate(cardinalities)
                    if cat_idx in missing_indices
                    and 0 <= self.module.encoded_missing_value < cardinality
                ]

                if invalid_features:
                    # Use feature names if they are available
                    if hasattr(self, "feature_names_in_"):
                        invalid_features = self.feature_names_in_[invalid_features]
                    raise ValueError(
                        f"encoded_missing_value ({self.module.encoded_missing_value}) "
                        "is already used to encode a known category in features: "
                        f"{invalid_features}"
                    )

        return self
    

def check_infrequent_enabled(min_frequency, max_categories):
    if (max_categories is not None and max_categories >= 1
        ) or min_frequency is not None:
        return True
    else:
        return False


def compute_category_union(client_categories):
    categories = []
    for feature_idx in range(len(client_categories[0])):
        categories_for_idx = []
        for client_cat in client_categories:
            categories_for_idx.append(client_cat[feature_idx])
        categories_for_idx = np.concatenate(categories_for_idx)
        categories_for_idx = unique(categories_for_idx)
        categories.append(categories_for_idx)
    return categories


def get_min_freq_indices_counts(min_frequency,
                                categories,
                                category_counts,
                                ignore_missing_for_infrequent=False):
    
    missing_indices = {}
    if ignore_missing_for_infrequent:
        missing_indices = compute_missing_indices(categories)

    min_freq_indices_counts = []
    for col_idx, counts in enumerate(category_counts):
        min_freq_mask = counts < min_frequency
        indices = np.flatnonzero(min_freq_mask)
        if missing_indices and col_idx in missing_indices:
            indices = np.delete(indices, indices == missing_indices[col_idx])
        min_freq_indices_counts.append((indices, counts[indices]) \
                                        if indices.size > 0 else (None, None))
    return min_freq_indices_counts


def infreq_indices_for_min_freq(min_frequency,
                                categories,
                                client_categories,
                                min_freq_indices_counts):
    infrequent_indices = []
    for feature_idx in range(len(categories)):
        count_dict = {}
        for client_idx in range(len(client_categories)):
            indices, counts = min_freq_indices_counts[client_idx][feature_idx]
            if indices is not None:
                for i, cat in enumerate(client_categories[client_idx][feature_idx][indices]):
                    if cat in count_dict:
                        count_dict[cat] += counts[i]
                    else:
                        count_dict[cat] = counts[i]

        min_freq_indices = [np.where(categories[feature_idx]== k)[0][0] \
                            for k, v in count_dict.items() if v < min_frequency]
        if min_freq_indices:
            infrequent_indices.append(np.array(min_freq_indices))
        else:
            infrequent_indices.append(None)
    return infrequent_indices
                

def compute_count_histogram(categories,
                            client_categories,
                            client_category_counts):
    category_counts = []
    for feature_idx in range(len(categories)):
        categories_for_idx = categories[feature_idx]
        category_counts_for_idx = np.zeros(len(categories_for_idx))
        category_map = {}

        for client_idx, client_cat in enumerate(client_categories):
            idx_map = category_to_idx_map(
                client_cat[feature_idx],
                categories_for_idx,
                category_map
            )

            category_counts_for_idx[idx_map] += \
                client_category_counts[client_idx][feature_idx]
            
        category_counts.append(category_counts_for_idx)
    return category_counts


def category_to_idx_map(client_category, server_category, category_map):
    idx_map = []
    i = 0
    for ccat in client_category:
        if ccat in category_map:
            i = category_map[ccat]
            idx_map.append(i)
            continue
        while i < len(server_category):
            scat = server_category[i]
            if ccat == scat or (is_scalar_nan(ccat) and is_scalar_nan(scat)):
                idx_map.append(i)
                category_map[ccat] = i
                break
            i += 1
    return idx_map


def compute_default_to_infrequent_mappings(categories, 
                                           infrequent_indices, 
                                           missing_indices={}):
    # compute mapping from default mapping to infrequent mapping
    default_to_infrequent_mappings = []

    for feature_idx, infreq_idx in enumerate(infrequent_indices):
        cats = categories[feature_idx]
        # no infrequent categories
        if infreq_idx is None:
            default_to_infrequent_mappings.append(None)
            continue

        n_cats = len(cats)
        if feature_idx in missing_indices:
            # Missing index was removed from this category when computing
            # infrequent indices, thus we need to decrease the number of
            # total categories when considering the infrequent mapping.
            n_cats -= 1

        # infrequent indices exist
        mapping = np.empty(n_cats, dtype=np.int64)
        n_infrequent_cats = infreq_idx.size

        # infrequent categories are mapped to the last element.
        n_frequent_cats = n_cats - n_infrequent_cats
        mapping[infreq_idx] = n_frequent_cats

        frequent_indices = np.setdiff1d(np.arange(n_cats), infreq_idx)
        mapping[frequent_indices] = np.arange(n_frequent_cats)

        default_to_infrequent_mappings.append(mapping)
    return default_to_infrequent_mappings


def compute_missing_indices(categories):
    missing_indices = {}
    for feature_idx, categories_for_idx in enumerate(categories):
        for category_idx, category in enumerate(categories_for_idx):
            if is_scalar_nan(category):
                missing_indices[feature_idx] = category_idx
                break
    return missing_indices