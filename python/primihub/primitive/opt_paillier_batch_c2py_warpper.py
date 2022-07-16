import opt_paillier_c2py


class Opt_paillier_batch_ciphertext(object):
    """
    Attributes:
        ciphertexts         list<string> <-> list<mpz_t>
        batch_size           int
        crtMod              dict         <-> CrtMod   
            crt_half_mod    list<string> <-> mpz_t[]
            crt_mod         list<string> <-> mpz_t[]
            crt_size        int          <-> size_t
            mod_size        int          <-> mp_bitcnt_t
    """

    def __init__(self):
        pass

    def __str__(self):
        return str(self.__dict__)


def opt_paillier_batch_encrypt_crt(pub, prv, plain_text_list, crt_mod=None):
    if not isinstance(plain_text_list, list):
        print("opt_paillier_encrypt plain_text should be type of int")
        return

    plain_text_list_str = []
    for plain_text in plain_text_list:
        plain_text_list_str.append(str(plain_text))

    batch_ciphertext = Opt_paillier_batch_ciphertext()
    opt_paillier_c2py.opt_paillier_batch_encrypt_crt_warpper(batch_ciphertext, pub, prv, plain_text_list_str, crt_mod)

    return batch_ciphertext


def opt_paillier_batch_decrypt_crt(pub, prv, batch_cipher_text):
    decrypt_text_list = opt_paillier_c2py.opt_paillier_batch_decrypt_crt_warpper(pub, prv, batch_cipher_text)

    decrypt_text_num_list = []
    for decrypt_text in decrypt_text_list:
        decrypt_text_num_list.append(int(decrypt_text))

    return decrypt_text_num_list


def opt_paillier_batch_add(pub, op1_batch_cipher_text, op2_batch_cipher_text):
    if op1_batch_cipher_text.batch_size != op2_batch_cipher_text.batch_size:
        print("two batch ciphertext is not in the same batch size")
        return
    if op1_batch_cipher_text.crtMod != op2_batch_cipher_text.crtMod:
        print("two batch ciphertext is not in the same batch crtmod")
        return

    add_res_cipher_text = Opt_paillier_batch_ciphertext()

    opt_paillier_c2py.opt_paillier_batch_add_warpper(add_res_cipher_text, op1_batch_cipher_text, op2_batch_cipher_text,
                                                     pub)

    return add_res_cipher_text
