import opt_paillier_c2py

class Opt_paillier_pack_ciphertext(object):
    """
    Attributes:
        ciphertexts         list<string> <-> list<mpz_t>
        pack_size           int
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

def opt_paillier_pack_encrypt_crt(pub, prv, plain_text_list, crt_mod = None):

    if not isinstance (plain_text_list, list):
        print("opt_paillier_pack_encrypt plain_texts should be type of list")
        return

    plain_text_list_str = []
    for plain_text in plain_text_list:
        if not isinstance (plain_text, int):
            print("opt_paillier_pack_encrypt plain_text should be type of int")
            return
        plain_text_list_str.append(str(plain_text))

    pack_ciphertext = Opt_paillier_pack_ciphertext()

    opt_paillier_c2py.opt_paillier_pack_encrypt_crt_warpper(pack_ciphertext, pub, prv, plain_text_list_str, crt_mod)
    
    return pack_ciphertext

def opt_paillier_pack_encrypt(pub, plain_text_list, crt_mod = None):

    if not isinstance (plain_text_list, list):
        print("opt_paillier_pack_encrypt plain_texts should be type of list")
        return

    plain_text_list_str = []
    for plain_text in plain_text_list:
        if not isinstance (plain_text, int):
            print("opt_paillier_pack_encrypt plain_text should be type of int")
            return
        plain_text_list_str.append(str(plain_text))

    pack_ciphertext = Opt_paillier_pack_ciphertext()
    
    opt_paillier_c2py.opt_paillier_pack_encrypt_warpper(pack_ciphertext, pub, plain_text_list_str, crt_mod)
    
    return pack_ciphertext

def opt_paillier_pack_decrypt_crt(pub, prv, pack_cipher_text):

    if not isinstance (pack_cipher_text, Opt_paillier_pack_ciphertext):
        print("opt_paillier_pack_decrypt pack_cipher_text should be type of Opt_paillier_pack_ciphertext()")
        return

    decrypt_text_list = opt_paillier_c2py.opt_paillier_pack_decrypt_crt_warpper(pub, prv, pack_cipher_text)

    decrypt_text_num_list = []
    for decrypt_text in decrypt_text_list:
        decrypt_text_num_list.append(int(decrypt_text))

    return decrypt_text_num_list

def opt_paillier_pack_add(pub, op1_pack_cipher_text, op2_pack_cipher_text):

    if not isinstance (op1_pack_cipher_text, Opt_paillier_pack_ciphertext):
        print("opt_paillier_pack_add op1_pack_cipher_text should be type of Opt_paillier_pack_ciphertext()")
        return
    if not isinstance (op2_pack_cipher_text, Opt_paillier_pack_ciphertext):
        print("opt_paillier_pack_add op2_pack_cipher_text should be type of Opt_paillier_pack_ciphertext()")
        return
    if op1_pack_cipher_text.pack_size != op2_pack_cipher_text.pack_size:
        print("two pack ciphertext is not in the same pack size")
        return
    if op1_pack_cipher_text.crtMod != op2_pack_cipher_text.crtMod:
        print("two pack ciphertext is not in the same pack crtmod")
        return

    add_res_cipher_text = Opt_paillier_pack_ciphertext()

    opt_paillier_c2py.opt_paillier_pack_add_warpper(add_res_cipher_text, op1_pack_cipher_text, op2_pack_cipher_text, pub)

    return add_res_cipher_text

