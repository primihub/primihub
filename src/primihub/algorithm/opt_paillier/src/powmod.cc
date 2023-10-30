#include "../include/powmod.h"
#include <cstdlib>

#define POWMOD_DEBUG 0

#if POWMOD_DEBUG
#include <cstdio>
#endif

/**
 * @brief arbitrary sizes of slide-window
 * @author Jiang Zhengliang
 * 
 * functions:
 *    fbpowmod_init_extend -> init a fixed-base instance
 *    fbpowmod_extend      -> implementation of powmod with fixed-base
 *    fbpowmod_end_extend  -> free memory after malloc
 */

using ui = unsigned int;
#define ROUNDUP(a,b) ((a)-1)/(b)+1

void fbpowmod_init_extend(
  fb_instance& fb_ins,
  const mpz_t base,
  const mpz_t mod,
  size_t maxbits,
  size_t winsize) {
    fb_ins.m_w = winsize;
    fb_ins.m_h = (1 << winsize);
    fb_ins.m_t = ROUNDUP(maxbits, winsize);
    fb_ins.m_table_G = (mpz_t*)malloc(sizeof(mpz_t) * (fb_ins.m_t + 1));
    mpz_t m_bi;
    mpz_init(m_bi);
    mpz_init(fb_ins.m_mod);
    mpz_set(fb_ins.m_mod, mod);
    for (size_t i = 0; i <= fb_ins.m_t; ++i) {
      mpz_init(fb_ins.m_table_G[i]);
      mpz_ui_pow_ui(m_bi, (ui)fb_ins.m_h, i);
      mpz_powm(fb_ins.m_table_G[i], base, m_bi, mod);
    }
    mpz_clear(m_bi);
  }

void fbpowmod_extend(
  const fb_instance& fb_ins,
  mpz_t result,
  const mpz_t exp) {
    ui* m_e = (ui*)malloc(sizeof(ui) * (fb_ins.m_t + 1));
    mpz_t m_exp, temp;
    mpz_inits(m_exp, temp, nullptr);
    mpz_set(m_exp, exp);
    size_t t = 0;
    for (; mpz_cmp_ui(m_exp, 0) > 0; ++t) {
      // mpz_tdiv_r_2exp(temp, m_exp, fb_ins.m_w);
      m_e[t] = mpz_mod_ui(temp, m_exp, fb_ins.m_h);
      mpz_tdiv_q_2exp(m_exp, m_exp, fb_ins.m_w);
      // m_e[t] = mpz_get_ui(temp);
    }
    mpz_set_ui(temp, 1);
    mpz_set_ui(result, 1);
    for (size_t j = fb_ins.m_h - 1; j >= 1; --j) {
      for (size_t i = 0; i < t; ++i) {
        if (m_e[i] == j) {
          mpz_mul(temp, temp, fb_ins.m_table_G[i]);
          mpz_mod(temp, temp, fb_ins.m_mod);
        }
      }
      mpz_mul(result, result, temp);
      mpz_mod(result, result, fb_ins.m_mod);
    }
    mpz_clears(m_exp, temp, nullptr);
    free(m_e);
    m_e = nullptr;
  }

void fbpowmod_end_extend(
  fb_instance& fb_ins) {
    for (size_t i = 0; i <= fb_ins.m_t; ++i) {
      mpz_clear(fb_ins.m_table_G[i]);
    }
    mpz_clear(fb_ins.m_mod);
    free(fb_ins.m_table_G);
    fb_ins.m_table_G = nullptr;
  }
