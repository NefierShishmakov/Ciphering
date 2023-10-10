#include <fstream>
#include <streambuf>
#include <iostream>
#include <string>
#include <cstdint>
#include <random>
#include <cstring>

uint32_t A = 0x67452301;
uint32_t B = 0xefcdab89;
uint32_t C = 0x98badcfe;
uint32_t D = 0x10325476;

void init_T(uint32_t *T) {
    for(int i = 0; i < 64; i++) {
        T[i] = (uint32_t)(pow(2,32)*fabs(sin(i + 1)));
    }
}

uint32_t rotate_left(uint32_t value, uint32_t shift) {
    return value << shift | value >> (32-shift);
}

uint32_t F(uint32_t x, uint32_t y, uint32_t z) {
    return (x & y) | (~x & z);
}

uint32_t G(uint32_t x, uint32_t y, uint32_t z) {
    return (x & z) | (~z & y);
}

uint32_t H(uint32_t x, uint32_t y, uint32_t z) {
    return x ^ y ^ z;
}

uint32_t I(uint32_t x, uint32_t y, uint32_t z) {
    return y ^ (~z | x);
}

std::string get_file_contents(const char *filename)
{
  std::ifstream in(filename, std::ios::in | std::ios::binary);
  std::string contents;
  in.seekg(0, std::ios::end);
  contents.reserve(in.tellg());
  in.seekg(0, std::ios::beg);
  contents.assign((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
  in.close();
  return (contents);
}

static size_t get_file_content_size_in_bytes(std::string &file_content) {
    return strlen(file_content.c_str());
}

static size_t get_result_md5_hash_size(size_t size) {
    int rest = size % 64;
    return (rest < 56) ? size - rest + 64: size - rest + 128;
}

static void prepare_md5_hash(uint8_t *md5_hash, size_t result_size, uint8_t *file_text, size_t size) {
    std::memcpy(md5_hash, file_text, size);
    md5_hash[size] = 0x80;
    for (size_t i = size + 1; i < result_size - 8; ++i) {
        md5_hash[i] = 0x00;
    }

    uint64_t size_bits = size * 8;
    for (int i = 0; i < 8; ++i) {
        md5_hash[result_size - 8 + i] = size_bits >> (i * 8);
    }
}

void process_block(uint32_t *block, uint32_t *T) {
    uint32_t AA = A;
    uint32_t BB = B;
    uint32_t CC = C;
    uint32_t DD = D;
    // на википедии есть алгоритм, как это сделать через цикл, но там ничего не понятно.
    // поэтому просто решил в тупую пойти, зато всё наглядно.
    // прогоняем 1-ый раунд
    A = B + rotate_left((A + F(B, C, D) + block[0] + T[0]), 7);
    D = A + rotate_left((D + F(A, B, C) + block[1] + T[1]), 12);
    C = D + rotate_left((C + F(D, A, B) + block[2] + T[2]), 17);
    B = C + rotate_left((B + F(C, D, A) + block[3] + T[3]), 22);

    A = B + rotate_left((A + F(B, C, D) + block[4] + T[4]), 7);
    D = A + rotate_left((D + F(A, B, C) + block[5] + T[5]), 12);
    C = D + rotate_left((C + F(D, A, B) + block[6] + T[6]), 17);
    B = C + rotate_left((B + F(C, D, A) + block[7] + T[7]), 22);

    A = B + rotate_left((A + F(B, C, D) + block[8] + T[8]), 7);
    D = A + rotate_left((D + F(A, B, C) + block[9] + T[9]), 12);
    C = D + rotate_left((C + F(D, A, B) + block[10] + T[10]), 17);
    B = C + rotate_left((B + F(C, D, A) + block[11] + T[11]), 22);

    A = B + rotate_left((A + F(B, C, D) + block[12] + T[12]), 7);
    D = A + rotate_left((D + F(A, B, C) + block[13] + T[13]), 12);
    C = D + rotate_left((C + F(D, A, B) + block[14] + T[14]), 17);
    B = C + rotate_left((B + F(C, D, A) + block[15] + T[15]), 22);

    // прогоняем 2-ой раунд
    A = B + rotate_left((A + G(B, C, D) + block[1] + T[16]), 5);
    D = A + rotate_left((D + G(A, B, C) + block[6] + T[17]), 9);
    C = D + rotate_left((C + G(D, A, B) + block[11] + T[18]), 14);
    B = C + rotate_left((B + G(C, D, A) + block[0] + T[19]), 20);

    A = B + rotate_left((A + G(B, C, D) + block[5]+ T[20]), 5);
    D = A + rotate_left((D + G(A, B, C) + block[10] + T[21]), 9);
    C = D + rotate_left((C + G(D, A, B) + block[15] + T[22]), 14);
    B = C + rotate_left((B + G(C, D, A) + block[4] + T[23]), 20);

    A = B + rotate_left((A + G(B, C, D) + block[9] + T[24]), 5);
    D = A + rotate_left((D + G(A, B, C) + block[14] + T[25]), 9);
    C = D + rotate_left((C + G(D, A, B) + block[3] + T[26]), 14);
    B = C + rotate_left((B + G(C, D, A) + block[8] + T[27]), 20);

    A = B + rotate_left((A + G(B, C, D) + block[13] + T[28]), 5);
    D = A + rotate_left((D + G(A, B, C) + block[2] + T[29]), 9);
    C = D + rotate_left((C + G(D, A, B) + block[7] + T[30]), 14);
    B = C + rotate_left((B + G(C, D, A) + block[12] + T[31]), 20);

    // прогоняем 3-ий раунд
    A = B + rotate_left((A + H(B, C, D) + block[5] + T[32]), 4);
    D = A + rotate_left((D + H(A, B, C) + block[8] + T[33]), 11);
    C = D + rotate_left((C + H(D, A, B) + block[11] + T[34]), 16);
    B = C + rotate_left((B + H(C, D, A) + block[14] + T[35]), 23);

    A = B + rotate_left((A + H(B, C, D) + block[1] + T[36]), 4);
    D = A + rotate_left((D + H(A, B, C) + block[4] + T[37]), 11);
    C = D + rotate_left((C + H(D, A, B) + block[7] + T[38]), 16);
    B = C + rotate_left((B + H(C, D, A) + block[10] + T[39]), 23);

    A = B + rotate_left((A + H(B, C, D) + block[13] + T[40]), 4);
    D = A + rotate_left((D + H(A, B, C) + block[0] + T[41]), 11);
    C = D + rotate_left((C + H(D, A, B) + block[3] + T[42]), 16);
    B = C + rotate_left((B + H(C, D, A) + block[6] + T[43]), 23);

    A = B + rotate_left((A + H(B, C, D) + block[9] + T[44]), 4);
    D = A + rotate_left((D + H(A, B, C) + block[12] + T[45]), 11);
    C = D + rotate_left((C + H(D, A, B) + block[15] + T[46]), 16);
    B = C + rotate_left((B + H(C, D, A) + block[2] + T[47]), 23);

    // прогоняем 4-ый раунд
    A = B + rotate_left((A + I(B, C, D) + block[0] + T[48]), 6);
    D = A + rotate_left((D + I(A, B, C) + block[7] + T[49]), 10);
    C = D + rotate_left((C + I(D, A, B) + block[14] + T[50]), 15);
    B = C + rotate_left((B + I(C, D, A) + block[5] + T[51]), 21);

    A = B + rotate_left((A + I(B, C, D) + block[12] + T[52]), 6);
    D = A + rotate_left((D + I(A, B, C) + block[3] + T[53]), 10);
    C = D + rotate_left((C + I(D, A, B) + block[10] + T[54]), 15);
    B = C + rotate_left((B + I(C, D, A) + block[1] + T[55]), 21);

    A = B + rotate_left((A + I(B, C, D) + block[8] + T[56]), 6);
    D = A + rotate_left((D + I(A, B, C) + block[15] + T[57]), 10);
    C = D + rotate_left((C + I(D, A, B) + block[6] + T[58]), 15);
    B = C + rotate_left((B + I(C, D, A) + block[13] + T[59]), 21);

    A = B + rotate_left((A + I(B, C, D) + block[4] + T[60]), 6);
    D = A + rotate_left((D + I(A, B, C) + block[11] + T[61]), 10);
    C = D + rotate_left((C + I(D, A, B) + block[2] + T[62]), 15);
    B = C + rotate_left((B + I(C, D, A) + block[9] + T[63]), 21);

    // обновляем наши переменные, в которых будет хеш
    A += AA;
    B += BB;
    C += CC;
    D += DD;
}

// прогоняем все блоки по 512 бит.
void process(uint8_t *md5_hash, size_t result_size) {
    uint32_t T[64];
    init_T(T);
    for (int i = 0; i < result_size; i += 64) {
        uint32_t *block = (uint32_t *)(md5_hash + i);
        process_block(block, T); // прогоняем блок размером 512 бит.
    }
}

std::vector<uint8_t> hashToArr() {
    std::vector<uint8_t> hash(16);
    memcpy(&hash[0], &A, 4);
    memcpy(&hash[4], &B, 4);
    memcpy(&hash[8], &C, 4);
    memcpy(&hash[12], &D, 4);
    return hash;
}

std::string getHexMD5(uint8_t *md5_hash, size_t result_size) {
    process(md5_hash, result_size);
    std::string hex = "0123456789ABCDEF";
    std::string result;

    std::vector<uint8_t> hash = hashToArr();

    for(int i = 0; i < hash.size(); i++) {
        result += hex[hash[i] >> 4];
        result += hex[hash[i] & 0x0F];
    }

    return result;
}

int main() {
    const char *filename = "test.txt";
    std::string file_content = get_file_contents(filename);

    size_t size = get_file_content_size_in_bytes(file_content);
    uint8_t* file_content_in_bytes = const_cast<uint8_t*>(reinterpret_cast<const uint8_t*>(file_content.c_str()));

    size_t result_size = get_result_md5_hash_size(size);
    uint8_t *md5_hash = new uint8_t[result_size];
    prepare_md5_hash(md5_hash, result_size, file_content_in_bytes, size);

    std::cout << getHexMD5(md5_hash, result_size) << '\n';

    delete[] md5_hash;
    return EXIT_SUCCESS;
}