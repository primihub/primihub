import opt_paillier_c2py

class Opt_paillier_public_key(object):
    """
    Attributes:
        nbits            int    <-> ui
        lbits            int    <-> ui
        n                string <-> mpz_t
        half_n           string <-> mpz_t
        n_squared        string <-> mpz_t
        h_s              string <-> mpz_t
        fb_mod_P_sqaured dict   <-> fb_instance
        fb_mod_Q_sqaured dict   <-> fb_instance

        fb_instance
            m_mod        string <-> mpz_t
            m_table_G    list   <-> mpz_t[]
            m_h          int    <-> size_t
            m_t          int    <-> size_t
            m_w          int    <-> size_t
    """
    def __init__(self):
        pass
    
    def __str__(self):
        return str(self.__dict__)

class Opt_paillier_secret_key(object):
    """
    Attributes:
        p                               string <-> mpz_t
        q                               string <-> mpz_t
        p_                              string <-> mpz_t
        q_                              string <-> mpz_t
        alpha                           string <-> mpz_t
        beta                            string <-> mpz_t
        P                               string <-> mpz_t
        Q                               string <-> mpz_t
        P_squared                       string <-> mpz_t
        Q_squared                       string <-> mpz_t
        double_alpha                    string <-> mpz_t
        double_beta                     string <-> mpz_t
        double_alpha_inverse            string <-> mpz_t
        P_squared_mul_P_squared_inverse string <-> mpz_t
        P_mul_P_inverse                 string <-> mpz_t
        double_p                        string <-> mpz_t
        double_q                        string <-> mpz_t
        Q_mul_double_p_inverse          string <-> mpz_t
        P_mul_double_q_inverse          string <-> mpz_t
    """
    def __init__(self):
        pass

    def __str__(self):
        return str(self.__dict__)

class Opt_paillier_ciphertext(object):
    """
    Attributes:
        ciphertext   string <-> mpz_t
    """
    def __init__(self):
        pass

    def __str__(self):
        return str(self.__dict__)

def opt_paillier_keygen(k_sec = 112):
    
    pub = Opt_paillier_public_key()
    prv = Opt_paillier_secret_key()

    opt_paillier_c2py.opt_paillier_keygen_warpper(k_sec, pub, prv)

    return pub, prv

def opt_paillier_encrypt_crt(pub, prv, plain_text):

    if not isinstance (plain_text, int):
        print("opt_paillier_encrypt plain_text should be type of int")
        return

    plain_text_str = str(plain_text)

    cipher_text = Opt_paillier_ciphertext()

    opt_paillier_c2py.opt_paillier_encrypt_crt_warpper(cipher_text, pub, prv, plain_text_str)

    return cipher_text

def opt_paillier_encrypt(pub, plain_text):

    if not isinstance (plain_text, int):
        print("opt_paillier_encrypt plain_text should be type of int")
        return

    plain_text_str = str(plain_text)

    cipher_text = Opt_paillier_ciphertext()

    opt_paillier_c2py.opt_paillier_encrypt_warpper(cipher_text, pub, plain_text_str)

    return cipher_text

def opt_paillier_decrypt_crt(pub, prv, cipher_text):

    if not isinstance (cipher_text, Opt_paillier_ciphertext):
        print("opt_paillier_decrypt cipher_text should be type of Opt_paillier_ciphertext()")
        return

    decrypt_text = opt_paillier_c2py.opt_paillier_decrypt_crt_warpper(pub, prv, cipher_text)

    decrypt_text_num = int(decrypt_text)

    return decrypt_text_num

def opt_paillier_add(pub, op1_cipher_text, op2_cipher_text):

    if not isinstance (op1_cipher_text, Opt_paillier_ciphertext):
        print("opt_paillier_add op1_cipher_text should be type of Opt_paillier_ciphertext()")
        return
    if not isinstance (op2_cipher_text, Opt_paillier_ciphertext):
        print("opt_paillier_add op2_cipher_text should be type of Opt_paillier_ciphertext()")
        return

    add_res_cipher_text = Opt_paillier_ciphertext()

    opt_paillier_c2py.opt_paillier_add_warpper(add_res_cipher_text, op1_cipher_text, op2_cipher_text, pub)

    return add_res_cipher_text

def opt_paillier_cons_mul(pub, op1_cipher_text, op2_cons_value):

    if not isinstance (op1_cipher_text, Opt_paillier_ciphertext):
        print("opt_paillier_cons_mul op1_cipher_text should be type of Opt_paillier_ciphertext()")
        return
    if not isinstance (op2_cons_value, int):
        print("opt_paillier_cons_mul op2_cons_value should be type of int()")
        return

    cons_mul_res_cipher_text = Opt_paillier_ciphertext()

    opt_paillier_c2py.opt_paillier_cons_mul_warpper(cons_mul_res_cipher_text, op1_cipher_text, str(op2_cons_value), pub)

    return cons_mul_res_cipher_text
