#include "Cache.h"
#include <utility>
#include <cmath>

Cache::Cache(std::string policy) {
    policy_ = policy;
    misses_ = 0;
    hits_ = 0;
    tacts_ = 0;
    for (int i = 0; i < CACHE_SETS_COUNT; i++) {
        for (int j = 0; j < CACHE_WAY; j++) {
            cache_data[i][j].is_valid = false;
            cache_data[i][j].is_dirty = false;
            cache_data[i][j].tag = 0;
            cache_data[i][j].age = 0;
        }
    }
}

// Меняем политику вытеснения кэша
void Cache::change_policy() {
    if (policy_ == "LRU") {
        policy_ = "pLRU";
        return;
    }
    else if (policy_ == "pLRU") {
        policy_ = "LRU";
    }
}

// Приводим кэш в начальное состояние
void Cache::RESET() {
    misses_ = 0;
    hits_ = 0;
    tacts_ = 0;
    for (int i = 0; i < CACHE_SETS_COUNT; i++) {
        for (int j = 0; j < CACHE_WAY; j++) {
            cache_data[i][j].is_valid = false;
            cache_data[i][j].is_dirty = false;
            cache_data[i][j].tag = 0;
            cache_data[i][j].age = 0;
        }
    }
}

// Меняем LRU возраст у кэш-линий в блоке (после обращения к линии с возрастом request_age)
// возраст 0 - самое давнее обращение, 3 - самое последнее обращение
void Cache::update_order_LRU(int line_index, int set_index) {
    int request_age = cache_data[set_index][line_index].age;

    if (request_age >= 3) {
        return;
    }

    for (int i = 0; i < CACHE_WAY; i++) {
        if (cache_data[set_index][i].age > request_age) {
            cache_data[set_index][i].age -= 1;
        }
    }
    cache_data[set_index][line_index].age = 3;
}

// Меняем pLRU возраст у кэш-линий в блоке
void Cache::update_order_pLRU(int line_index, int set_index) {
    if (cache_data[set_index][line_index].age == 1) {
        return;
    }

    cache_data[set_index][line_index].age = 1;
    int age_counter = 0;
    for (int i = 0; i < CACHE_WAY; i++) {
        age_counter += cache_data[set_index][i].age;
    }

    if (age_counter == 4) {                   // У всех линий в блоке бит возраста = 1
        for (int i = 0; i < CACHE_WAY; i++) {
            cache_data[set_index][i].age = 0;
        }
        cache_data[set_index][line_index].age = 1;
    }


}

std::pair<int, int> Cache::parseAddress(int address) {
    address >>= CACHE_OFFSET_LEN;  // Убираем нули (offset)
    int index =  address & 0x0F;   // Сохраняем биты для индекса блока
    address >>= CACHE_IDX_LEN;
    int tag = address & 0x3FF;     // Сохраняем биты тега

    return std::make_pair(tag, index);
}

// Ищем кэш-линию в кэше
int Cache::find_data_in_cache(int address) {
    std::pair<int, int> tag_index = parseAddress(address);
    int tag_ = tag_index.first;
    int index = tag_index.second;

    for (int i = 0; i < CACHE_WAY; i++) {
        if (cache_data[index][i].tag == tag_ && cache_data[index][i].is_valid) {
            return i;
        }
    }
    return -1;
}

// кэш читает линию из оперативки
void Cache::C2_READ_LINE(int address) {
    tacts_++;                   // Память отправляет кэшу команду и адрес кэш-линии
    tacts_ += MEM_DELAY;        // Время, через которое память начинает отвечать память
    tacts_ += CACHE_LINE_SIZE / DATA2_BUS_LEN; // Передаем кэш-линию по шине D2
    tacts_++;                                // Память сообщает, что команда завершена
    WRITE_NEW_LINE(address);
}

void Cache::C2_WRITE_LINE() {
    tacts_++;                   // Кэш отправляет команду и адрес памяти
    tacts_ += MEM_DELAY;        // Время, через которое память начинает отвечать память
    tacts_ += CACHE_LINE_SIZE / DATA2_BUS_LEN; // Передаем кэш-линию по шине D2
    tacts_++;                   // Ответ, что кэш-линия записалась в память (C2_RESPONSE)
}

// Записываем новую кэш-линию в кэш
void Cache::WRITE_NEW_LINE(int address) {
    std::pair<int, int> tag_index = parseAddress(address);
    int tag = tag_index.first;
    int index = tag_index.second;

    for (int i = 0; i < CACHE_WAY; i++) {
        if (cache_data[index][i].age == 0 || !cache_data[index][i].is_valid) {
            if (cache_data[index][i].is_dirty && cache_data[index][i].is_valid) {  // Записываем модифицированные данные в память
                C2_WRITE_LINE();
            }

            cache_data[index][i].is_valid = true;
            cache_data[index][i].is_dirty = false;
            cache_data[index][i].tag = tag;                           // Записываем новую кэш-линию

            if (policy_ == "LRU") {
                update_order_LRU(i, index);         // Меняем возраст у линий в блоке
            }
            else if (policy_ == "pLRU") {
                update_order_pLRU(i, index);
            }

            return;
        }
    }
}

void Cache::C1_RESPONSE(int address) {
    std::pair<int, int> tag_index = parseAddress(address);

    int is_data_in_cache = find_data_in_cache(address);
    if (is_data_in_cache != -1) {                              // Данные нашлись в кэше
        tacts_ += CACHE_HIT_DELAY;                             // Время ответа 6 тактов

        hits_++;
        if (policy_ == "pLRU") {
            update_order_pLRU(is_data_in_cache, tag_index.second);
        }
        if (policy_ == "LRU") {
            update_order_LRU(is_data_in_cache, tag_index.second);
        }
    }
    else {                                                       // Кэш промах
        tacts_ += CACHE_MISS_DELAY;                            // Время ответа 4 тактов
        misses_++;
        C2_READ_LINE(address);                              // Кэш посылает запрос к памяти для получения данных
    }
    tacts_++;                                               // Ответ от кэша, что команда выполнена
}


void Cache::C1_READ(int address, int bytes) {
    tacts_++;                                                    // Процессор отправляет команду на чтение кэшу
    C1_RESPONSE(address);
    tacts_ += std::ceil((float)bytes / (float)DATA1_BUS_LEN);      // Время на передачу данных из кэша в процессор
}

void Cache::C1_WRITE(int address, int bytes) {
    tacts_++;                                                     // Процессор отправляет команду на запись кэшу
    C1_RESPONSE(address);
    tacts_ += std::ceil((float)bytes / (float)DATA1_BUS_LEN);   // Время на передачу данных от процессора кэшу

    int cache_line_index = find_data_in_cache(address);
    address >>= CACHE_OFFSET_LEN;
    cache_data[address & 0x0F][cache_line_index].is_dirty = true;     // Изменили данные в кэш-линию
}


void Cache::one_tact_operation() {
    tacts_++;
}

void Cache::mult_operation() {
    tacts_ += 5;
}

int Cache::get_tacts() {
    return tacts_;
}

float Cache::get_hit_percentage() {
    return ((float)(hits_) / (float)(hits_ + misses_)) * 100;
}