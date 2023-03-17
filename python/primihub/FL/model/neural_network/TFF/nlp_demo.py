import tensorflow as tf
import tensorflow_federated as tff

import numpy as np
import collections

import os

# A fixed vocabularly of ASCII chars that occur in the works of Shakespeare and Dickens:
vocab = list(
    'dhlptx@DHLPTX $(,048cgkoswCGKOSW[_#\'/37;?bfjnrvzBFJNRVZ"&*.26:\naeimquyAEIMQUY]!%)-159\r'
)

# Creating a mapping from unique characters to indices
char2idx = {u: i for i, u in enumerate(vocab)}
idx2char = np.array(vocab)


def load_model(batch_size):
    urls = {
        1:
            'https://storage.googleapis.com/tff-models-public/dickens_rnn.batch1.kerasmodel',
        8:
            'https://storage.googleapis.com/tff-models-public/dickens_rnn.batch8.kerasmodel'
    }
    assert batch_size in urls, 'batch_size must be in ' + str(urls.keys())
    url = urls[batch_size]
    local_file = tf.keras.utils.get_file(os.path.basename(url), origin=url)
    return tf.keras.models.load_model(local_file, compile=False)


def generate_text(model, start_string):
    # From https://www.tensorflow.org/tutorials/sequences/text_generation
    num_generate = 200
    input_eval = [char2idx[s] for s in start_string]
    input_eval = tf.expand_dims(input_eval, 0)
    text_generated = []
    temperature = 1.0

    model.reset_states()
    for i in range(num_generate):
        predictions = model(input_eval)
        predictions = tf.squeeze(predictions, 0)
        predictions = predictions / temperature
        predicted_id = tf.random.categorical(predictions,
                                             num_samples=1)[-1, 0].numpy()
        input_eval = tf.expand_dims([predicted_id], 0)
        text_generated.append(idx2char[predicted_id])

    return (start_string + ''.join(text_generated))


# Text generation requires a batch_size=1 model.
keras_model_batch1 = load_model(batch_size=1)
print(
    generate_text(keras_model_batch1,
                  'What of TensorFlow Federated, you ask? '))

train_data, test_data = tff.simulation.datasets.shakespeare.load_data()

# Here the play is "The Tragedy of King Lear" and the character is "King".
raw_example_dataset = train_data.create_tf_dataset_for_client(
    'THE_TRAGEDY_OF_KING_LEAR_KING')

# Input pre-processing parameters
SEQ_LENGTH = 100
BATCH_SIZE = 8
BUFFER_SIZE = 100  # For dataset shuffling

# Construct a lookup table to map string chars to indexes,
# using the vocab loaded above:
table = tf.lookup.StaticHashTable(tf.lookup.KeyValueTensorInitializer(
    keys=vocab, values=tf.constant(list(range(len(vocab))), dtype=tf.int64)),
                                  default_value=0)


def to_ids(x):
    s = tf.reshape(x['snippets'], shape=[1])
    chars = tf.strings.bytes_split(s).values
    ids = table.lookup(chars)
    return ids


def split_input_target(chunk):
    input_text = tf.map_fn(lambda x: x[:-1], chunk)
    target_text = tf.map_fn(lambda x: x[1:], chunk)
    return (input_text, target_text)


def preprocess(dataset):
    return (
        # Map ASCII chars to int64 indexes using the vocab
        dataset.map(to_ids)
        # Split into individual chars
        .unbatch()
        # Form example sequences of SEQ_LENGTH +1
        .batch(SEQ_LENGTH + 1, drop_remainder=True)
        # Shuffle and form minibatches
        .shuffle(BUFFER_SIZE).batch(BATCH_SIZE, drop_remainder=True)
        # And finally split into (input, target) tuples,
        # each of length SEQ_LENGTH.
        .map(split_input_target))


example_dataset = preprocess(raw_example_dataset)


class FlattenedCategoricalAccuracy(tf.keras.metrics.SparseCategoricalAccuracy):

    def __init__(self, name='accuracy', dtype=tf.float32):
        super().__init__(name, dtype=dtype)

    def update_state(self, y_true, y_pred, sample_weight=None):
        y_true = tf.reshape(y_true, [-1, 1])
        y_pred = tf.reshape(y_pred, [-1, len(vocab), 1])
        return super().update_state(y_true, y_pred, sample_weight)


BATCH_SIZE = 8  # The training and eval batch size for the rest of this tutorial.
keras_model = load_model(batch_size=BATCH_SIZE)
keras_model.compile(
    loss=tf.keras.losses.SparseCategoricalCrossentropy(from_logits=True),
    metrics=[FlattenedCategoricalAccuracy()])

# Confirm that loss is much lower on Shakespeare than on random data
loss, accuracy = keras_model.evaluate(example_dataset.take(5), verbose=0)
print(
    'Evaluating on an example Shakespeare character: {a:3f}'.format(a=accuracy))

# As a sanity check, we can construct some completely random data, where we expect
# the accuracy to be essentially random:
random_guessed_accuracy = 1.0 / len(vocab)
print('Expected accuracy for random guessing: {a:.3f}'.format(
    a=random_guessed_accuracy))
random_indexes = np.random.randint(low=0,
                                   high=len(vocab),
                                   size=1 * BATCH_SIZE * (SEQ_LENGTH + 1))
data = collections.OrderedDict(snippets=tf.constant(
    ''.join(np.array(vocab)[random_indexes]), shape=[1, 1]))
random_dataset = preprocess(tf.data.Dataset.from_tensor_slices(data))
loss, accuracy = keras_model.evaluate(random_dataset, steps=10, verbose=0)
print('Evaluating on completely random data: {a:.3f}'.format(a=accuracy))


# Clone the keras_model inside `create_tff_model()`, which TFF will
# call to produce a new copy of the model inside the graph that it will
# serialize. Note: we want to construct all the necessary objects we'll need
# _inside_ this method.
def create_tff_model():
    # TFF uses an `input_spec` so it knows the types and shapes
    # that your model expects.
    input_spec = example_dataset.element_spec
    keras_model_clone = tf.keras.models.clone_model(keras_model)
    return tff.learning.from_keras_model(
        keras_model_clone,
        input_spec=input_spec,
        loss=tf.keras.losses.SparseCategoricalCrossentropy(from_logits=True),
        metrics=[FlattenedCategoricalAccuracy()])


# This command builds all the TensorFlow graphs and serializes them:
fed_avg = tff.learning.algorithms.build_weighted_fed_avg(
    model_fn=create_tff_model,
    client_optimizer_fn=lambda: tf.keras.optimizers.SGD(learning_rate=0.5))


def data(client, source=train_data):
    return preprocess(source.create_tf_dataset_for_client(client)).take(5)


clients = [
    'ALL_S_WELL_THAT_ENDS_WELL_CELIA',
    'MUCH_ADO_ABOUT_NOTHING_OTHELLO',
]

train_datasets = [data(client) for client in clients]

# We concatenate the test datasets for evaluation with Keras by creating a
# Dataset of Datasets, and then identity flat mapping across all the examples.
test_dataset = tf.data.Dataset.from_tensor_slices(
    [data(client, test_data) for client in clients]).flat_map(lambda x: x)

NUM_ROUNDS = 20

# The state of the FL server, containing the model and optimization state.
state = fed_avg.initialize()

# Load our pre-trained Keras model weights into the global model state.
pre_trained_weights = tff.learning.models.ModelWeights(
    trainable=[v.numpy() for v in keras_model.trainable_weights],
    non_trainable=[v.numpy() for v in keras_model.non_trainable_weights])
state = fed_avg.set_model_weights(state, pre_trained_weights)


def keras_evaluate(state, round_num):
    # Take our global model weights and push them back into a Keras model to
    # use its standard `.evaluate()` method.
    keras_model = load_model(batch_size=BATCH_SIZE)
    keras_model.compile(
        loss=tf.keras.losses.SparseCategoricalCrossentropy(from_logits=True),
        metrics=[FlattenedCategoricalAccuracy()])
    model_weights = fed_avg.get_model_weights(state)
    model_weights.assign_weights_to(keras_model)
    loss, accuracy = keras_model.evaluate(example_dataset, steps=2, verbose=0)
    print('\tEval: loss={l:.3f}, accuracy={a:.3f}'.format(l=loss, a=accuracy))


for round_num in range(NUM_ROUNDS):
    print('Round {r}'.format(r=round_num))
    keras_evaluate(state, round_num)
    result = fed_avg.next(state, train_datasets)
    state = result.state
    train_metrics = result.metrics['client_work']['train']
    print('\tTrain: loss={l:.3f}, accuracy={a:.3f}'.format(
        l=train_metrics['loss'], a=train_metrics['accuracy']))

print('Final evaluation')
keras_evaluate(state, NUM_ROUNDS + 1)

# Set our newly trained weights back in the originally created model.
keras_model_batch1.set_weights([v.numpy() for v in keras_model.weights])
# Text generation requires batch_size=1
print(
    generate_text(keras_model_batch1,
                  'What of TensorFlow Federated, you ask? '))

