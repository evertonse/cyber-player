
typedef unsigned long long int u64;

u64 cool_sqrtu64(u64 n) {
    u64 s  = 0;          // sqrt result
    u64 s2 = 0;          // s^2 during algorithm
    int i  = 31;         // bit index
    u64 m  = 0x80000000; // bit mask = 1<<i

    while (m) {
        u64 s2_new = s2 + (s << (i + 1)) + ((u64)1 << (i << 1));
        if (s2_new <= n) {
            s2 = s2_new;
            s ^= m;
        }
        i--;
        m >>= 1;
    }

    return s;
}
