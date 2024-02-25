/**
 * Created by jraynor on 2/25/2024.
 */

#include "kern/kernel.h"
#include "core/vlogger.h"
//compute the fibonacci sequence

VAPI i32 fib(const i32 n) {
    if (n <= 1) {
        return n;
    }
    return fib(n - 1) + fib(n - 2);
}


VAPI b8 prime(const i32 n) {
    for (int i = 2; i <= n / 2; i++) {
        if (n % i == 0) {
            return false;
        }
    }
    return true;
}
