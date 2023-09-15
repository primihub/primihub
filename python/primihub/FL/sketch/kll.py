from datasketches import (kll_ints_sketch,
                          kll_floats_sketch,
                          kll_doubles_sketch,
                          kll_items_sketch,
                          vector_of_kll_floats_sketches,
                          vector_of_kll_ints_sketches,)
from primihub.utils.logger_util import logger


def send_local_kll_sketch(X, channel, vector=True, type='float', k=200):
    sketch = select_kll_sketch(vector, type)
    if vector:
        kll = sketch(k=k, d=X.shape[1])
    else:
        kll = sketch(k=k)
    kll.update(X)
    channel.send('kll_sketch', kll.serialize())


def merge_client_kll_sketch(channel, vector=True, type='float', k=200):
    client_sketch = channel.recv_all('kll_sketch')
    sketch = select_kll_sketch(vector, type)

    if vector:
        d = len(client_sketch[0])
        sk = sketch(k=k, d=d)
    else:
        sk = sketch(k=k)

    for i in range(len(client_sketch)):
        if vector:
            csk = sketch(k=k, d=d)
            for s in range(d):
                csk.deserialize(client_sketch[i][s], s)
        else:
            csk = sketch(k=k)
            csk.deserialize(client_sketch[i])
        
        sk.merge(csk)

    return sk


def select_kll_sketch(vector=True, type='float'):
    if vector:
        if type == 'float':
            sketch = vector_of_kll_floats_sketches
        elif type == 'int':
            sketch = vector_of_kll_ints_sketches
        else:
            error_msg = f'Unsupported kll sketch: vector {vector}, type {type}'
            logger.error(error_msg)
            raise ValueError(error_msg)
    else:
        if type == 'float':
            sketch = kll_floats_sketch
        elif type == 'int':
            sketch = kll_ints_sketch
        elif type == 'double':
            sketch = kll_doubles_sketch
        elif type == 'item':
            sketch = kll_items_sketch
        else:
            error_msg = f'Unsupported kll sketch: vector {vector}, type {type}'
            logger.error(error_msg)
            raise ValueError(error_msg)
        
    return sketch