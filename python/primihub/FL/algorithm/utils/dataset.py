import numpy as np
from numpy.random import default_rng
import pandas as pd


def read_csv(data_path, selected_column=None, id=None):
    data = pd.read_csv(data_path)
    if selected_column:
        data = data[selected_column]
    if id in data.columns:
        data.pop(id)
    return data

def read_image(img_dir, annotations_file, transform=None, target_transform=None):
    #Todo
    pass


class DataLoader:

    def __init__(self, dataset, label=None, batch_size=1, shuffle=True):
        self.dataset = dataset
        self.label = label
        self.batch_size = batch_size
        self.shuffle = shuffle
        self.n_samples = self.__len__()
        self.indices = np.arange(self.n_samples)
        self.start = 0

    def __len__(self):
        return len(self.dataset)

    def __getitem__(self, idx):
        return self.dataset[idx]

    def __iter__(self):
        return self

    def __next__(self):
        if self.start == 0:
            if self.shuffle:
                np.random.shuffle(self.indices)

        start = self.start
        end = start + self.batch_size
        if end > self.n_samples:
            end = self.n_samples
        self.start += self.batch_size

        if start < self.n_samples:
            batch_idx = self.indices[start:end]
            if self.label is not None:
                return self.dataset[batch_idx], self.label[batch_idx]
            else:
                return self.dataset[batch_idx]
        else:
            self.start = 0
            raise StopIteration


class DPDataLoader(DataLoader):

    def __init__(self, dataset, label=None, batch_size=1):
        self.dataset = dataset
        self.label = label
        self.batch_size = batch_size
        self.rng = default_rng()
        self.n_samples = self.__len__()
        self.max_iter = self.n_samples // self.batch_size
        self.num_iter = 0

    def __next__(self):
        self.num_iter += 1
        if self.num_iter <= self.max_iter:
            batch_idx = self.rng.choice(self.n_samples,
                                        self.batch_size,
                                        replace=False)
            if self.label is not None:
                return self.dataset[batch_idx], self.label[batch_idx]
            else:
                return self.dataset[batch_idx]
        else:
            self.num_iter = 0
            raise StopIteration