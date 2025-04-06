#ifndef THROW_H

#define THROW_H

/**
 * Call to immediately halt execution with an error message.
 * @param fmt The format of the error message.
 * @param ... Variables to be substituted into the error message.
 */
__attribute__((noreturn)) void throw(const char *const fmt, ...);

#endif
