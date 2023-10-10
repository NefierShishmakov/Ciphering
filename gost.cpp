#include <fstream>
#include <streambuf>
#include <iostream>
#include <string>
#include <cstdint>
#include <random>
#include <cstring>

// получаем содержимое всего файла
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

// получаем случайное число от 0 до 255 для генерации рандомного ключа
uint8_t get_random_number_from_0_to_255() {
    std::random_device rd;  
    std::mt19937 gen(rd()); 
    std::uniform_int_distribution<int> dis(0, 255);
    return dis(gen);
}

// генерация рандомного ключа 
uint8_t *generate_256b_random_key() {
    uint8_t *key = new uint8_t[32];
    for (int i = 0; i < 32; ++i) {
        key[i] = get_random_number_from_0_to_255();
    }
    return key;
}

// циклический сдвиг
#define LSHIFT_nBIT(x, L, N) (((x << L) | (x >> (-L & (N - 1)))) & (((uint64_t)1 << N) - 1))

void split_256bits_to_32bits(uint8_t *key256b, uint32_t *keys32b) {
    uint8_t *p8 = key256b;
    for (uint32_t *p32 = keys32b; p32 < keys32b + 8; ++p32) {
        for (uint8_t i = 0; i < 4; ++i) {
            *p32 = (*p32 << 8) | *(p8 + i);
        }
        p8 += 4;
    }
}

void split_64bits_to_32bits(uint64_t block64b, uint32_t *block32b_1, uint32_t *block32b_2) {
    *block32b_2 = (uint32_t)(block64b);
    *block32b_1 = (uint32_t)(block64b >> 32);
}

uint64_t join_8bits_to_64bits(uint8_t *blocks8b) {
    uint64_t block64b;
    for (uint8_t *p = blocks8b; p < blocks8b + 8; ++p) {
        block64b = (block64b << 8) | *p;
    }
    return block64b;
}

static const uint8_t Sbox[8][16] = { 
    {0xF, 0xC, 0x2, 0xA, 0x6, 0x4, 0x5, 0x0, 0x7, 0x9, 0xE, 0xD, 0x1, 0xB, 0x8, 0x3}, 
    {0xB, 0x6, 0x3, 0x4, 0xC, 0xF, 0xE, 0x2, 0x7, 0xD, 0x8, 0x0, 0x5, 0xA, 0x9, 0x1},
    {0x1, 0xC, 0xB, 0x0, 0xF, 0xE, 0x6, 0x5, 0xA, 0xD, 0x4, 0x8, 0x9, 0x3, 0x7, 0x2},
    {0x1, 0x5, 0xE, 0xC, 0xA, 0x7, 0x0, 0xD, 0x6, 0x2, 0xB, 0x4, 0x9, 0x3, 0xF, 0x8},
    {0x0, 0xC, 0x8, 0x9, 0xD, 0x2, 0xA, 0xB, 0x7, 0x3, 0x6, 0x5, 0x4, 0xE, 0xF, 0x1},
    {0x8, 0x0, 0xF, 0x3, 0x2, 0x5, 0xE, 0xB, 0x1, 0xA, 0x4, 0x7, 0xC, 0x9, 0xD, 0x6},
    {0x3, 0x0, 0x6, 0xF, 0x1, 0xE, 0x9, 0x2, 0xD, 0x8, 0xC, 0x4, 0xB, 0xA, 0x5, 0x7},
    {0x1, 0xA, 0x6, 0x8, 0xF, 0xB, 0x0, 0x4, 0xC, 0x3, 0x5, 0x9, 0x7, 0xD, 0x2, 0xE},
};

void substitution_table_by_4bits(uint8_t *blocks4b, uint8_t sbox_row) {
    uint8_t block4b_1, block4b_2;
    for (uint8_t i = 0; i < 4; ++i) {
        block4b_1 = Sbox[sbox_row][blocks4b[i] & 0x0F]; 
        block4b_2 = Sbox[sbox_row][blocks4b[i] >> 4]; 
        blocks4b[i] = block4b_2; 
        blocks4b[i] = (blocks4b[i] << 4) | block4b_1; 
    }
}

uint32_t join_4bits_to_32bits(uint8_t *blocks4b) {
    uint32_t block32b;
    for (uint8_t i = 0; i < 4; ++i) {
        block32b = (block32b << 8) | blocks4b[i];
    }
    return block32b;
}

void split_32bits_to_8bits(uint32_t block32b, uint8_t *blocks8b) {
    for (uint8_t i = 0; i < 4; ++i) {
        blocks8b[i] = (uint8_t)(block32b >> (24 - (i * 8)));
    }
}
// здесь мы используем нашу Sbox - часть алгоритма
uint32_t substitution_table(uint32_t block32b, uint8_t sbox_row) {
    uint8_t blocks4bits[4];
    split_32bits_to_8bits(block32b, blocks4bits);
    substitution_table_by_4bits(blocks4bits, sbox_row);
    return join_4bits_to_32bits(blocks4bits);
}

// Один раунд шифра Фейстера
void round_of_feistel_cipher(uint32_t *block32b_1, uint32_t *block32b_2, uint32_t *keys32b, uint8_t round) {
    uint32_t result_of_iter, temp;

    // RES = (N1 + Ki) mod 2^32
    result_of_iter = (*block32b_1 + keys32b[round % 8]) % UINT32_MAX;
    
    // RES = RES -> Sbox
    result_of_iter = substitution_table(result_of_iter, round % 8);
    
    // RES = RES <<< 11
    result_of_iter = (uint32_t)LSHIFT_nBIT(result_of_iter, 11, 32);

    // N1, N2 = (RES xor N2), N1
    temp = *block32b_1;
    *block32b_1 = result_of_iter ^ *block32b_2;
    *block32b_2 = temp;
}

// ГОСТ-28147-89 работает на базе шифра Фейстера
// keys32b = [K0, K1, K2, K3, K4, K5, K6, K7]
void feistel_cipher(uint8_t mode, uint32_t *block32b_1, uint32_t *block32b_2, uint32_t *keys32b) {
    switch (mode) {
        case 'E': case 'e': {
            // K0, K1, K2, K3, K4, K5, K6, K7, K0, K1, K2, K3, K4, K5, K6, K7, K0, K1, K2, K3, K4, K5, K6, K7
            for (uint8_t round = 0; round < 24; ++round)
                round_of_feistel_cipher(block32b_1, block32b_2, keys32b, round);

            // K7, K6, K5, K4, K3, K2, K1, K0
            for (uint8_t round = 31; round >= 24; --round)
                round_of_feistel_cipher(block32b_1, block32b_2, keys32b, round);
            break;
        }
        case 'D': case 'd': {
            // K0, K1, K2, K3, K4, K5, K6, K7
            for (uint8_t round = 0; round < 8; ++round)
                round_of_feistel_cipher(block32b_1, block32b_2, keys32b, round);

            // K7, K6, K5, K4, K3, K2, K1, K0, K7, K6, K5, K4, K3, K2, K1, K0, K7, K6, K5, K4, K3, K2, K1, K0
            for (uint8_t round = 31; round >= 8; --round)
                round_of_feistel_cipher(block32b_1, block32b_2, keys32b, round);
            break;
        }
    }
}

void split_64bits_to_8bits(uint64_t block64b, uint8_t * blocks8b) {
    for (size_t i = 0; i < 8; ++i) {
        blocks8b[i] = (uint8_t)(block64b >> ((7 - i) * 8));
    }
}

uint64_t join_32bits_to_64bits(uint32_t block32b_1, uint32_t block32b_2) {
    uint64_t block64b;
    block64b = block32b_2;
    block64b = (block64b << 32) | block32b_1;
    return block64b;
}

// Сам алгоритм, возможность зашифровки, дешифровки
size_t GOST_28147(uint8_t *to, uint8_t mode, uint8_t *key256b, uint8_t *from, size_t length) {
    length = length % 8 == 0 ? length : length + (8 - (length % 8));
    uint32_t N1, N2, keys32b[8];
    split_256bits_to_32bits(key256b, keys32b); // получаем 8 ключей для работы

    for (size_t i = 0; i < length; i += 8) {
        split_64bits_to_32bits(
            join_8bits_to_64bits(from + i), 
            &N1, &N2
        ); // разбиваем 64 битный блок на переменный N1, N2 для алгоритмов
        feistel_cipher(mode, &N1, &N2, keys32b);
        split_64bits_to_8bits(
            join_32bits_to_64bits(N1, N2),
            (to + i) // после применения алгоритма в переменных N1, N2 хранится сообщение, пихаем их обратно в to
        );
    }

    return length;
}

static void print_array(uint8_t *array, size_t length) {
    std::cout << "[ ";
    for (size_t i = 0; i < length; ++i) {
        std::cout << array[i] << " ";
    }
    std::cout << "]\n";
}

// получаем размер файла в байтах
static size_t get_file_content_size_in_bytes(std::string &file_content) {
    return strlen(file_content.c_str());
}

int main() {
    const char *filename = "test.txt";
    std::string file_content = get_file_contents(filename);

    size_t position = get_file_content_size_in_bytes(file_content);
    size_t encrypted_text_size_in_byte = position % 8 == 0 ? position : position + (8 - (position % 8));
    uint8_t* file_content_in_bytes = const_cast<uint8_t*>(reinterpret_cast<const uint8_t*>(file_content.c_str()));
    uint8_t *encrypted_text = new uint8_t[encrypted_text_size_in_byte];
    uint8_t *decrypted_text = new uint8_t[encrypted_text_size_in_byte];

    uint8_t *key = generate_256b_random_key();

    std::cout << "Open message:\n";
    print_array(file_content_in_bytes, position);
    std::cout << file_content_in_bytes << "\n\n";

    position = GOST_28147(encrypted_text, 'E', key, file_content_in_bytes, position);
    std::cout << "Encrypted message:\n";
    print_array(encrypted_text, position);
    std::cout << encrypted_text << "\n\n";

    std::cout << "Decrypted message:\n";
    position = GOST_28147(decrypted_text, 'D', key, encrypted_text, position);
    print_array(decrypted_text, position);
    std::cout << decrypted_text << "\n\n";

    delete[] key;
    delete[] encrypted_text;
    delete[] decrypted_text;
    return EXIT_SUCCESS;
}