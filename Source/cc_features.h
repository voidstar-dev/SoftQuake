#ifndef _CC_FEATURES_H_
#define _CC_FEATURES_H_

// Special compiler features go here


#if defined __GNUC__ || defined __clang__

// The compiler will error-check functions annotated with this attribute just like printf
#define PRINTF_FUNCTION __attribute__((format (printf, 1, 2)))
#define SNPRINTF_FUNCTION __attribute__((format (printf, 3, 4)))

#define NORETURN_FUNCTION __attribute__((noreturn))

#else

#define PRINTF_FUNCTION
#define SNPRINTF_FUNCTION

#define NORETURN_FUNCTION

#endif /* __GNUC__, __clang__ */


#endif /* _CC_FEATURES_H_ */
