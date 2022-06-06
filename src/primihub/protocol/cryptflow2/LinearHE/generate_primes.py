import math
import gmpy2
import random

bitlengths = range(32, 42, 1)
primes = []
mod_degree = 2**19

for bitlen in bitlengths:
    p = mod_degree
    rand_bitlen = bitlen-int(math.log2(mod_degree))-2
    while not gmpy2.is_prime(p):
        p = 2*mod_degree*random.randrange(int(2.0**(rand_bitlen+0.999)), 2**(rand_bitlen + 1)) + 1
    primes.append(p)

print(primes)

'''
mod_degree = 65536
const std::map<int32_t, uint64_t> default_prime_mod{
    {32, 4293918721},    {33, 8585084929},   {34, 17171218433},
    {35, 34359214081},   {36, 68686184449},  {37, 137352314881},
    {38, 274824036353},  {39, 549753716737}, {40, 1099480956929},
    {41, 2198100901889},
};
'''
