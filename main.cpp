#include "Cache.h"
#include <cstdio>
#include <cstdint>

#define M 64
#define N 60
#define K 32
int8_t a[M][K];
int16_t b[K][N];
int32_t c[M][N];

Cache cache("LRU");

void mmul() {
    int cur_a_address = 0;
    int cur_c_address = M * K + K * N * sizeof(std::int16_t);

    std::int8_t *pa = (std::int8_t *) a;
    cache.one_tact_operation();   // pa initialization

    std::int32_t *pc = (std::int32_t *) c;
    cache.one_tact_operation();   // pc initialization

    cache.one_tact_operation();   // y initialization
    for (int y = 0; y < M; y++) {
        cache.one_tact_operation();   // y increment
        cache.one_tact_operation();   // new for-loop iteration

        cache.one_tact_operation();   // x initialization
        for (int x = 0; x < N; x++) {
            cache.one_tact_operation();   // x increment
            cache.one_tact_operation();   // new for-loop iteration

            std::int16_t *pb = (std::int16_t *) b;
            cache.one_tact_operation();   // pb initialization

            int cur_b_address = M * K;
            std::int32_t s = 0;
            cache.one_tact_operation();   // s initialization

            cache.one_tact_operation();   // k initialization
            for (int k = 0; k < K; k++) {
                cache.one_tact_operation();   // k increment
                cache.one_tact_operation();   // new for-loop iteration

                s += pa[k] * pb[x];
                cache.one_tact_operation();   // s +=
                cache.mult_operation();       // pa[k] * pb[x]

                cache.C1_READ(cur_a_address + k, 1);
                cache.C1_READ(cur_b_address + 2 * x, 2);

                pb += N;
                cache.one_tact_operation(); // pb += N
                cur_b_address += 2 * N;
            }
            pc[x] = s;
            cache.C1_WRITE(cur_c_address + sizeof(std::int32_t) * x, 4);
        }
        pa += K;
        cache.one_tact_operation();   // pa += K
        cur_a_address += K;

        pc += N;
        cache.one_tact_operation();   // pc += N
        cur_c_address += 4 * N;
    }
    cache.one_tact_operation(); // exit from function
}


int main() {
    mmul();
    float LRU_percent = cache.get_hit_percentage();
    int LRU_tacts = cache.get_tacts();

    cache.change_policy();
    cache.RESET();

    mmul();
    float pLRU_percent = cache.get_hit_percentage();
    int pLRU_tacts = cache.get_tacts();

    printf("LRU: \thit perc. %3.4f%% \ttime: %u \npLRU: \thit perc. %3.4f%% \ttime: %u \n", LRU_percent, LRU_tacts, pLRU_percent, pLRU_tacts);

    return 0;
}