#ifndef _POWMOD_H_
#define _POWMOD_H_

#include <gmp.h>

/**
 * @brief arbitrary sizes of slide-window
 * @author Jiang Zhengliang
 * 
 */

struct fb_instance {
  mpz_t m_mod;
  mpz_t* m_table_G;
  size_t m_h;
  size_t m_t;
  size_t m_w;
};

void fbpowmod_init_extend(
  fb_instance& fb_ins,
  const mpz_t base,
  const mpz_t mod,
  size_t bitsize,
  size_t winsize);

void fbpowmod_extend(
  const fb_instance& fb_ins,
  mpz_t result,
  const mpz_t exp);

void fbpowmod_end_extend(
  fb_instance& fb_ins);

#endif
