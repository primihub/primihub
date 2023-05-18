import pandas as pd


def read_csv(data_path, selected_column, id='id'):
    data = pd.read_csv(data_path)
    if selected_column:
        data = data[selected_column]
    if id in data.columns:
        data.pop(id)
    return data

def read_image(img_dir, annotations_file, transform=None, target_transform=None):
    #Todo
    pass
