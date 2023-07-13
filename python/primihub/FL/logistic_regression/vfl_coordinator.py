from primihub.FL.utils.net_work import GrpcClient, MultiGrpcClients
from primihub.FL.utils.base import BaseModel
from primihub.utils.logger_util import logger

import math
import numpy as np
import tenseal as ts


class LogisticRegressionCoordinator(BaseModel):

    def __init__(self, **kwargs):
        super().__init__(**kwargs)

    def run(self):
        process = self.common_params['process']
        logger.info(f"process: {process}")
        if process == 'train':
            self.train()
        else:
            error_msg = f"Unsupported process: {process}"
            logger.error(error_msg)
            raise RuntimeError(error_msg)

    def train(self):
        # setup communication channels
        host_channel = GrpcClient(local_party=self.role_params['self_name'],
                                  remote_party=self.roles['host'],
                                  node_info=self.node_info,
                                  task_info=self.task_info)
        guest_channel = MultiGrpcClients(local_party=self.role_params['self_name'],
                                         remote_parties=self.roles['guest'],
                                         node_info=self.node_info,
                                         task_info=self.task_info)
        
        # coordinator init
        method = self.common_params['method']
        if method == 'CKKS':
            batch_size = host_channel.recv('batch_size')
            coordinator = CKKSCoordinator(batch_size,
                                          host_channel,
                                          guest_channel)
        else:
            error_msg = f"Unsupported method: {method}"
            logger.error(error_msg)
            raise RuntimeError(error_msg)

        # coordinator training
        logger.info("-------- start training --------")
        epoch = self.common_params['epoch']
        for i in range(epoch):
            logger.info(f"-------- epoch {i+1} / {epoch} --------")
            coordinator.train()
                
            # print metrics
            if self.common_params['print_metrics']:
                coordinator.compute_loss()
        logger.info("-------- finish training --------")

        # decrypt & send plaintext model
        coordinator.update_plaintext_model()


class CKKS:

    def __init__(self, context):
        if isinstance(context, bytes):
            context = ts.context_from(context)
        self.context = context

    def encrypt_vector(self, vector, context=None):
        if context:
            return ts.ckks_vector(context, vector)
        else:
            return ts.ckks_vector(self.context, vector)
    
    def decrypt(self, vector, secret_key=None):
        if vector.context().has_secret_key():
            return vector.decrypt()
        else:
            return vector.decrypt(secret_key)
    
    def load_vector(self, vector):
        return ts.ckks_vector_from(self.context, vector)


class CKKSCoordinator(CKKS):

    def __init__(self, batch_size, host_channel, guest_channel):
        self.t = 0
        self.host_channel = host_channel
        self.guest_channel = guest_channel

        # set CKKS params
        # use larger poly_mod_degree to support more encrypted multiplications
        poly_mod_degree = 32768
        # gradient descent uses as least two multiplications per interation
        multiply_per_iter = 2
        # more multiplications lead to larger context size
        self.max_iter = 7
        multiply_depth = multiply_per_iter * self.max_iter
        # sum(coeff_mod_bit_sizes) <= max coeff_modulus bit-length
        fe_bits_scale = 35
        bits_scale = 27
        # 35*2 + 27*2*7 = 448 < 881 (for N = 32768 & 128 bit security)
        coeff_mod_bit_sizes = [fe_bits_scale] + \
                              [bits_scale] * multiply_depth + \
                              [fe_bits_scale]

        # create TenSEALContext
        logger.info('create CKKS TenSEAL context')
        secret_context = ts.context(ts.SCHEME_TYPE.CKKS,
                                    poly_modulus_degree=poly_mod_degree,
                                    coeff_mod_bit_sizes=coeff_mod_bit_sizes)
        secret_context.global_scale = pow(2, bits_scale)
        secret_context.generate_galois_keys()

        context = secret_context.copy()
        context.make_context_public()

        super().__init__(context)
        self.secret_context = secret_context

        self.send_public_context()
        self.send_max_iter()
        num_examples = host_channel.recv('num_examples')

        self.iter_per_epoch = math.ceil(num_examples / batch_size)

    def send_max_iter(self):
        self.host_channel.send("max_iter", self.max_iter)
        self.guest_channel.send_all("max_iter", self.max_iter)

    def send_public_context(self):
        serialize_context = self.context.serialize()
        self.host_channel.send("public_context", serialize_context)
        self.guest_channel.send_all("public_context", serialize_context)

    def recv_model(self):
        host_weight = self.load_vector(self.host_channel.recv('host_weight'))
        host_bias = self.load_vector(self.host_channel.recv('host_bias'))

        guest_weight = self.guest_channel.recv_all('guest_weight')
        guest_weight = [self.load_vector(weight) for weight in guest_weight]

        return host_weight, host_bias, guest_weight
    
    def send_model(self, host_weight, host_bias, guest_weight):
        self.host_channel.send('host_weight', host_weight)
        self.host_channel.send('host_bias', host_bias)
        # send n sub-lists to n parties seperately
        self.guest_channel.send_seperately('guest_weight', guest_weight)

    def decrypt_model(self, host_weight, host_bias, guest_weight):
        host_weight = self.decrypt(host_weight, self.secret_context.secret_key())
        host_bias = self.decrypt(host_bias, self.secret_context.secret_key())

        guest_weight = [self.decrypt(weight, self.secret_context.secret_key()) \
                        for weight in guest_weight]
        
        return host_weight, host_bias, guest_weight
    
    def encrypt_model(self, host_weight, host_bias, guest_weight):
        host_weight = self.encrypt_vector(host_weight)
        host_bias = self.encrypt_vector(host_bias)

        guest_weight = [self.encrypt_vector(weight) for weight in guest_weight]
        
        return host_weight, host_bias, guest_weight

    def update_ciphertext_model(self):
        host_weight, host_bias, guest_weight = self.recv_model()
        host_weight, host_bias, guest_weight = self.decrypt_model(
            host_weight, host_bias, guest_weight)
        host_weight, host_bias, guest_weight = self.encrypt_model(
            host_weight, host_bias, guest_weight)
        
        host_weight = host_weight.serialize()
        host_bias = host_bias.serialize()
        guest_weight = [weight.serialize() for weight in guest_weight]

        self.send_model(host_weight, host_bias, guest_weight)

    def update_plaintext_model(self):
        host_weight, host_bias, guest_weight = self.recv_model()
        host_weight, host_bias, guest_weight = self.decrypt_model(
            host_weight, host_bias, guest_weight)
        
        # list to numpy ndarrry
        host_weight = np.array(host_weight)
        host_bias = np.array(host_bias)
        guest_weight = [np.array(weight) for weight in guest_weight]

        self.send_model(host_weight, host_bias, guest_weight)

    def train(self):
        logger.info(f'iteration {self.t} / {self.max_iter}')
        self.t += self.iter_per_epoch
        for i in range(self.t // self.max_iter):
            self.update_ciphertext_model()
            logger.warning(f'decrypt model #{i+1}')
        self.t = self.t % self.max_iter

    def compute_loss(self):
        logger.info(f'iteration {self.t} / {self.max_iter}')
        self.t += 1
        if self.t > self.max_iter:
            self.t = 0
            self.update_ciphertext_model()
            logger.warning('decrypt model')

        loss = self.load_vector(self.host_channel.recv('loss'))
        loss = self.decrypt(loss, self.secret_context.secret_key())[0]
        logger.info(f'loss={loss}')