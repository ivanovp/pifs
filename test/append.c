#include <stdio.h>
#include <stdint.h>

#define ASCII_CR    (13)
#define ASCII_LF    (10)
#define PRINT_BUF_COLUMNS               (16)

void print_hex_byte(uint8_t byte)
{
    printf("%02X", byte);
}

/*
 * Print buffer in hexadecimal and ASCII format to the UART interface.
 *
 * @param[in] buf       Buffer to print.
 * @param[in] buf_size  Length of buffer.
 * @param[in] address   Address of buffer
 */
void print_buffer (const void * a_buf, size_t a_buf_size, uint32_t a_addr)
{
    uint8_t * buf = (uint8_t*) a_buf;
    size_t  i, j, s;

    /* Extend buffer size to be dividable with PRINT_BUF_COLUMNS */
    if (a_buf_size % PRINT_BUF_COLUMNS == 0)
    {
        s = a_buf_size;
    }
    else
    {
        s = a_buf_size + PRINT_BUF_COLUMNS - a_buf_size % PRINT_BUF_COLUMNS;
    }

    print_hex_byte(a_addr >> 24);
    print_hex_byte(a_addr >> 16);
    print_hex_byte(a_addr >> 8);
    print_hex_byte(a_addr);
    putchar(' ');
    for (i = 0 ; i < s ; i++, a_addr++)
    {
        /* Print buffer in hexadecimal format */
        if (i < a_buf_size)
        {
            print_hex_byte(buf[i]);
        }
        else
        {
            putchar(' ');
            putchar(' ');
        }

        putchar(' ');
        if ((i + 1) % PRINT_BUF_COLUMNS == 0)
        {
            /* Print buffer in ASCII format */
            putchar(' ');
            putchar(' ');
            for (j = i - (PRINT_BUF_COLUMNS - 1) ; j <= i ; j++)
            {
                if (j < a_buf_size)
                {
                    uint8_t  data = buf[j];
                    if ((data >= 0x20) && (data <= 0x7F))
                    {
                        putchar(data);
                    }
                    else
                    {
                        putchar('.');
                    }
                }
                else
                {
                    putchar(' ');
                }
            }

            putchar(ASCII_CR);
            putchar(ASCII_LF);
            if (j < a_buf_size)
            {
                print_hex_byte((a_addr + 1) >> 24);
                print_hex_byte((a_addr + 1) >> 16);
                print_hex_byte((a_addr + 1) >> 8);
                print_hex_byte((a_addr + 1));
                putchar(' ');
            }
        }
    }

    putchar(ASCII_CR);
    putchar(ASCII_LF);
} /* print_buf */

int main(void)
{
    FILE * f;
    uint8_t buf[] = "Ez egy teszt.";
    uint8_t buf2[] = "Ez egy masik teszt.";
    uint8_t buf3[] = "Ez a harmadik teszt.";
    uint8_t buf_r[13] = { 0 };

    f = fopen("append.bin", "w");
    fwrite(buf, 1, sizeof(buf), f);
    fclose(f);

    f = fopen("append.bin", "a+");
    fwrite(buf2, 1, sizeof(buf2), f);
    fseek(f, 0, SEEK_SET);
    fread(buf_r, 1, sizeof(buf_r), f);
    fwrite(buf3, 1, sizeof(buf3), f);
    print_buffer(buf_r, sizeof(buf_r), 0);
    fclose(f);
    return 0;
}
