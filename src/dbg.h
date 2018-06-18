#ifndef __dbg_h__
#define __dbg_h__

/**
 * @file    dbg.h
 * @version 1.0.0
 * @date    2016-02-06
 *
 * @author  Zed Shaw
 *
 * @brief   Zed Shaw's awesome debug macros.
 *
 * These macros are meant to expedite the debugging process by allowing
 * convenient means to log errors and other debugging information. They also
 * provide a useful error control mechanism through an \c "error:" jump point.
 * Within this context, any macro that "aborts the function" does so by jumping
 * directly to \c error. After this point, anything that should be dealt with
 * prior to returning from the function should be cleaned up and an appropriate
 * return value should be given.
 */

#include <stdio.h>
#include <errno.h>
#include <string.h>

/**
 * @brief   Logs debugging information into \c stderr and can be compiled out.
 *
 * If \c NDEBUG is defined (usually during compilation by the cc flag
 * \c -DNDEFINE), all \c debug calls will be removed from the code at compile
 * time. Leaving this macro in code, therefore, will not effect the final
 * product while allowing for quick recall of all debugging information.
 *
 * @param   M   Message, as a char *, using \c printf formating
 * @param   ... Various parameters as would be passed to \c printf
 */
#ifdef NDEBUG
#define debug(M, ...)
#else
#define debug(M, ...) fprintf(stderr, "DEBUG %s:%d: " M "\n", __FILE__, __LINE__, ##__VA_ARGS__)
#endif

/**
 * @brief   A simple \c errno wrapper.
 *
 * The main purpose is for simplifying the other \c dbg macros.
 *
 * @return  Error message
 */
#define clean_errno() (errno == 0 ? "None" : strerror(errno))

/**
 * @brief   Logs an error into stderr.
 *
 * @param   M   Message, as a char *, using \c printf formating
 * @param   ... Various parameters as would be passed to \c printf
 */
#define log_err(M, ...) fprintf(stderr, "[ERROR] (%s:%d: errno: %s) " M "\n", __FILE__, __LINE__, clean_errno(), ##__VA_ARGS__)

/**
 * @brief   Logs a warning into \c stderr.
 *
 * @param   M   Message, as a char *, using \c printf formating
 * @param   ... Various parameters as would be passed to \c printf
 */
#define log_warn(M, ...) fprintf(stderr, "[WARN] (%s:%d: errno: %s) " M "\n", __FILE__, __LINE__, clean_errno(), ##__VA_ARGS__)

/**
 * @brief   Logs useful information into \c stderr.
 *
 * @param   M   Message, as a char *, using \c printf formating
 * @param   ... Various parameters as would be passed to \c printf
 */
#define log_info(M, ...) fprintf(stderr, "[INFO] (%s:%d) " M "\n", __FILE__, __LINE__, ##__VA_ARGS__)

/**
 * @brief   Performs a check on \a A and logs an error if false.
 *
 * This checks if \a A is false, and if so, logs message \a M into \c stderr
 * and aborts the function.
 *
 * NOTE: This macro demands that an \c error: tag be present in the code.
 *
 * @param   A   Expression to check
 * @param   M   Message, as a char *, using \c printf formating
 * @param   ... Various parameters as would be passed to \c printf
 */
#define check(A, M, ...) if(!(A)) { log_err(M, ##__VA_ARGS__); errno=0; goto error;}

/**
 * @brief   Marks a point that should never be reached.
 *
 * Place \c sentinel anywhere that should never be reached. If sentinel is ever
 * called, then something can be assumed to have gone wrong. If ever reached,
 * \c sentinel logs the message \a M into \c stderr and aborts the function.
 *
 * NOTE: This macro demands that an \c error: tag be present in the code.
 *
 * @param   M   Message, as a char *, using \c printf formating
 * @param   ... Various parameters as would be passed to \c printf
 */
#define sentinel(M, ...) { log_err(M, ##__VA_ARGS__); errno=0; goto error; }

/**
 * @brief   Specialization of \c check for checking memory allocation.
 *
 * Behaves exactly like \c check, but has a default message. This is meant
 * mostly for convenience.
 *
 * NOTE: This macro demands that an \c error: tag be present in the code.
 *
 * @see check()
 *
 * @param   A   Pointer to memory location
 */
#define check_mem(A) check((A), "Out of memory.")

/**
 * @brief   Specialization of \c check that logs using \a debug.
 *
 * This behaves almost exactly like \c check, but rather than logging an error
 * into \c stderr, the \c debug macro is used. This means that defining
 * \c NDEBUG results in this macro not printing any output. Note that this macro
 * still aborts the function.
 *
 * NOTE: This macro demands that an \c error: tag be present in the code.
 *
 * @see check()
 * @see debug()
 *
 * @param   A   Expression to check
 * @param   M   Message, as a char *, using \c printf formating
 * @param   ... Various parameters as would be passed to \c printf
 */
#define check_debug(A, M, ...) if(!(A)) { debug(M, ##__VA_ARGS__); errno=0; goto error; }

#endif
