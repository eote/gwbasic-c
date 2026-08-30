#include <stdio.h>
int main() {
    printf("Test 1: %.*f\n", 2, 1234.56);
    printf("Test 2: %.2f\n", 1234.56);
    printf("Test 3: %f\n", 1234.56);
    printf("Test 4: %g\n", 1234.56);
    return 0;
}
