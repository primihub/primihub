/*
Original Work Copyright (c) 2018 Xiao Wang (wangxiao@gmail.com)
Modified Work Copyright (c) 2020 Microsoft Research

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

Enquiries about further applications and development opportunities are welcome.

Modified by Nishant Kumar, Deevashwer Rathee
*/

template<class... Ts>
void run_function(void *function, const Ts&... args) {
    reinterpret_cast<void(*)(Ts...)>(function)(args...);
}

template<typename T>
void inline delete_array_null(T * ptr){
    if(ptr != nullptr) {
        delete[] ptr;
        ptr = nullptr;
    }
}

template <typename T>
std::string m128i_to_string(const __m128i var) {
    std::stringstream sstr;
    const T* values = (const T*) &var;
    for (unsigned int i = 0; i < sizeof(__m128i) / sizeof(T); i++) {
        sstr <<"0x"<<std::hex<< values[i] << " ";
    }
    return sstr.str();
}

inline time_point<high_resolution_clock> clock_start() {
    return high_resolution_clock::now();
}

inline double time_from(const time_point<high_resolution_clock>& s) {
    return std::chrono::duration_cast<std::chrono::microseconds>(high_resolution_clock::now() - s).count();
}

inline string bin_to_dec(const string& bin2) {
    if(bin2[0] == '0')
        return change_base(bin2, 2, 10);
    string bin = bin2;
    bool flip = false;
    for(int i = bin.size()-1; i>=0; --i) {
        if(flip)
            bin[i] = (bin[i] == '1' ? '0': '1');
        if(bin[i] == '1')
            flip = true;
    }
    return "-"+change_base(bin, 2, 10);
}

inline string dec_to_bin(const string& dec) {
    string bin = change_base(dec, 10, 2);
    if(dec[0] != '-')
        return '0' + bin;
    bin[0] = '1';
    bool flip = false;
    for(int i = bin.size()-1; i>=1; --i) {
        if(flip)
            bin[i] = (bin[i] == '1' ? '0': '1');
        if(bin[i] == '1')
            flip = true;
    }
    return bin;
}

inline string change_base(string str, int old_base, int new_base) {
    mpz_t tmp;
    mpz_init_set_str (tmp, str.c_str(), old_base);
    char * b = new char[mpz_sizeinbase(tmp, new_base) + 2];
    mpz_get_str(b, new_base, tmp);
    mpz_clear(tmp);
    string res(b);
    delete[]b;
    return res;
}

inline void error(const char * s, int line, const char * file) {
    fprintf(stderr, s, "\n");
    if(file != nullptr) {
        fprintf(stderr, "at %d, %s\n", line, file);
    }
    exit(1);
}

inline const char* hex_char_to_bin(char c) {
    switch(toupper(c)) {
        case '0': return "0000";
        case '1': return "0001";
        case '2': return "0010";
        case '3': return "0011";
        case '4': return "0100";
        case '5': return "0101";
        case '6': return "0110";
        case '7': return "0111";
        case '8': return "1000";
        case '9': return "1001";
        case 'A': return "1010";
        case 'B': return "1011";
        case 'C': return "1100";
        case 'D': return "1101";
        case 'E': return "1110";
        case 'F': return "1111";
        default: return "0";
    }
}

inline std::string hex_to_binary(std::string hex) {
    std::string bin;
    for(unsigned i = 0; i != hex.length(); ++i)
        bin += hex_char_to_bin(hex[i]);
    return bin;
}
inline void parse_party_and_port(char ** arg, int argc, int * party, int * port) {
    if (argc == 1)
        error("ERROR: argc = 1, need two argsm party ID {1,2} and port.");
    else if (argc == 2)
        *party = atoi(arg[1]);
    else if (argc >= 3) {
        *party = atoi (arg[1]);
        *port = atoi (arg[2]);
    }
}

inline std::string Party(int p) {
    if (p == ALICE)
        return "ALICE";
    else if (p == BOB)
        return "BOB";
    else return "PUBLIC";
}

template<typename T>
inline void int_to_bool(bool * data, T input, int len) {
	for (int i = 0; i < len; ++i) {
		data[i] = (input & 1)==1;
		input >>= 1;
	}
}

template<typename t>
t bool_to_int(const bool * data, size_t len) {
    if (len != 0) len = (len > sizeof(t)*8 ? sizeof(t)*8 : len);
    else len = sizeof(t)*8;
    t res = 0;
    for(size_t i = 0; i < len-1; ++i) {
        if(data[i])
            res |= (1LL<<i);
    }
    if(data[len-1]) return -1*res;
    else return res;
}

inline uint8_t bool_to_uint8(const uint8_t * data, size_t len) {
    if (len != 0) len = (len > 8 ? 8 : len);
    else len = 8;
    uint8_t res = 0;
    for(size_t i = 0; i < len; ++i) {
        if(data[i])
            res |= (1ULL<<i);
    }
    return res;
}

inline uint64_t bool_to64(const bool * data) {
    uint64_t res = 0;
    for(int i = 0; i < 64; ++i) {
        if(data[i])
            res |= (1ULL<<i);
    }
    return res;
}
inline block128 bool_to128(const bool * data) {
    return makeBlock128(bool_to64(data+64), bool_to64(data));
}
inline block256 bool_to256(const bool * data) {
    return makeBlock256(bool_to128(data+128), bool_to128(data));
}

inline void int64_to_bool(bool * data, uint64_t input, int length) {
    for (int i = 0; i < length; ++i) {
        data[i] = (input & 1)==1;
        input >>= 1;
    }
}

inline void uint8_to_bool(uint8_t * data, uint8_t input, int length) {
    for (int i = 0; i < length; ++i) {
        data[i] = (input & 1)==1;
        input >>= 1;
    }
}

// NOTE: This function doesn't return bitlen. It returns log_alpha
inline int bitlen(int x) { // x & (-x)
    if (x < 1) return 0;
    for (int i = 0; i < 32; i++) {
        int curr = 1 << i;
        if (curr >= x) return i;
    }
    return 0;
}

inline int bitlen_true(int x) {
    if (x < 1) return 0;
    for (int i = 0; i < 32; i++) {
        int curr = 1 << i;
        if (curr > x) return (i);
				else if(curr == x) return (i+1);
    }
    return 0;
}

// Taken from https://github.com/mc2-project/delphi/blob/master/rust/protocols-sys/c++/src/lib/conv2d.h
/* Helper function for performing modulo with possibly negative numbers */
inline int64_t neg_mod(int64_t val, int64_t mod) {
    return ((val % mod) + mod) % mod;
}

/* Helper function for performing modulo with possibly negative numbers */
inline int8_t neg_mod(int8_t val, int8_t mod) {
    return ((val % mod) + mod) % mod;
}
// Converts  uint64_t values to __m128i or block128
inline block128 toBlock(uint64_t high_u64, uint64_t low_u64) { return _mm_set_epi64x(high_u64, low_u64); }
inline block128 toBlock(uint64_t low_u64) { return toBlock(0, low_u64); }

inline uint64_t all1Mask(int x){
    if (x==64) return -1;
    else return (1ULL<<x)-1;
}

#define INIT_TIMER auto start_timer = std::chrono::high_resolution_clock::now(); \
    uint64_t pause_timer = 0;
#define START_TIMER  start_timer = std::chrono::high_resolution_clock::now();
#define PAUSE_TIMER(name) pause_timer += std::chrono::duration_cast<std::chrono::milliseconds>( \
            std::chrono::high_resolution_clock::now()-start_timer).count(); \
    std::cout << "[PAUSING TIMER] RUNTIME till now of " << name << ": " << pause_timer<<" ms"<<std::endl;
#define STOP_TIMER(name) std::cout << "------------------------------------" << std::endl; std::cout << "[STOPPING TIMER] Total RUNTIME of " << name << ": " << \
    std::chrono::duration_cast<std::chrono::milliseconds>( \
            std::chrono::high_resolution_clock::now()-start_timer \
    ).count() + pause_timer << " ms " << std::endl; 
#define TIMER_TILL_NOW std::chrono::duration_cast<std::chrono::milliseconds>(\
    std::chrono::high_resolution_clock::now()-start_timer).count()

#define INIT_IO_DATA_SENT uint64_t dataSentCtr__1 = io->counter;
#define IO_TILL_NOW (io->counter - dataSentCtr__1);
#define RESET_IO dataSentCtr__1 = io->counter;

#define INIT_ALL_IO_DATA_SENT uint64_t __ioStartTracker[::num_threads];\
        for(int __thrdCtr = 0; __thrdCtr < ::num_threads; __thrdCtr++){\
            __ioStartTracker[__thrdCtr] = ::ioArr[__thrdCtr]->counter;\
        }
#define FIND_ALL_IO_TILL_NOW(var) uint64_t __curComm = 0;\
        for(int __thrdCtr = 0; __thrdCtr < ::num_threads; __thrdCtr++){\
             __curComm += ((::ioArr[__thrdCtr]->counter) - __ioStartTracker[__thrdCtr]);\
        }\
        var = __curComm;
#define RESET_ALL_IO for(int __thrdCtr = 0; __thrdCtr < ::num_threads; __thrdCtr++){\
            __ioStartTracker[__thrdCtr] = ::ioArr[__thrdCtr]->counter;\
        }

inline void print128_num(__m128i var) 
{
    uint64_t *v64val = (uint64_t*) &var;
    printf("%.16llx %.16llx\n", v64val[1], v64val[0]);
}

inline void linIdxRowMInverseMapping(int cur, int s1, int s2, int& i, int& j){
    i = cur/s2;
    j = (cur-i*s2);
}

inline void linIdxRowMInverseMapping(int cur, int s1, int s2, int s3, int& i, int& j, int& k){
    i = cur/(s2*s3);
    j = (cur-i*s2*s3)/s3;
    k = (cur-i*s2*s3-j*s3);
}

// Note of the bracket around each expression use -- if this is not there, not macro expansion
//  can result in hard in trace bugs when expanded around expressions like 1-2.
#define Arr1DIdxRowM(arr,s0,i) (*((arr) + (i)))
#define Arr2DIdxRowM(arr,s0,s1,i,j) (*((arr) + (i)*(s1) + (j)))
#define Arr3DIdxRowM(arr,s0,s1,s2,i,j,k) (*((arr) + (i)*(s1)*(s2) + (j)*(s2) + (k)))
#define Arr4DIdxRowM(arr,s0,s1,s2,s3,i,j,k,l) (*((arr) + (i)*(s1)*(s2)*(s3) + (j)*(s2)*(s3) + (k)*(s3) + (l)))
#define Arr5DIdxRowM(arr,s0,s1,s2,s3,s4,i,j,k,l,m) (*((arr) + (i)*(s1)*(s2)*(s3)*(s4) + (j)*(s2)*(s3)*(s4) + (k)*(s3)*(s4) + (l)*(s4) + (m)))

#define Arr2DIdxColM(arr,s0,s1,i,j) (*((arr) + (j)*(s0) + (i)))

template<typename intType>
void elemWiseAdd(int len, const intType* A, const intType* B, intType* C){
    for(int i=0;i<len;i++){
        C[i] = A[i] + B[i];
    }
}

template<typename intType>
void elemWiseSub(int len, const intType* A, const intType* B, intType* C){
    for(int i=0;i<len;i++){
        C[i] = A[i] - B[i];
    }
}   

template<typename intType>
void print2DArr(int s1, int s2, const intType* arr){
    for(int i=0;i<s1;i++){
        for(int j=0;j<s2;j++){
            std::cout<<Arr2DIdxRowM(arr,s1,s2,i,j)<<" ";
        }
        std::cout<<"\n";
    }
    std::cout<<std::endl;
}

template<typename intType>
void convertRowToColMajor(int s1, int s2, const intType* rowMajor, intType* columnMajor){
    for(int i=0;i<s1;i++){
        for(int j=0;j<s2;j++){
            Arr2DIdxColM(columnMajor,s1,s2,i,j) = Arr2DIdxRowM(rowMajor,s1,s2,i,j);
        }
    }
}

template<typename intType>
void convertColToRowMajor(int s1, int s2, const intType* columnMajor, intType* rowMajor){
    for(int i=0;i<s1;i++){
        for(int j=0;j<s2;j++){
            Arr2DIdxRowM(rowMajor,s1,s2,i,j) = Arr2DIdxColM(columnMajor,s1,s2,i,j);
        }
    }
}

template<typename intType>
void copyElemWisePadded(int s1, intType* inp, int s2, intType* outp, int padVal){
    assert(s2>=s1);
    for(int i=0;i<s1;i++) outp[i] = inp[i];
    for(int i=s1;i<s2;i++) outp[i] = padVal;
}

inline uint64_t moduloMult(uint64_t a, uint64_t b, uint64_t mod){ 
    uint64_t res = 0;
    a %= mod; 
    while (b){
        if (b & 1) 
            res = (res + a) % mod; 
        a = (2 * a) % mod; 
        b >>= 1; 
    }  
    return res; 
}

inline uint64_t ceil_val(uint64_t x, uint64_t y){
    return (x+y-1)/y;
}

/*
    Given a byte array arr of length arrLen, interpret this as a bit array,
        and read from bitIdx = bitIdx, #bits = bitsToRead.
    Return the read value as uint64_t (assuming obviously that bitsToRead <= 64).
    The answer is stored in the lower bitsToRead bits of the returned value (lsb onwards).
    Note: In the new implementation, while reading, this reads a uint64_t. Ensure the array
    from which reading is being done has one uint64_t extra at the end to prevent reading
    out of bounds.
    This code assumes little endian encoding for the system
*/
static uint64_t readFromPackedArr(uint8_t* arr, int arrLen, uint64_t bitIdx, uint64_t bitsToRead){
    assert(bitsToRead<=64);
    uint64_t firstByteIdx = bitIdx/8;
    uint64_t toRejectBitsInFirstByte = bitIdx%8;
    uint64_t ans = *((uint64_t*)(arr+firstByteIdx));
    ans >>= toRejectBitsInFirstByte;
    if (bitsToRead > 64 - toRejectBitsInFirstByte){
        // Read from the unaligned byte at firstByteIdx+8
        ans = ans & ((1ULL<<(64 - toRejectBitsInFirstByte))-1);
        uint64_t temp = ((uint64_t)(arr[firstByteIdx+8])) & ((1ULL<<toRejectBitsInFirstByte)-1);
        ans += (temp<<(64 - toRejectBitsInFirstByte));
    }
    ans = ans & all1Mask(bitsToRead);
    return ans;
}

/*
    In the uint8_t pointed by x, starts writing lower 8-idx values of val starting from bit idx
*/
static void writeInAByte(uint8_t* x, int idx, uint8_t val){
    (*x) = (*x) & ((1<<idx)-1);
    (*x) = (*x) + (val << idx);
}

/*
    Similar to above, given a byte array arr of length = arrLen, interpret this as a bit array,
    and start writing bits at bitIdx = bitIdx with #bits = bitlen. Actual bits to be written are given
    in the lower bits of val, i.e. lower bitlen bits of val represent the bits to be written.
    Obviously again bitlen <= 64.
    Note that in the final unaligned byte this writes 0 in the extra positions.
    Also, assuming extra bits of val after bitlen are filled with 0s.
    Note: The new implementation assumes that there one extra uint64_t space at the end.
    This is because it writes the whole uint64_t given and so to prevent writing out of bounds,
    ensure the array has one extra uint64_t space at the end.
    Note: this code assumes little endian encoding
*/
static void writeToPackedArr(uint8_t* arr, int arrLen, uint64_t bitIdx, uint64_t bitlen, uint64_t val){
    assert(bitlen<=64);
    //First write the unaligned bits -- i.e. first byte which needs to be written
    uint64_t firstByteIdx = bitIdx/8;
    uint64_t freeBitsInFirstByte = 8 - (bitIdx%8);
    uint8_t valLowerByte = val; //This will give the lower 8 bits of val
    writeInAByte(arr+firstByteIdx,bitIdx%8,valLowerByte); //If bitlen < freeBitsInFirstByte, this will write out 0 in those extra positions
    // Now things are byte aligned
    uint64_t valIter = (val >> freeBitsInFirstByte); //Since val is unsigned, this is unsigned right shift
    (*((uint64_t*)(arr+firstByteIdx+1))) = valIter;
}

inline int64_t unsigned_val(uint64_t x, int bw_x) {
    uint64_t mask_x = (bw_x == 64 ? -1: ((1ULL << bw_x) - 1));
    return x & mask_x;
}

inline int64_t signed_val(uint64_t x, int bw_x) {
    uint64_t pow_x = (bw_x == 64? 0ULL: (1ULL << bw_x));
    uint64_t mask_x = pow_x - 1;
    x = x & mask_x;
    return int64_t(x - ((x >= (pow_x/2)) * pow_x));
}
