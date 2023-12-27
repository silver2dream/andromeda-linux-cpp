#include <cstdarg>
#include <cstdint>
#include <cstring>

#include "andro_func.h"
#include "andro_macro.h"

static u_char* sprintf_num(u_char* buffer, u_char* last, uint64_t ui64, u_char zero, uintptr_t hexadecimal, uintptr_t width);

u_char* slprintf(u_char* buffer, u_char* last, const char* fmt, ...) {
    va_list args;
    u_char* p;

    va_start(args, fmt);
    p = vslprintf(buffer, last, fmt, args);
    va_end(args);
    return p;
}

u_char* snprintf(u_char* buffer, size_t max, const char* fmt, ...) {
    u_char* p;
    va_list args;

    va_start(args, fmt);
    p = vslprintf(buffer, buffer + max, fmt, args);
    va_end(args);
    return p;
}

u_char* vslprintf(u_char* buffer, u_char* last, const char* fmt, va_list args) {
    u_char zero;

    uintptr_t width, sign, hex, frac_width, scale, n;

    int64_t i64;    // Save the corresponding variable parameters for %d
    uint64_t ui64;  // Save the corresponding variable parameters for %ud,
                    // temporarily using it as the integer part of the %f variable parameter is also acceptable.

    u_char* p;      // Save the corresponding variable parameters for %s
    double f;       // Save the corresponding variable parameters for %f
    uint64_t frac;  // For the %f variable parameter, obtain the content after the 2 decimal places based on %.2f, etc.

    while (*fmt && buffer < last) {
        if (*fmt == '%') {
            zero = (u_char)((*++fmt == '0') ? '0' : ' ');

            width = 0;       // If there is a number following the format character %, it will be stored in the width variable. Currently, this is only effective for numeric formats, such as %d and %f.
            sign = 1;        // Display whether it is a signed number; if it is, the value will be 1, unless %u is used, where u represents an unsigned number.
            hex = 0;         // Indicate whether to display in hexadecimal format (e.g., displaying addresses); 0: no, 1: yes, and display lowercase letters a-f, 2: yes, and display uppercase letters A-F.
            frac_width = 0;  // The number of digits after the decimal point is usually used in combination with %.10f, where 10 is the frac_width.
            i64 = 0;
            ui64 = 0;

            // This while loop is used to determine whether there is a number following the % character.
            // If there is a number, it will extract the number.
            // For example, in %16, the loop will eventually extract 16 and place it into the width field.
            // in %16d, the final width value will be 16.
            while (*fmt >= '0' && *fmt <= '9') {
                // (*fmt++ - '0') is a way to convert a character to its corresponding numeric value.
                // For example, the result of '8' - '0' is 8 (subtracting the ASCII codes of the characters),
                // so the purpose of (*fmt++ - '0') is to convert the character 8 into the number 8.
                width = width * 10 + (*fmt++ - '0');
            }

            for (;;) {
                switch (*fmt) {
                    case 'u':
                        sign = 0;
                        fmt++;
                        continue;
                    case 'X':
                        hex = 2;
                        sign = 0;
                        fmt++;
                        continue;
                    case 'x':
                        hex = 1;
                        sign = 0;
                        fmt++;
                        continue;
                    case '.':
                        fmt++;
                        while (*fmt >= '0' && *fmt <= '9') {
                            frac_width = frac_width * 10 + (*fmt++ - '0');
                        }
                        break;
                    default:
                        break;
                }
                break;
            }

            switch (*fmt) {
                case '%':
                    *buffer++ = '%';
                    fmt++;
                    continue;
                case 'd':
                    if (sign) {
                        i64 = (int64_t)va_arg(args, int);
                    } else {
                        ui64 = (uint64_t)va_arg(args, u_int);
                    }
                    break;
                case 'i':
                    if (sign) {
                        i64 = (int64_t)va_arg(args, intptr_t);
                    } else {
                        ui64 = (uint64_t)va_arg(args, uintptr_t);
                    }
                    break;
                case 's':
                    p = va_arg(args, u_char*);
                    while (*p && buffer < last) {
                        *buffer++ = *p++;
                    }
                    fmt++;
                    continue;
                case 'P':
                    i64 = (int64_t)va_arg(args, pid_t);
                    sign = 1;
                    break;
                case 'f':
                    f = va_arg(args, double);
                    if (f < 0) {
                        *buffer++ = '-';
                        f = -f;
                    }
                    ui64 = (int64_t)f;
                    frac = 0;

                    if (frac_width) {
                        scale = 1;
                        for (n = frac_width; n; n--) {
                            scale *= 10;
                        }

                        frac = (uint64_t)((f - (double)ui64) * scale + 0.5);

                        if (frac == scale) {
                            ui64++;
                            frac = 0;
                        }

                        buffer = sprintf_num(buffer, last, ui64, zero, 0, width);

                        if (frac_width) {
                            if (buffer < last) {
                                *buffer++ = '.';
                            }
                            buffer = sprintf_num(buffer, last, frac, '0', 0, frac_width);
                        }
                        fmt++;
                        continue;
                    }
                default:
                    *buffer++ = *fmt++;
                    continue;
            }

            if (sign) {
                if (i64 < 0) {
                    *buffer++ = '-';
                    ui64 = (uint64_t)-i64;
                } else {
                    ui64 = (uint64_t)i64;
                }
            }
            buffer = sprintf_num(buffer, last, ui64, zero, hex, width);
            fmt++;
        } else {
            *buffer++ = *fmt++;
        }
    }

    return buffer;
}

static u_char* sprintf_num(u_char* buffer, u_char* last, uint64_t ui64, u_char zero, uintptr_t hexadecimal, uintptr_t width) {
    u_char *p, temp[ANDRO_INT64_LEN + 1];
    size_t len;
    uint32_t ui32;

    static u_char hex[] = "0123456789abcdef";
    static u_char HEX[] = "0123456789ABCDEF";

    p = temp + ANDRO_INT64_LEN;

    if (hexadecimal == 0) {
        if (ui64 <= (uint64_t)ANDRO_MAX_UINT32_VALUE) {
            ui32 = (uint32_t)ui64;
            do {
                *--p = (u_char)(ui32 % 10 + '0');
            } while (ui32 /= 10);
        } else {
            do {
                *--p = (u_char)(ui64 % 10 + '0');
            } while (ui64 /= 10);
        }
    } else if (hexadecimal == 1) {
        do {
            *--p = hex[(uint32_t)(ui64 & 0xf)];
        } while (ui64 >> 4);
    } else {
        do {
            *--p = HEX[(uint32_t)(ui64 & 0xf)];
        } while (ui64 >>= 4);
    }

    len = (temp + ANDRO_INT64_LEN) - p;

    while (len++ < width && buffer < last) {
        *buffer++ = zero;
    }

    len = (temp + ANDRO_INT64_LEN) - p;
    if ((buffer + len) >= last) {
        len = last - buffer;
    }

    return Andro_Cpy_Mem(buffer, p, len);
}