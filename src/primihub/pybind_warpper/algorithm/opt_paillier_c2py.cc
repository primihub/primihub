#include "src/primihub/pybind_warpper/algorithm/opt_paillier_c2py.hpp"

namespace py = pybind11;
using namespace pybind11::literals;

py::dict fb_instance_2_pydict(fb_instance fb) {
    py::dict dict = py::dict();
    dict["m_mod"]     = mpz_get_str(nullptr, BASE, fb.m_mod);
    py::list t_pylist = py::list();
    for (size_t i = 0; i <= fb.m_t; i++) {
        t_pylist.append(mpz_get_str(nullptr, BASE, fb.m_table_G[i]));
    }
    dict["m_table_G"] = t_pylist;
    dict["m_h"]       = fb.m_h;
    dict["m_t"]       = fb.m_t;
    dict["m_w"]       = fb.m_w;
    return dict;
}

fb_instance pydict_2_fb_instance(py::dict dict) {
    fb_instance fb;
    mpz_init_set_str(fb.m_mod, std::string(py::str(dict["m_mod"])).c_str(), BASE);
    fb.m_t = py::int_(dict["m_t"]);
    fb.m_table_G = (mpz_t*)malloc(sizeof(mpz_t)*(fb.m_t+1));
    py::list py_m_table_G = dict["m_table_G"];
    for (size_t i = 0; i <= fb.m_t; i++) {
        mpz_init_set_str(fb.m_table_G[i], std::string(py::str(py_m_table_G[i])).c_str(), BASE);
    }
    fb.m_h = py::int_(dict["m_h"]);
    fb.m_w = py::int_(dict["m_w"]);
    return fb;
}

void opt_paillier_keygen_warpper(
    int k_sec,
    const py::object &py_pub,
    const py::object &py_prv) {

    opt_public_key_t* pub;
    opt_secret_key_t* prv;

    opt_paillier_keygen(k_sec, &pub, &prv);

    py::dict py_pub_dict = py_pub.attr("__dict__");
    py_pub_dict["nbits"]           = pub->nbits;
    py_pub_dict["lbits"]           = pub->lbits;
    py_pub_dict["n"]               = mpz_get_str(nullptr, BASE, pub->n);
    py_pub_dict["half_n"]          = mpz_get_str(nullptr, BASE, pub->half_n);
    py_pub_dict["n_squared"]       = mpz_get_str(nullptr, BASE, pub->n_squared);
    py_pub_dict["h_s"]             = mpz_get_str(nullptr, BASE, pub->h_s);
    py_pub_dict["fb_mod_P_sqaured"]= fb_instance_2_pydict(pub->fb_mod_P_sqaured);
    py_pub_dict["fb_mod_Q_sqaured"]= fb_instance_2_pydict(pub->fb_mod_Q_sqaured);

    py::dict py_prv_dict = py_prv.attr("__dict__");
    py_prv_dict["p"]                               = mpz_get_str(nullptr, BASE, prv->p);
    py_prv_dict["q"]                               = mpz_get_str(nullptr, BASE, prv->q);
    py_prv_dict["p_"]                              = mpz_get_str(nullptr, BASE, prv->p_);
    py_prv_dict["q_"]                              = mpz_get_str(nullptr, BASE, prv->q_);
    py_prv_dict["alpha"]                           = mpz_get_str(nullptr, BASE, prv->alpha);
    py_prv_dict["beta"]                            = mpz_get_str(nullptr, BASE, prv->beta);
    py_prv_dict["P"]                               = mpz_get_str(nullptr, BASE, prv->P);
    py_prv_dict["Q"]                               = mpz_get_str(nullptr, BASE, prv->Q);
    py_prv_dict["P_squared"]                       = mpz_get_str(nullptr, BASE, prv->P_squared);
    py_prv_dict["Q_squared"]                       = mpz_get_str(nullptr, BASE, prv->Q_squared);
    py_prv_dict["double_alpha"]                    = mpz_get_str(nullptr, BASE, prv->double_alpha);
    py_prv_dict["double_beta"]                     = mpz_get_str(nullptr, BASE, prv->double_beta);
    py_prv_dict["double_alpha_inverse"]            = mpz_get_str(nullptr, BASE, prv->double_alpha_inverse);
    py_prv_dict["P_squared_mul_P_squared_inverse"] = mpz_get_str(nullptr, BASE, prv->P_squared_mul_P_squared_inverse);
    py_prv_dict["P_mul_P_inverse"]                 = mpz_get_str(nullptr, BASE, prv->P_mul_P_inverse);
    py_prv_dict["double_p"]                        = mpz_get_str(nullptr, BASE, prv->double_p);
    py_prv_dict["double_q"]                        = mpz_get_str(nullptr, BASE, prv->double_q);
    py_prv_dict["Q_mul_double_p_inverse"]          = mpz_get_str(nullptr, BASE, prv->Q_mul_double_p_inverse);
    py_prv_dict["P_mul_double_q_inverse"]          = mpz_get_str(nullptr, BASE, prv->P_mul_double_q_inverse);
}

opt_public_key_t* py_pub_2_cpp_pub(const py::object &py_pub) {
    opt_public_key_t* res = (opt_public_key_t*)malloc(sizeof(opt_public_key_t));

    py::dict py_pub_dict = py_pub.attr("__dict__");
    res->nbits     = py::int_(py_pub_dict["nbits"]);
    res->lbits     = py::int_(py_pub_dict["lbits"]);
    mpz_init_set_str(res->n,         std::string(py::str(py_pub_dict["n"])).c_str(), BASE);
    mpz_init_set_str(res->half_n,    std::string(py::str(py_pub_dict["half_n"])).c_str(), BASE);
    mpz_init_set_str(res->n_squared, std::string(py::str(py_pub_dict["n_squared"])).c_str(), BASE);
    mpz_init_set_str(res->h_s,       std::string(py::str(py_pub_dict["h_s"])).c_str(), BASE);
    res->fb_mod_P_sqaured = pydict_2_fb_instance(py_pub_dict["fb_mod_P_sqaured"]);
    res->fb_mod_Q_sqaured = pydict_2_fb_instance(py_pub_dict["fb_mod_Q_sqaured"]);

    return res;
}

opt_secret_key_t* py_prv_2_cpp_prv(const py::object &py_prv) {
    opt_secret_key_t* res = (opt_secret_key_t*)malloc(sizeof(opt_secret_key_t));

    py::dict py_prv_dict = py_prv.attr("__dict__");
    mpz_init_set_str(res->p                              , std::string(py::str(py_prv_dict["p"])).c_str(), BASE);
    mpz_init_set_str(res->q                              , std::string(py::str(py_prv_dict["q"])).c_str(), BASE);
    mpz_init_set_str(res->p_                             , std::string(py::str(py_prv_dict["p_"])).c_str(), BASE);
    mpz_init_set_str(res->q_                             , std::string(py::str(py_prv_dict["q_"])).c_str(), BASE);
    mpz_init_set_str(res->alpha                          , std::string(py::str(py_prv_dict["alpha"])).c_str(), BASE);
    mpz_init_set_str(res->beta                           , std::string(py::str(py_prv_dict["beta"])).c_str(), BASE);
    mpz_init_set_str(res->P                              , std::string(py::str(py_prv_dict["P"])).c_str(), BASE);
    mpz_init_set_str(res->Q                              , std::string(py::str(py_prv_dict["Q"])).c_str(), BASE);
    mpz_init_set_str(res->P_squared                      , std::string(py::str(py_prv_dict["P_squared"])).c_str(), BASE);
    mpz_init_set_str(res->Q_squared                      , std::string(py::str(py_prv_dict["Q_squared"])).c_str(), BASE);
    mpz_init_set_str(res->double_alpha                   , std::string(py::str(py_prv_dict["double_alpha"])).c_str(), BASE);
    mpz_init_set_str(res->double_beta                    , std::string(py::str(py_prv_dict["double_beta"])).c_str(), BASE);
    mpz_init_set_str(res->double_alpha_inverse           , std::string(py::str(py_prv_dict["double_alpha_inverse"])).c_str(), BASE);
    mpz_init_set_str(res->P_squared_mul_P_squared_inverse, std::string(py::str(py_prv_dict["P_squared_mul_P_squared_inverse"])).c_str(), BASE);
    mpz_init_set_str(res->P_mul_P_inverse                , std::string(py::str(py_prv_dict["P_mul_P_inverse"])).c_str(), BASE);
    mpz_init_set_str(res->double_p                       , std::string(py::str(py_prv_dict["double_p"])).c_str(), BASE);
    mpz_init_set_str(res->double_q                       , std::string(py::str(py_prv_dict["double_q"])).c_str(), BASE);
    mpz_init_set_str(res->Q_mul_double_p_inverse         , std::string(py::str(py_prv_dict["Q_mul_double_p_inverse"])).c_str(), BASE);
    mpz_init_set_str(res->P_mul_double_q_inverse         , std::string(py::str(py_prv_dict["P_mul_double_q_inverse"])).c_str(), BASE);

    return res;
}

void opt_paillier_encrypt_warpper(
    const py::object &py_cipher_text,
    const py::object &py_pub,
    const std::string py_plain_text) {

    opt_public_key_t* pub = py_pub_2_cpp_pub(py_pub);

    mpz_t plain_text;
    mpz_t cipher_text;
    mpz_init(plain_text);
    mpz_init(cipher_text);

    opt_paillier_set_plaintext(plain_text, py_plain_text.c_str(), pub, PYTHON_INPUT_BASE);

    opt_paillier_encrypt(cipher_text, pub, plain_text);

    py::dict py_cipher_text_dict = py_cipher_text.attr("__dict__");
    py_cipher_text_dict["ciphertext"] = mpz_get_str(nullptr, BASE, cipher_text);

    mpz_clears(plain_text, cipher_text, nullptr);
    opt_paillier_freepubkey(pub);
}

void opt_paillier_encrypt_crt_warpper(
    const py::object &py_cipher_text,
    const py::object &py_pub,
    const py::object &py_prv,
    const std::string py_plain_text) {

    opt_public_key_t* pub = py_pub_2_cpp_pub(py_pub);
    opt_secret_key_t* prv = py_prv_2_cpp_prv(py_prv);

    mpz_t plain_text;
    mpz_t cipher_text;
    mpz_init(plain_text);
    mpz_init(cipher_text);

    opt_paillier_set_plaintext(plain_text, py_plain_text.c_str(), pub, PYTHON_INPUT_BASE);

    opt_paillier_encrypt_crt_fb(cipher_text, pub, prv, plain_text);

    py::dict py_cipher_text_dict = py_cipher_text.attr("__dict__");
    py_cipher_text_dict["ciphertext"] = mpz_get_str(nullptr, BASE, cipher_text);

    mpz_clears(plain_text, cipher_text, nullptr);
    opt_paillier_freepubkey(pub);
    opt_paillier_freeprvkey(prv);
}

py::str opt_paillier_decrypt_crt_warpper(
    const py::object &py_pub,
    const py::object &py_prv,
    const py::object &py_cipher_text) {

    opt_public_key_t* pub = py_pub_2_cpp_pub(py_pub);
    opt_secret_key_t* prv = py_prv_2_cpp_prv(py_prv);

    mpz_t cipher_text;
    mpz_t decrypt_text;
    mpz_init(cipher_text);
    mpz_init(decrypt_text);

    py::dict py_cipher_text_dict = py_cipher_text.attr("__dict__");
    mpz_set_str(cipher_text, std::string(py::str(py_cipher_text_dict["ciphertext"])).c_str(), BASE); // 0: success -1:error

    opt_paillier_decrypt_crt(decrypt_text, pub, prv, cipher_text);

    char *tmp_str;
    opt_paillier_get_plaintext(tmp_str, decrypt_text, pub, PYTHON_INPUT_BASE);

    mpz_clears(cipher_text, decrypt_text, nullptr);
    opt_paillier_freepubkey(pub);
    opt_paillier_freeprvkey(prv);

    return std::string(tmp_str);
}

void opt_paillier_add_warpper(
    const py::object &py_add_res,
    const py::object &py_op1,
    const py::object &py_op2,
    const py::object &py_pub) {

    opt_public_key_t* pub = py_pub_2_cpp_pub(py_pub);

    mpz_t op1;
    mpz_t op2;
    mpz_t res;
    mpz_init(op1);
    mpz_init(op2);
    mpz_init(res);

    py::dict py_op1_dict = py_op1.attr("__dict__");
    mpz_set_str(op1, std::string(py::str(py_op1_dict["ciphertext"])).c_str(), BASE); // 0: success -1:error
    py::dict py_op2_dict = py_op2.attr("__dict__");
    mpz_set_str(op2, std::string(py::str(py_op2_dict["ciphertext"])).c_str(), BASE); // 0: success -1:error

    opt_paillier_add(res, op1, op2, pub);

    py::dict py_add_res_dict = py_add_res.attr("__dict__");
    py_add_res_dict["ciphertext"] = mpz_get_str(nullptr, BASE, res);

    mpz_clears(op1, op2, res, nullptr);
    opt_paillier_freepubkey(pub);
}

void opt_paillier_cons_mul_warpper(
    const py::object &py_cons_mul_res,
    const py::object &py_cipher_text,
    const py::str    &py_cons_value,
    const py::object &py_pub) {

    opt_public_key_t* pub = py_pub_2_cpp_pub(py_pub);

    mpz_t cipher_text;
    mpz_t cons_value;
    mpz_t res;
    mpz_init(cipher_text);
    mpz_init(cons_value);
    mpz_init(res);

    py::dict py_cipher_text_dict = py_cipher_text.attr("__dict__");
    mpz_set_str(cipher_text, std::string(py::str(py_cipher_text_dict["ciphertext"])).c_str(), BASE); // 0: success -1:error
    opt_paillier_set_plaintext(cons_value, std::string(py_cons_value).c_str(), pub);

    opt_paillier_constant_mul(res, cipher_text, cons_value, pub);

    py::dict py_mul_res_dict = py_cons_mul_res.attr("__dict__");
    py_mul_res_dict["ciphertext"] = mpz_get_str(nullptr, BASE, res);

    mpz_clears(cipher_text, cons_value, res, nullptr);
    opt_paillier_freepubkey(pub);
}

py::dict crtMod_2_dict(CrtMod* crtmod) {
    py::dict res = py::dict();
    py::list crt_half_mod = py::list();
    py::list crt_mod = py::list();
    for (size_t i = 0; i < crtmod->crt_size; ++i) {
        crt_half_mod.append(mpz_get_str(nullptr, BASE, crtmod->crt_half_mod[i]));
        crt_mod.append(mpz_get_str(nullptr, BASE, crtmod->crt_mod[i]));
    }
    res["crt_half_mod"] = crt_half_mod;
    res["crt_mod"] = crt_mod;
    res["crt_size"] = crtmod->crt_size;
    res["mod_size"] = (uint64_t)crtmod->mod_size;
    return res;
}

CrtMod* dict_2_CrtMod(py::dict py_crtMod) {
    CrtMod* res = (CrtMod*)malloc(sizeof(CrtMod));

    res->crt_size = py::int_(py_crtMod["crt_size"]);
    res->mod_size = py::int_(py_crtMod["mod_size"]);

    res->crt_half_mod = (mpz_t*)malloc(sizeof(mpz_t) * res->crt_size);
    res->crt_mod = (mpz_t*)malloc(sizeof(mpz_t) * res->crt_size);
    py::list crt_half_mod = py_crtMod["crt_half_mod"];
    py::list crt_mod = py_crtMod["crt_mod"];
    for (size_t i = 0; i < res->crt_size; i++) {
        mpz_init_set_str(res->crt_half_mod[i], std::string(py::str(crt_half_mod[i])).c_str(), BASE);
        mpz_init_set_str(res->crt_mod[i], std::string(py::str(crt_mod[i])).c_str(), BASE);
    }

    return res;
}

void opt_paillier_pack_encrypt_warpper(
    const py::object &py_pack_cipher_text,
    const py::object &py_pub,
    const py::list &py_plain_texts,
    const py::object &py_crt_mod
    ) {

    opt_public_key_t* pub = py_pub_2_cpp_pub(py_pub);

    CrtMod* crtmod;
    // if (py_crt_mod == py::none()) {
    if (py_crt_mod.is(py::none())) {
        init_crt(&crtmod, CRT_MOD_MAX_DIMENSION, CRT_MOD_SIZE);
    } else {
        crtmod = dict_2_CrtMod(py::dict(py_crt_mod));
    }

    size_t plain_texts_dimension = py::len(py_plain_texts);

    py::list ciphertexts = py::list();
    mpz_t pack;
    mpz_init(pack);
    mpz_t cipher_text;
    mpz_init(cipher_text);
    for (size_t pos = 0; pos < plain_texts_dimension; pos += CRT_MOD_MAX_DIMENSION) {
        size_t data_size = std::min(plain_texts_dimension - pos,
                                    static_cast<size_t>(CRT_MOD_MAX_DIMENSION));
        char** nums = (char**)malloc(sizeof(char*) * data_size);
        for (size_t j = 0; j < data_size; ++j) {
            std::string py_plain_text = std::string(py::str(py_plain_texts[pos + j]));
            nums[j] = (char*)malloc(sizeof(char) * (py_plain_text.length() + 1));
            strcpy(nums[j], py_plain_text.c_str());
        }

        data_packing_crt(pack, nums, data_size, crtmod, PYTHON_INPUT_BASE);
        opt_paillier_encrypt(cipher_text, pub, pack);

        ciphertexts.append(mpz_get_str(nullptr, BASE, cipher_text));

        for (size_t j = 0; j < data_size; ++j) {
            free(nums[j]);
        }
        free(nums);
    }

    py::dict py_pack_cipher_text_dict = py_pack_cipher_text.attr("__dict__");
    py_pack_cipher_text_dict["ciphertexts"] = ciphertexts;
    py_pack_cipher_text_dict["crtMod"] = crtMod_2_dict(crtmod);
    py_pack_cipher_text_dict["pack_size"] = plain_texts_dimension;

    mpz_clears(pack, cipher_text, nullptr);
    free_crt(crtmod);
    opt_paillier_freepubkey(pub);
}

void opt_paillier_pack_encrypt_crt_warpper(
    const py::object &py_pack_cipher_text,
    const py::object &py_pub,
    const py::object &py_prv,
    const py::list &py_plain_texts,
    const py::object &py_crt_mod
    ) {

    opt_public_key_t* pub = py_pub_2_cpp_pub(py_pub);
    opt_secret_key_t* prv = py_prv_2_cpp_prv(py_prv);

    CrtMod* crtmod;
    // if (py_crt_mod == py::none()) {
    if (py_crt_mod.is(py::none())) {
        init_crt(&crtmod, CRT_MOD_MAX_DIMENSION, CRT_MOD_SIZE);
    } else {
        crtmod = dict_2_CrtMod(py::dict(py_crt_mod));
    }

    size_t plain_texts_dimension = py::len(py_plain_texts);

    py::list ciphertexts = py::list();
    mpz_t pack;
    mpz_init(pack);
    mpz_t cipher_text;
    mpz_init(cipher_text);
    for (size_t pos = 0; pos < plain_texts_dimension; pos += CRT_MOD_MAX_DIMENSION) {
        size_t data_size = std::min(plain_texts_dimension - pos,
                                   static_cast<size_t>(CRT_MOD_MAX_DIMENSION));
        char** nums = (char**)malloc(sizeof(char*) * data_size);
        for (size_t j = 0; j < data_size; ++j) {
            std::string py_plain_text = std::string(py::str(py_plain_texts[pos + j]));
            nums[j] = (char*)malloc(sizeof(char) * (py_plain_text.length() + 1));
            strcpy(nums[j], py_plain_text.c_str());
        }

        data_packing_crt(pack, nums, data_size, crtmod, PYTHON_INPUT_BASE);
        opt_paillier_encrypt_crt(cipher_text, pub, prv, pack);

        ciphertexts.append(mpz_get_str(nullptr, BASE, cipher_text));

        for (size_t j = 0; j < data_size; ++j) {
            free(nums[j]);
        }
        free(nums);
    }

    py::dict py_pack_cipher_text_dict = py_pack_cipher_text.attr("__dict__");
    py_pack_cipher_text_dict["ciphertexts"] = ciphertexts;
    py_pack_cipher_text_dict["crtMod"] = crtMod_2_dict(crtmod);
    py_pack_cipher_text_dict["pack_size"] = plain_texts_dimension;

    mpz_clears(pack, cipher_text, nullptr);
    free_crt(crtmod);
    opt_paillier_freepubkey(pub);
    opt_paillier_freeprvkey(prv);
}

py::list opt_paillier_pack_decrypt_crt_warpper(
    const py::object &py_pub,
    const py::object &py_prv,
    const py::object &py_pack_cipher_text) {

    opt_public_key_t* pub = py_pub_2_cpp_pub(py_pub);
    opt_secret_key_t* prv = py_prv_2_cpp_prv(py_prv);

    py::dict py_pack_cipher_text_dict = py_pack_cipher_text.attr("__dict__");
    py::list py_ciphertexts = py_pack_cipher_text_dict["ciphertexts"];
    size_t cipher_text_num = py::int_(py_pack_cipher_text_dict["pack_size"]);

    CrtMod* crtmod = dict_2_CrtMod(py_pack_cipher_text_dict["crtMod"]);

    mpz_t cipher_text;
    mpz_t decrypt_text;
    mpz_init(cipher_text);
    mpz_init(decrypt_text);

    py::list res = py::list();
    for (py::handle py_ciphertext : py_ciphertexts) {
        size_t data_size = std::min((size_t)CRT_MOD_MAX_DIMENSION, cipher_text_num);
        cipher_text_num = cipher_text_num - CRT_MOD_MAX_DIMENSION;

        mpz_set_str(cipher_text, std::string(py::str(py_ciphertext)).c_str(), BASE); // 0: success -1:error
        opt_paillier_decrypt_crt(decrypt_text, pub, prv, cipher_text);

        char** nums;
        data_retrieve_crt(nums, decrypt_text, crtmod, data_size, PYTHON_INPUT_BASE);
        for (size_t i = 0; i < data_size; i++) {
            res.append(nums[i]);
        }

        for (size_t j = 0; j < data_size; ++j) {
            free(nums[j]);
        }
        free(nums);
    }

    mpz_clears(cipher_text, decrypt_text, nullptr);
    free_crt(crtmod);
    opt_paillier_freepubkey(pub);
    opt_paillier_freeprvkey(prv);

    return res;
}

void opt_paillier_pack_add_warpper(
    const py::object &py_pack_add_res,
    const py::object &py_pack_op1,
    const py::object &py_pack_op2,
    const py::object &py_pub) {

    opt_public_key_t* pub = py_pub_2_cpp_pub(py_pub);

    mpz_t op1;
    mpz_t op2;
    mpz_t res;
    mpz_init(op1);
    mpz_init(op2);
    mpz_init(res);

    py::dict py_pack_op1_dict = py_pack_op1.attr("__dict__");
    py::dict py_pack_op2_dict = py_pack_op2.attr("__dict__");

    py::list py_pack_op1_ciphertexts = py_pack_op1_dict["ciphertexts"];
    py::list py_pack_op2_ciphertexts = py_pack_op2_dict["ciphertexts"];
    py::list ciphertexts = py::list();
    size_t cipher_text_num = py::len(py_pack_op1_ciphertexts);
    for (size_t i = 0; i < cipher_text_num; i++) {
        mpz_set_str(op1, std::string(py::str(py_pack_op1_ciphertexts[i])).c_str(), BASE); // 0: success -1:error
        mpz_set_str(op2, std::string(py::str(py_pack_op2_ciphertexts[i])).c_str(), BASE); // 0: success -1:error

        opt_paillier_add(res, op1, op2, pub);

        ciphertexts.append(mpz_get_str(nullptr, BASE, res));
    }

    py::dict py_pack_add_res_dict = py_pack_add_res.attr("__dict__");
    py_pack_add_res_dict["ciphertexts"] = ciphertexts;
    py_pack_add_res_dict["crtMod"] = py_pack_op1_dict["crtMod"];
    py_pack_add_res_dict["pack_size"] = py_pack_op1_dict["pack_size"];

    mpz_clears(op1, op2, res, nullptr);
    opt_paillier_freepubkey(pub);
}

PYBIND11_MODULE(opt_paillier_c2py, m) {
    m.doc() = "opt paillier cpp to python plugin"; // optional module docstring

    m.def("opt_paillier_keygen_warpper",
         &opt_paillier_keygen_warpper,
         "A function that generate opt paillier publice key and private key");

    m.def("opt_paillier_encrypt_warpper",
         &opt_paillier_encrypt_warpper,
         "A opt paillier encrypt function that encrypt plaintext");

    m.def("opt_paillier_encrypt_crt_warpper",
         &opt_paillier_encrypt_crt_warpper,
         "A opt paillier encrypt function that encrypt plaintext");

    m.def("opt_paillier_decrypt_crt_warpper",
         &opt_paillier_decrypt_crt_warpper,
         "A opt paillier decrypt function that decrypt ciphertext");

    m.def("opt_paillier_add_warpper",
         &opt_paillier_add_warpper,
         "A opt paillier add function that add two ciphertext");

    m.def("opt_paillier_cons_mul_warpper",
         &opt_paillier_cons_mul_warpper,
         "A opt paillier constant multiplication function that multify one ciphertext with one constant value");

    m.def("opt_paillier_pack_encrypt_warpper",
         &opt_paillier_pack_encrypt_warpper,
         "A opt paillier encrypt function that pack encrypt plaintext");

    m.def("opt_paillier_pack_encrypt_crt_warpper",
         &opt_paillier_pack_encrypt_crt_warpper,
         "A opt paillier encrypt function that pack encrypt plaintext");

    m.def("opt_paillier_pack_decrypt_crt_warpper",
         &opt_paillier_pack_decrypt_crt_warpper,
         "A opt paillier decrypt function that pack decrypt ciphertext");

    m.def("opt_paillier_pack_add_warpper",
         &opt_paillier_pack_add_warpper,
         "A opt paillier add function that add two pack ciphertext");
}
