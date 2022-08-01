from python.primihub.primitive.opt_paillier_c2py_warpper import *
from python.primihub.primitive.opt_paillier_pack_c2py_warpper import *
import random
import time
from os import path
import pytest

def random_plaintext(length):
    res = 0
    for i in range(length - 1):
        res = res << 1
        res = res | random.randint(0, 1)
    if random.randint(0, 1) == 0:
         res = -res
    return res

def test_opt_paillier_pack_c2py():
    keygen_cost = 0
    keygen_st = time.time()
    pub, prv = opt_paillier_keygen(112)
    keygen_ed = time.time()
    keygen_cost += keygen_ed - keygen_st

    print("==================KeyGen is finished==================")
    print("KeyGen costs: " + str(keygen_cost * 1000.0) + " ms.")

    _round = 10
    pack_size = 1000
    encrypt_cost = 0
    decrypt_cost = 0
    add_cost = 0
    plain_text_bit_length = 70 - 1
    for i in range(_round):
        plain_text_list = []
        for j in range(pack_size):
            plain_text_list.append(random_plaintext(plain_text_bit_length))

        e_st = time.time()
        pack_cipher_text = opt_paillier_pack_encrypt_crt(pub, prv, plain_text_list)
        e_ed = time.time()
        encrypt_cost += e_ed - e_st

        d_st = time.time()
        pack_decrypt_text = opt_paillier_pack_decrypt_crt(pub, prv, pack_cipher_text)
        d_ed = time.time()
        decrypt_cost += d_ed - d_st

        for j in range(pack_size):
            assert pack_decrypt_text[j] == plain_text_list[j]

        plain_text_list2 = []
        for j in range(pack_size):
            plain_text_list2.append(random_plaintext(plain_text_bit_length))
        pack_cipher_text2 = opt_paillier_pack_encrypt(pub, plain_text_list2, pack_cipher_text.crtMod)

        a_st = time.time()
        pack_add_cipher_text = opt_paillier_pack_add(pub, pack_cipher_text, pack_cipher_text2)
        a_ed = time.time()
        add_cost += a_ed - a_st

        pack_add_decrypt_text = opt_paillier_pack_decrypt_crt(pub, prv, pack_add_cipher_text)
        
        for j in range(pack_size):
            assert pack_add_decrypt_text[j] == plain_text_list2[j] + plain_text_list[j]

    print("=========================opt test=========================")
    encrypt_cost = 1.0 / _round * encrypt_cost;
    decrypt_cost = 1.0 / _round * decrypt_cost;
    add_cost     = 1.0 / _round * add_cost;

    print("The avg encryption cost is " + str(encrypt_cost * 1000.0 ) + " ms.")
    print("The avg decryption cost is " + str(decrypt_cost * 1000.0 ) + " ms.")
    print("The avg addition   cost is " + str(add_cost     * 1000.0 ) + " ms.")

    print("========================================================")


if __name__ == '__main__':
    pytest.main(['-q', path.dirname(__file__)])