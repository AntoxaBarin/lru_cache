#pragma once
#include <vector>
#include <utility>
#include <string>

const int MEM_SIZE = 512 * 1024;      // 512 Kbyte
const int ADDR_LEN = 19;              // 19 bit
const int CACHE_WAY = 4;
const int CACHE_TAG_LEN = 10;         // 10 bit
const int CACHE_IDX_LEN = 4;          // 4 bit
const int CACHE_OFFSET_LEN = 5;       // 5 bit
const int CACHE_SIZE = 2 * 1024;      // 2 Kbyte
const int CACHE_LINE_SIZE = 32;       // 32 byte
const int CACHE_LINE_COUNT = 64;
const int CACHE_SETS_COUNT = 16;

const int DATA1_BUS_LEN = 2;           // 16 bit
const int DATA2_BUS_LEN = 2;           // 16 bit
const int CTR1_BUS_LEN = 3;            // 3 bit
const int CTR2_BUS_LEN = 2;            // 2 bit
const int ADDR1_BUS_LEN = 19;          // 19 bit
const int ADDR2_BUS_LEN = 14;          // 14 bit

const int CACHE_MISS_DELAY = 4;       // Время, через которое память начинает отвечать кэш при промахе
const int CACHE_HIT_DELAY = 6;        // Время, через которое память начинает отвечать кэш при попадании
const int MEM_DELAY = 100;            // Время, через которое память начинает отвечать память

struct CacheLine {
    int  tag;            // тег кэш-линии
    int  age;            // LRU - число от 0 до 3 (CACHE_WAY - 1). pLRU - число 0 или 1.
    bool is_valid;       // валидна ли кэш-линия
    bool is_dirty;       // изменены ли данные в кэш-линии
};

class Cache {
public:
    Cache(std::string policy);
    std::pair<int, int> parseAddress(int address);
    int find_data_in_cache(int address);

    void RESET();
    void change_policy();
    void WRITE_NEW_LINE(int address);
    void update_order_LRU(int line_index, int set_index);
    void update_order_pLRU(int line_index, int set_index);

    void C1_READ(int address, int bytes);
    void C1_WRITE(int address, int bytes);
    void C1_RESPONSE(int address);
    void C2_READ_LINE(int address);
    void C2_WRITE_LINE();

    void mult_operation();
    void one_tact_operation();

    int get_tacts();
    float get_hit_percentage();
private:
    CacheLine cache_data[CACHE_SETS_COUNT][CACHE_WAY];
    std::string policy_;
    int tacts_;
    int hits_;
    int misses_;
};