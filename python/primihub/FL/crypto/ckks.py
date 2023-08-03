import tenseal as ts


class CKKS:

    def __init__(self, context):
        if isinstance(context, bytes):
            context = ts.context_from(context)
        self.context = context
        self.multiply_depth = context.data.seal_context().first_context_data().chain_index()

    def encrypt_vector(self, vector, context=None):
        if context:
            return ts.ckks_vector(context, vector)
        else:
            return ts.ckks_vector(self.context, vector)
    
    def encrypt_tensor(self, tensor, context=None):
        if context:
            return ts.ckks_tensor(context, tensor)
        else:
            return ts.ckks_tensor(self.context, tensor)
        
    def decrypt(self, ciphertext, secret_key=None):
        if ciphertext.context().has_secret_key():
            return ciphertext.decrypt()
        else:
            return ciphertext.decrypt(secret_key)
    
    def load_vector(self, vector):
        return ts.ckks_vector_from(self.context, vector)

    def load_tensor(self, tensor):
        return ts.ckks_tensor_from(self.context, tensor)