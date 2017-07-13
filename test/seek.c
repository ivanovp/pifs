#include <stdio.h>
#include <stdint.h>

int main(void)
{
    FILE * f;
    uint8_t buf[] = "Ez egy teszt.";

    f = fopen("seek.bin", "w");
    fseek(f, 100, SEEK_SET);
    fwrite(buf, 1, sizeof(buf), f);
    fseek(f, 50, SEEK_SET);
    fwrite(buf, 1, sizeof(buf), f);
    fclose(f);
    return 0;
}
