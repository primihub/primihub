import random
import time

from python.primihub.primitive.opt_paillier_batch_c2py_warpper import *


def random_plaintext(length):
    res = 0
    for i in range(length - 1):
        res = res << 1
        res = res | random.randint(0, 1)
    # if random.randint(0, 1) == 0:
    #      res = -res
    return res


def local_crtMod():
    crt_mod = {'crt_half_mod': ['2VFp3a0BL8', '2VFp3a0BLT', '2VFp3a0BLn', '2VFp3a0BLu', '2VFp3a0BLw', '2VFp3a0BM5',
                                '2VFp3a0BMR', '2VFp3a0BNX', '2VFp3a0BNg', '2VFp3a0BOA', '2VFp3a0BOB', '2VFp3a0BOQ',
                                '2VFp3a0BOt', '2VFp3a0BOz', '2VFp3a0BQL', '2VFp3a0BQX', '2VFp3a0BQp', '2VFp3a0BQs',
                                '2VFp3a0BRH', '2VFp3a0BRK', '2VFp3a0BRx', '2VFp3a0BSN', '2VFp3a0BTU', '2VFp3a0BTq',
                                '2VFp3a0BTy', '2VFp3a0BU5', '2VFp3a0BUG', '2VFp3a0BVB'],
               'crt_mod': ['50Ve7A0MgH', '50Ve7A0Mgx', '50Ve7A0Mhb', '50Ve7A0Mhp', '50Ve7A0Mht', '50Ve7A0MiB',
                           '50Ve7A0Mit', '50Ve7A0Ml5', '50Ve7A0MlN', '50Ve7A0MmL', '50Ve7A0MmN', '50Ve7A0Mmr',
                           '50Ve7A0Mnn', '50Ve7A0Mnz', '50Ve7A0Mqh', '50Ve7A0Mr5', '50Ve7A0Mrf', '50Ve7A0Mrl',
                           '50Ve7A0MsZ', '50Ve7A0Msf', '50Ve7A0Mtv', '50Ve7A0Mul', '50Ve7A0Mwz', '50Ve7A0Mxh',
                           '50Ve7A0Mxx', '50Ve7A0MyB', '50Ve7A0MyX', '50Ve7A0N0N'], 'crt_size': 28, 'mod_size': 70}
    return crt_mod


if __name__ == '__main__':
    keygen_cost = 0
    keygen_st = time.time()
    pub, prv = opt_paillier_keygen(112)
    keygen_ed = time.time()
    keygen_cost += keygen_ed - keygen_st

    print("==================KeyGen is finished==================")
    print("KeyGen costs: " + str(keygen_cost * 1000.0) + " ms.")

    _round = 1
    batch_size = 100
    encrypt_cost = 0
    decrypt_cost = 0
    add_cost = 0
    for i in range(_round):
        plain_text_list = []
        for j in range(batch_size):
            plain_text_list.append(random_plaintext(40))
            # plain_text_list.append(-1048096593770162930)

        e_st = time.time()
        batch_cipher_text = opt_paillier_batch_encrypt_crt(pub, prv, plain_text_list)
        e_ed = time.time()
        encrypt_cost += e_ed - e_st

        d_st = time.time()
        batch_decrypt_text = opt_paillier_batch_decrypt_crt(pub, prv, batch_cipher_text)
        d_ed = time.time()
        decrypt_cost += d_ed - d_st

        for j in range(batch_size):
            if batch_decrypt_text[j] != plain_text_list[j]:
                # print(plain_text_list[j])
                # print(str(batch_cipher_text.crtMod))
                # print(batch_decrypt_text[j])
                print("Encrypt Error")

        plain_text_list2 = []
        for j in range(batch_size):
            plain_text_list2.append(random_plaintext(40))
        batch_cipher_text2 = opt_paillier_batch_encrypt_crt(pub, prv, plain_text_list, batch_cipher_text.crtMod)

        a_st = time.time()
        batch_add_cipher_text = opt_paillier_batch_add(pub, batch_cipher_text, batch_cipher_text2)
        a_ed = time.time()
        add_cost += a_ed - a_st

        batch_add_decrypt_text = opt_paillier_batch_decrypt_crt(pub, prv, batch_add_cipher_text)

        for j in range(batch_size):
            if batch_add_decrypt_text[j] != plain_text_list2[j] + plain_text_list[j]:
                print(batch_add_decrypt_text[j])
                print(plain_text_list2[j])
                print(plain_text_list[j])
                print("Add Error")

    print("=========================opt test=========================")
    encrypt_cost = 1.0 / _round * encrypt_cost;
    decrypt_cost = 1.0 / _round * decrypt_cost;
    add_cost = 1.0 / _round * add_cost;

    print("The total encryption cost is " + str(encrypt_cost * 1000.0) + " ms.")
    print("The total decryption cost is " + str(decrypt_cost * 1000.0) + " ms.")
    print("The total addition   cost is " + str(add_cost * 1000.0) + " ms.")

    print("========================================================")
