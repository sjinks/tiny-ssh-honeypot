#ifndef A0CC6AE9_A96E_46F5_9D0F_064F50D0CB15
#define A0CC6AE9_A96E_46F5_9D0F_064F50D0CB15

extern int terse_logs;

#if defined(__clang__) || defined(__GNUC__)
__attribute__((format(printf, 1, 2)))
#endif
void my_log(const char *format, ...);

#endif /* A0CC6AE9_A96E_46F5_9D0F_064F50D0CB15 */
