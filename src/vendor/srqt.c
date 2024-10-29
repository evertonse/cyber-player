uint64_t cool_sqrtu64(uint64_t n){
    uint64_t s  = 0 ;         // sqrt result
    uint64_t s2 = 0 ;         // s^2 during algorithm
    int      i  = 31;         // bit index
    uint64_t m  = 0x80000000; // bit mask = 1<<i

    while (m) {
        uint64_t s2_new = s2+(s<<(i+1))+((uint64_t)1<<(i<<1));
        if(s2_new <= n){
            s2 = s2_new;
            s ^= m;
        }
        i--;
        m>>=1;
    }
    
    return s;
}
