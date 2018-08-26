#ifndef __dbg_h__
#define __dbg_h__

/**
 * @file    dbg.h
 * @version 1.0.0
 * @date    2016-02-06
 *
 * @author  Zed Shaw
 *
 * @brief   Zed Shaw's awesome Debug macros.
 *
 * These macros are meant to expedite the Debugging process by allowing
 * convenient means to log errors and other Debugging information. They also
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
 * @brief   Logs Debugging information into \c stderr and can be compiled out.
 *
 * If \c NDEBUG is defined (usually during compilation by the cc flag
 * \c DNDEBUG), all \c Debug calls will be removed from the code at compile
 * time. Leaving this macro in code, therefore, will not effect the final
 * product while allowing for quick recall of all Debugging information.
 *
 * @param   M   Message, as a char *, using \c printf formating
 * @param   ... Various parameters as would be passed to \c printf
 */
#ifdef NDEBUG
#define Debug(M, ...)
#else
#define Debug(M, ...) fprintf(stderr, "Debug %s:%d: " M "\n", __FILE__, __LINE__, ##__VA_ARGS__)
#endif

/**
 * @brief   A simple \c errno wrapper.
 *
 * The main purpose is for simplifying the other \c dbg macros.
 *
 * @return  Error message
 */
#define CleanErrno() (errno == 0 ? "None" : strerror(errno))

/**
 * @brief   Logs an error into stderr.
 *
 * @param   M   Message, as a char *, using \c printf formating
 * @param   ... Various parameters as would be passed to \c printf
 */
#define Error(M, ...) fprintf(stderr, "[ERROR] (%s:%d: errno: %s) " M "\n", __FILE__, __LINE__, CleanErrno(), ##__VA_ARGS__)

/**
 * @brief   Logs a warning into \c stderr.
 *
 * @param   M   Message, as a char *, using \c printf formating
 * @param   ... Various parameters as would be passed to \c printf
 */
#define Warn(M, ...) fprintf(stderr, "[WARN] (%s:%d: errno: %s) " M "\n", __FILE__, __LINE__, CleanErrno(), ##__VA_ARGS__)

/**
 * @brief   Logs useful information into \c stderr.
 *
 * @param   M   Message, as a char *, using \c printf formating
 * @param   ... Various parameters as would be passed to \c printf
 */
#define Info(M, ...) fprintf(stderr, "[INFO] (%s:%d) " M "\n", __FILE__, __LINE__, ##__VA_ARGS__)

/**
 * @brief   Performs a Check on \a A and logs an error if false.
 *
 * This Checks if \a A is false, and if so, logs message \a M into \c stderr
 * and aborts the function.
 *
 * NOTE: This macro demands that an \c error: tag be present in the code.
 *
 * @param   A   Expression to Check
 * @param   M   Message, as a char *, using \c printf formating
 * @param   ... Various parameters as would be passed to \c printf
 */
#define Check(A, M, ...) if(!(A)) { Error(M, ##__VA_ARGS__); errno=0; goto error;}

/**
 * @brief   Marks a point that should never be reached.
 *
 * Place \c Sentinel anywhere that should never be reached. If \c Sentinel is
 * ever called, then something can be assumed to have gone wrong. If ever
 * reached, \c sentinel logs the message \a M into \c stderr and aborts the
 * function.
 *
 * NOTE: This macro demands that an \c error: tag be present in the code.
 *
 * @param   M   Message, as a char *, using \c printf formating
 * @param   ... Various parameters as would be passed to \c printf
 */
#define Sentinel(M, ...) { Error(M, ##__VA_ARGS__); errno=0; goto error; }

/**
 * @brief   Specialization of \c Check for Checking memory allocation.
 *
 * Behaves exactly like \c Check, but has a default message. This is meant
 * mostly for convenience.
 *
 * NOTE: This macro demands that an \c error: tag be present in the code.
 *
 * @see Check()
 *
 * @param   A   Pointer to memory location
 */
#define CheckMemory(A) Check((A), "Out of memory.")

/**
 * @brief   Specialization of \c Check that logs using \a Debug.
 *
 * This behaves almost exactly like \c Check, but rather than logging an error
 * into \c stderr, the \c Debug macro is used. This means that defining
 * \c NDEBUG results in this macro not printing any output. Note that this macro
 * still aborts the function.
 *
 * NOTE: This macro demands that an \c error: tag be present in the code.
 *
 * @see Check()
 * @see Debug()
 *
 * @param   A   Expression to Check
 * @param   M   Message, as a char *, using \c printf formating
 * @param   ... Various parameters as would be passed to \c printf
 */
#define CheckDebug(A, M, ...) if(!(A)) { Debug(M, ##__VA_ARGS__); errno=0; goto error; }

#endif
