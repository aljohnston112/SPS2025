#include <omp.h>
#include <stdio.h>

int main(void)
{
#ifdef _OPENMP
    printf("OpenMP version: %d\n", _OPENMP);
#else
    printf("OpenMP is not enabled.\n");
#endif
    return 0;
    return 0;
}
