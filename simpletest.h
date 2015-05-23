/*! 
    Simple unit testing for c/c++
    Copyright Jerryzhou@outlook.com
    Licence: Apache 2.0

    Project: https://github.com/JerryZhou/simpletest.git
    Purpose: facilitate the unit testing for programs written in c/c++, 
        simple, tiny and easy to use, 
        the whole unit test framework is just one .h file

    Use:
    SIMPLETEST_SUIT(suit);
    SIMPLETEST_CASE(suit, case)
    {
        SIMPLETEST_EQUAL(1, 1);
        SIMPLETEST_EQUAL(2, 1);
    }
 
    run all the test just call:
    runAllTest();

    Please see examples for more details.

    Tests are meant to be in the same place as the tested code.
    Declare SIMPLETEST_NOTEST if you don't want to compile the test code.
    Usually you will declare SIMPLETEST_NOTEST together with NDEBUG.
 
    Thanks for Cosmin Cremarenco
 */

#ifndef __SIMPLETEST_H__
#define __SIMPLETEST_H__

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <assert.h>
#include <string.h>

#define TINTEST_LOG (1)

#define ST_BOOL int
#define ST_TRUE 1
#define ST_FALSE 0

#if TINTEST_LOG
// as android log define : Jerryzhou@outlook.com
enum {
    SC_LOG_UNKNOWN = 0,
    SC_LOG_DEFAULT,    /* only for SetMinPriority() */
    SC_LOG_VERBOSE,
    SC_LOG_DEBUG,
    SC_LOG_INFO,
    SC_LOG_WARN,
    SC_LOG_ERROR,
    SC_LOG_FATAL,
    SC_LOG_SILENT,     /* only for SetMinPriority(); must be last */
};
#define LOG_ENABLE_COLOR (1)

#include <time.h>
#ifdef WIN32
#   include <windows.h>
#else
#   include <sys/time.h>
#endif

#ifdef WIN32
#if defined(_MSC_VER) || defined(_MSC_EXTENSIONS)
#define DELTA_EPOCH_IN_MICROSECS  11644473600000000Ui64
#else
#define DELTA_EPOCH_IN_MICROSECS  11644473600000000ULL
#endif

struct timezone
{
    int  tz_minuteswest; /* minutes W of Greenwich */
    int  tz_dsttime;     /* type of dst correction */
};

int gettimeofday(struct timeval *tv, struct timezone *tz)
{
    FILETIME ft;
    unsigned __int64 tmpres = 0;
    static int tzflag;
    
    if (NULL != tv)
    {
        GetSystemTimeAsFileTime(&ft);
        
        tmpres |= ft.dwHighDateTime;
        tmpres <<= 32;
        tmpres |= ft.dwLowDateTime;
        
        /*converting file time to unix epoch*/
        tmpres -= DELTA_EPOCH_IN_MICROSECS;
        tmpres /= 10;  /*convert into microseconds*/
        tv->tv_sec = (long)(tmpres / 1000000UL);
        tv->tv_usec = (long)(tmpres % 1000000UL);
    }
    
    if (NULL != tz)
    {
        if (!tzflag)
        {
            _tzset();
            tzflag++;
        }
        tz->tz_minuteswest = _timezone / 60;
        tz->tz_dsttime = _daylight;
    }
    
    return 0;
}
#endif

#if LOG_ENABLE_COLOR
// class PrettyUnitTestResultPrinter
typedef enum SimpleTestColor {
    COLOR_DEFAULT,
    COLOR_RED,
    COLOR_GREEN,
    COLOR_YELLOW,
    COLOR_BLUE,
    COLOR_MAGENTA,
}SimpleTestColor;

// port from GTest : JerryZhou@outlook.com
// This header defines the following utilities:
//
// Macros indicating the current platform (defined to 1 if compiled on
// the given platform; otherwise undefined):
//   SIMPLETEST_OS_AIX      - IBM AIX
//   SIMPLETEST_OS_CYGWIN   - Cygwin
//   SIMPLETEST_OS_HPUX     - HP-UX
//   SIMPLETEST_OS_LINUX    - Linux
//     SIMPLETEST_OS_LINUX_ANDROID - Google Android
//   SIMPLETEST_OS_MAC      - Mac OS X
//     SIMPLETEST_OS_IOS    - iOS
//       SIMPLETEST_OS_IOS_SIMULATOR - iOS simulator
//   SIMPLETEST_OS_NACL     - Google Native Client (NaCl)
//   SIMPLETEST_OS_OPENBSD  - OpenBSD
//   SIMPLETEST_OS_QNX      - QNX
//   SIMPLETEST_OS_SOLARIS  - Sun Solaris
//   SIMPLETEST_OS_SYMBIAN  - Symbian
//   SIMPLETEST_OS_WINDOWS  - Windows (Desktop, MinGW, or Mobile)
//     SIMPLETEST_OS_WINDOWS_DESKTOP  - Windows Desktop
//     SIMPLETEST_OS_WINDOWS_MINGW    - MinGW
//     SIMPLETEST_OS_WINDOWS_MOBILE   - Windows Mobile
//   SIMPLETEST_OS_ZOS      - z/OS

// Determines the platform on which Google Test is compiled.
#ifdef __CYGWIN__
# define SIMPLETEST_OS_CYGWIN 1
#elif defined __SYMBIAN32__
# define SIMPLETEST_OS_SYMBIAN 1
#elif defined _WIN32
# define SIMPLETEST_OS_WINDOWS 1
# ifdef _WIN32_WCE
#  define SIMPLETEST_OS_WINDOWS_MOBILE 1
# elif defined(__MINGW__) || defined(__MINGW32__)
#  define SIMPLETEST_OS_WINDOWS_MINGW 1
# else
#  define SIMPLETEST_OS_WINDOWS_DESKTOP 1
# endif  // _WIN32_WCE
#elif defined __APPLE__
# define SIMPLETEST_OS_MAC 1
# if TARGET_OS_IPHONE
#  define SIMPLETEST_OS_IOS 1
#  if TARGET_IPHONE_SIMULATOR
#   define SIMPLETEST_OS_IOS_SIMULATOR 1
#  endif
# endif
#elif defined __linux__
# define SIMPLETEST_OS_LINUX 1
# if defined __ANDROID__
#  define SIMPLETEST_OS_LINUX_ANDROID 1
# endif
#elif defined __MVS__
# define SIMPLETEST_OS_ZOS 1
#elif defined(__sun) && defined(__SVR4)
# define SIMPLETEST_OS_SOLARIS 1
#elif defined(_AIX)
# define SIMPLETEST_OS_AIX 1
#elif defined(__hpux)
# define SIMPLETEST_OS_HPUX 1
#elif defined __native_client__
# define SIMPLETEST_OS_NACL 1
#elif defined __OpenBSD__
# define SIMPLETEST_OS_OPENBSD 1
#elif defined __QNX__
# define SIMPLETEST_OS_QNX 1
#endif  // __CYGWIN__


#if SIMPLETEST_OS_WINDOWS
#include <windows.h>
#include <io.h>
#else
#include <unistd.h>
#endif

const char* posixGetEnv(const char* name) {
#if SIMPLETEST_OS_WINDOWS_MOBILE
    // We are on Windows CE, which has no environment variables.
    return NULL;
#elif defined(__BORLANDC__) || defined(__SunOS_5_8) || defined(__SunOS_5_9)
    // Environment variables which we programmatically clear will be set to the
    // empty string rather than unset (NULL).  Handle that case.
    const char* const env = getenv(name);
    return (env != NULL && env[0] != '\0') ? env : NULL;
#else
    // safe call on windows: _dupenv_s();
    return getenv(name);
#endif
}

#if SIMPLETEST_OS_WINDOWS
# ifdef __BORLANDC__
static inline int posixIsATTY(int fd) { return isatty(fd); }
static inline int posixStrCaseCmp(const char* s1, const char* s2) {
    return stricmp(s1, s2);
}
# else  // !__BORLANDC__
#  if SIMPLETEST_OS_WINDOWS_MOBILE
static inline int posixIsATTY(int /* fd */) { return 0; }
#  else
static inline int posixIsATTY(int fd) { return _isatty(fd); }
#  endif  // SIMPLETEST_OS_WINDOWS_MOBILE
static inline int posixStrCaseCmp(const char* s1, const char* s2) {
    return _stricmp(s1, s2);
}
# endif  // __BORLANDC__
# if SIMPLETEST_OS_WINDOWS_MOBILE
static inline int posixFileNo(FILE* file) { return reinterpret_cast<int>(_fileno(file)); }
// Stat(), RmDir(), and IsDir() are not needed on Windows CE at this
// time and thus not defined there.
# else
static inline int posixFileNo(FILE* file) { return _fileno(file); }
# endif  // SIMPLETEST_OS_WINDOWS_MOBILE
#else
static inline int posixStrCaseCmp(const char* s1, const char* s2) {
    return strcasecmp(s1, s2);
}
static inline int posixFileNo(FILE* file) { return fileno(file); }
static inline int posixIsATTY(int fd) { return isatty(fd); }
#endif  // SIMPLETEST_OS_WINDOWS

// Compares two C strings, ignoring case.  Returns true iff they have
// the same content.
//
// Unlike strcasecmp(), this function can handle NULL argument(s).  A
// NULL C string is considered different to any non-NULL C string,
// including the empty string.
ST_BOOL StringCaseInsensitiveCStringEquals(const char * lhs, const char * rhs) {
    if (lhs == NULL)
        return rhs == NULL;
    if (rhs == NULL)
        return ST_FALSE;
    return posixStrCaseCmp(lhs, rhs) == 0;
}

// Compares two C strings.  Returns true iff they have the same content.
//
// Unlike strcmp(), this function can handle NULL argument(s).  A NULL
// C string is considered different to any non-NULL C string,
// including the empty string.
ST_BOOL StringCStringEquals(const char * lhs, const char * rhs) {
    if ( lhs == NULL ) return rhs == NULL;
    
    if ( rhs == NULL ) return ST_FALSE;
    
    return strcmp(lhs, rhs) == 0;
}

#if SIMPLETEST_OS_WINDOWS && !SIMPLETEST_OS_WINDOWS_MOBILE

// Returns the character attribute for the given color.
WORD GetColorAttribute(SimpleTestColor color) {
    switch (color) {
        case COLOR_RED:    return FOREGROUND_RED;
        case COLOR_GREEN:  return FOREGROUND_GREEN;
        case COLOR_YELLOW: return FOREGROUND_RED | FOREGROUND_GREEN;
        case COLOR_BLUE:   return FOREGROUND_BLUE;
        case COLOR_MAGENTA:return FOREGROUND_RED | FOREGROUND_BLUE;
        default:           return 0;
    }
}

#else

// Returns the ANSI color code for the given color.  COLOR_DEFAULT is
// an invalid input.
const char* GetAnsiColorCode(SimpleTestColor color) {
    switch (color) {
        case COLOR_RED:     return "1";
        case COLOR_GREEN:   return "2";
        case COLOR_YELLOW:  return "3";
        case COLOR_BLUE:    return "4";
        case COLOR_MAGENTA: return "5";
        default:            return NULL;
    };
}

#endif  // SIMPLETEST_OS_WINDOWS && !SIMPLETEST_OS_WINDOWS_MOBILE

// Returns true iff Google Test should use colors in the output.
ST_BOOL ShouldUseColor(ST_BOOL stdout_is_tty) {
    const char* const gtest_color = "auto";
    
    if (StringCaseInsensitiveCStringEquals(gtest_color, "auto")) {
#if SIMPLETEST_OS_WINDOWS
        // On Windows the TERM variable is usually not set, but the
        // console there does support colors.
        return stdout_is_tty;
#else
        // On non-Windows platforms, we rely on the TERM variable.
        const char* const term = posixGetEnv("TERM");
        const ST_BOOL term_supports_color =
        StringCStringEquals(term, "xterm") ||
        StringCStringEquals(term, "xterm-color") ||
        StringCStringEquals(term, "xterm-256color") ||
        StringCStringEquals(term, "screen") ||
        StringCStringEquals(term, "screen-256color") ||
        StringCStringEquals(term, "linux") ||
        StringCStringEquals(term, "cygwin");
        return stdout_is_tty && term_supports_color;
#endif  // SIMPLETEST_OS_WINDOWS
    }
    
    return StringCaseInsensitiveCStringEquals(gtest_color, "yes") ||
    StringCaseInsensitiveCStringEquals(gtest_color, "true") ||
    StringCaseInsensitiveCStringEquals(gtest_color, "t") ||
    StringCStringEquals(gtest_color, "1");
    // We take "yes", "true", "t", and "1" as meaning "yes".  If the
    // value is neither one of these nor "auto", we treat it as "no" to
    // be conservative.
}

// Helpers for printing colored strings to stdout. Note that on Windows, we
// cannot simply emit special characters and have the terminal change colors.
// This routine must actually emit the characters rather than return a string
// that would be colored when printed, as can be done on Linux.
void ColoredPrintf(SimpleTestColor color, const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    
#if SIMPLETEST_OS_WINDOWS_MOBILE || SIMPLETEST_OS_SYMBIAN || SIMPLETEST_OS_ZOS || SIMPLETEST_OS_IOS
    const ST_BOOL use_color = ST_FALSE;
#else
    static ST_BOOL in_color_mode = -1;
    if(in_color_mode == -1){
	in_color_mode =	ShouldUseColor(posixIsATTY(posixFileNo(stdout)) != 0);
    }
    const ST_BOOL use_color = in_color_mode && (color != COLOR_DEFAULT);
#endif  // SIMPLETEST_OS_WINDOWS_MOBILE || SIMPLETEST_OS_SYMBIAN || SIMPLETEST_OS_ZOS
    // The '!= 0' comparison is necessary to satisfy MSVC 7.1.
    
    if (!use_color) {
        vprintf(fmt, args);
        va_end(args);
        return;
    }
    
#if SIMPLETEST_OS_WINDOWS && !SIMPLETEST_OS_WINDOWS_MOBILE
    const HANDLE stdout_handle = GetStdHandle(STD_OUTPUT_HANDLE);
    
    // Gets the current text color.
    CONSOLE_SCREEN_BUFFER_INFO buffer_info;
    GetConsoleScreenBufferInfo(stdout_handle, &buffer_info);
    const WORD old_color_attrs = buffer_info.wAttributes;
    
    // We need to flush the stream buffers into the console before each
    // SetConsoleTextAttribute call lest it affect the text that is already
    // printed but has not yet reached the console.
    fflush(stdout);
    SetConsoleTextAttribute(stdout_handle,
                            GetColorAttribute(color) | FOREGROUND_INTENSITY);
    vprintf(fmt, args);
    
    fflush(stdout);
    // Restores the text color.
    SetConsoleTextAttribute(stdout_handle, old_color_attrs);
#else
    printf("\033[0;3%sm", GetAnsiColorCode(color));
    vprintf(fmt, args);
    printf("\033[m");  // Resets the terminal to default.
#endif  // SIMPLETEST_OS_WINDOWS && !SIMPLETEST_OS_WINDOWS_MOBILE
    va_end(args);
}
#define __COLOR_PRINT(color, ...) ColoredPrintf((SimpleTestColor)(color), __VA_ARGS__)
#else
#define __COLOR_PRINT(color, ...) printf(__VA_ARGS__)
#endif

/// colors
#define __sc_color_red      1 //"\033[0;31m"        /* 0 -> normal ;  31 -> red */
#define __sc_color_cyan     6 //"\033[1;36m"        /* 1 -> bold ;  36 -> cyan */
#define __sc_color_green    2 //"\033[4;32m"        /* 4 -> underline ;  32 -> green */
#define __sc_color_blue     4 //"\033[9;34m"        /* 9 -> strike ;  34 -> blue */
#define __sc_color_black    0 //"\033[0;30m"
#define __sc_color_brown    3 //"\033[0;33m"
#define __sc_color_magenta  5 //"\033[0;35m"
#define __sc_color_gray     0 //"\033[0;37m"
#define __sc_color_none     0 //"\033[0m"        /* to flush the previous property */

static int __sc_colors[] = {
    __sc_color_none, // unknown
    __sc_color_none, // default
    __sc_color_none, // verbose
    __sc_color_brown, // debug
    __sc_color_green, // info
    __sc_color_blue, // warn
    __sc_color_red, // error
    __sc_color_cyan, // fatal
    __sc_color_gray, // silent
    __sc_color_black,
    __sc_color_brown,
};

/*
 * Send a simple string to the log.
 **/
int __sc_log_write(int prio, const char *tag, const char *text){
    struct tm *tm;
    struct timeval tv;
    time_t timep;
    
    gettimeofday(&tv, NULL);
    time(&timep);
    tm=gmtime(&timep);
    if (tag) {
        __COLOR_PRINT(__sc_colors[prio], "%d:%02d:%02d %d \t %s \t %s\n",
                      tm->tm_hour, tm->tm_min, tm->tm_sec, tv.tv_usec, tag, text);
        return 0;
    }else{
        __COLOR_PRINT(__sc_colors[prio], "%d:%02d:%02d %d %s\n",
                      tm->tm_hour, tm->tm_min, tm->tm_sec, tv.tv_usec, text);
        return 0;
    }
}

/*
 * A variant of __android_log_print() that takes a va_list to list
 * additional parameters.
 * */
int __sc_log_vprint(int prio, const char* tag, const char* fmt, va_list ap){
    char content[4096]={0};
    vsnprintf(content, 4095, fmt, ap);
    return __sc_log_write(prio, tag, content);
}

/*
 * Send a formatted string to the log, used like printf(fmt,...)
 **/
int __sc_log_print(int prio, const char* tag, const char* fmt, ...){
    int ret = 0;
    va_list ap;
    va_start(ap, fmt);
    ret = __sc_log_vprint(prio, tag, fmt, ap);
    va_end(ap);
    return ret;
}

#define TEST_LOG_INFO(...) __sc_log_print(SC_LOG_INFO, NULL, __VA_ARGS__)
#define TEST_LOG_ERROR(...) __sc_log_print(SC_LOG_ERROR, NULL, __VA_ARGS__)
#define TEST_LOG_DEBUG(...) __sc_log_print(SC_LOG_DEBUG, NULL, __VA_ARGS__)
#else
#define TEST_LOG_INFO(...) printf(__VA_ARGS__)
#define TEST_LOG_ERROR(...) printf(__VA_ARGS__)
#define TEST_LOG_DEBUG(...) printf(__VA_ARGS__)
#endif

struct SimpleTestStruct;

typedef void (*SimpleTestFunc)(struct SimpleTestStruct* test);

typedef struct SimpleTestStruct
{
  SimpleTestFunc m_func;
  const char* m_name;
  struct SimpleTestStruct* m_next;
  int m_result;
} SimpleTest;

typedef struct SimpleTestSuiteStruct
{
  struct SimpleTestStruct* m_head;
  const char* m_name;
  struct SimpleTestStruct* m_headTest;
  struct SimpleTestStruct* m_tailTest;
  struct SimpleTestSuiteStruct* m_next;
} SimpleTestSuite;

typedef struct SimpleTestRegistryStruct
{
  SimpleTestSuite* m_headSuite;
  SimpleTestSuite* m_tailSuite;
} SimpleTestRegistry;

typedef void (*SimpleTestSuiteFunc)(SimpleTestRegistry* registry);

static void freeTestRegistry(SimpleTestRegistry* registry)
{
    SimpleTestSuite* s = registry->m_headSuite;
    SimpleTestSuite* ss = NULL;
    for ( ; s; s = ss )
    {
        ss = s->m_next;
        SimpleTest* t = s->m_headTest;
        SimpleTest* tt = NULL;
        for ( ; t; t = tt )
        {
            tt = t->m_next;
            free(t);
        }
        free(s);
    }
}

static void runTestRegistry(SimpleTestRegistry* registry)
{
    int okTests = 0, failedTests = 0, curOkTests = 0, curFailedTests = 0;
    SimpleTestSuite* s = registry->m_headSuite;
    SimpleTest* t = NULL;
    
    for ( ; s; s = s->m_next ) {
        t = s->m_headTest;
        t->m_result = 0;
        curOkTests = 0;
        curFailedTests = 0;
        TEST_LOG_INFO("begin suite: %s", s->m_name);
        for ( ; t; t = t->m_next ) {
            (*t->m_func)(t);
            if ( t->m_result == 0 ) {
                TEST_LOG_INFO("%s.%s : ok", s->m_name, t->m_name);
                ++okTests;
                ++curOkTests;
            } else {
                TEST_LOG_ERROR("%s.%s : failed", s->m_name, t->m_name);
                ++failedTests;
                ++curFailedTests;
            }
        }
        if(curFailedTests==0){
            TEST_LOG_INFO("end suite: %s, success %d cases\n", s->m_name, curOkTests);
        } else {
            TEST_LOG_INFO("end suite: %s, success %d cases, failed %d cases\n",
                               s->m_name, curOkTests, curFailedTests);
        }
    }
    if ( failedTests ) {
        TEST_LOG_DEBUG("Result: \n\tOK: %d, FAILED: %d\n", okTests, failedTests);
    }else{
        TEST_LOG_DEBUG("Result: \n\tOK: %d\n", okTests);
    }
}
static SimpleTestRegistry* globalRegistry()
{
    static SimpleTestRegistry registry;
    return &registry;
}

static ST_BOOL addTestSuite(SimpleTestSuite *suite)
{
    suite->m_next = NULL;
    if (globalRegistry()->m_tailSuite) {
        globalRegistry()->m_tailSuite->m_next = suite;
    }
    globalRegistry()->m_tailSuite = suite;
    if (!globalRegistry()->m_headSuite) {
        globalRegistry()->m_headSuite = suite;
    }
    return ST_TRUE;
}

static ST_BOOL addTestSuiteCase(SimpleTestSuite *suite, SimpleTestFunc func, const char* name)
{
    SimpleTest* testDecl = (SimpleTest*)malloc(sizeof(SimpleTest));
    testDecl->m_func = func;
    testDecl->m_name = name;
    testDecl->m_result = 0;
    testDecl->m_next = NULL;
    
    if (suite->m_tailTest) {
        suite->m_tailTest->m_next = testDecl;
    }
    
    suite->m_tailTest = testDecl;
    
    if (!suite->m_headTest) {
        suite->m_headTest = testDecl;
    }
    return ST_TRUE;
}

static void runAllTest()
{
    SimpleTestRegistry* registry = globalRegistry();
    runTestRegistry(registry);
    freeTestRegistry(registry);
}

#ifndef SIMPLETEST_NOTESTING

#define SIMPLETEST_TRUE()     (void)__test
#define SIMPLETEST_ERROR()    ++(__test->m_result); return

#define SIMPLETEST_EQUAL_MSG(expected, actual, msg)                     \
  if ( (expected) != (actual) )                                         \
  {                                                                     \
    const char* _msg = (msg);                                           \
    TEST_LOG_ERROR("%s:%d expected %s, actual: %s %s%s%s",              \
           __FILE__, __LINE__, #expected, #actual,                      \
           _msg ? "(":"", _msg ? _msg : "", _msg ? ")":"");             \
    SIMPLETEST_ERROR();                                                 \
  }

#define SIMPLETEST_EQUAL(expected, actual)                              \
  SIMPLETEST_EQUAL_MSG(expected, actual, NULL)

#define SIMPLETEST_STR_EQUAL_MSG(expected, actual, msg)                 \
  if ( strcmp((expected), (actual)) )                                   \
  {                                                                     \
    const char* _msg = (msg);                                           \
    TEST_LOG_ERROR("%s:%d expected \"%s\", actual: \"%s\"%s%s%s",       \
           __FILE__, __LINE__, expected, actual,                        \
           _msg ? "(":"", _msg ? _msg : "", _msg ? ")":"");             \
    SIMPLETEST_ERROR();                                                 \
  }

#define SIMPLETEST_STR_EQUAL(expected, actual)                          \
  SIMPLETEST_STR_EQUAL_MSG(expected, actual, NULL)

#define SIMPLETEST_ASSERT_MSG(assertion, msg)                           \
  if ( !(assertion) )                                                   \
  {                                                                     \
    const char* _msg = (msg);                                           \
    TEST_LOG_ERROR("%s:%d assertion failed: \"%s\"%s%s%s",              \
           __FILE__, __LINE__, #assertion,                              \
           _msg ? "(":"", _msg ? _msg : "", _msg ? ")":"");             \
    SIMPLETEST_ERROR();                                                 \
  }

#define SIMPLETEST_ASSERT(assertion)                                    \
  SIMPLETEST_ASSERT_MSG(assertion, NULL)

#define SIMPLETEST_DECLARE_SUITE(suiteName)                             \
  void Suite##suiteName(SimpleTestRegistry* registry)

#define SIMPLETEST_START_SUITE(suiteName)                               \
void Suite##suiteName(SimpleTestRegistry* registry)                     \
{                                                                       \
  SimpleTestSuite* suite = (SimpleTestSuite*)malloc(sizeof(SimpleTestSuite)); \
  suite->m_name = #suiteName;                                           \
  suite->m_headTest = NULL;                                             \
  suite->m_tailTest = NULL;                                             \
  suite->m_next = NULL
  
#define SIMPLETEST_ADD_TEST(test)                                       \
  SimpleTest* test##decl = (SimpleTest*)malloc(sizeof(SimpleTest));     \
  test##decl->m_reslt = 0;                                              \
  test##decl->m_func = test;                                            \
  test##decl->m_name = #test;                                           \
  test##decl->m_next = suite->m_headTest;                               \
  suite->m_headTest = test##decl         

#define SIMPLETEST_END_SUITE()                                          \
  suite->m_next = registry->m_headSuite;                                \
  registry->m_headSuite = suite;                                        \
}

#define SIMPLETEST_START_MAIN()                                         \
  int main(int argc, char* argv[])                                      \
  {                                                                     \
    SimpleTestRegistry objRegistry;                                     \
    SimpleTestRegistry* registry = &objRegistry;                        \
    registry->m_headSuite = NULL;                                       \
    registry->m_tailSuite = NULL

#define SIMPLETEST_RUN_SUITE(suiteName)                                 \
  Suite##suiteName(&registry) 

#define SIMPLETEST_INTERNAL_FREE_TESTS_OF(registry)                     \
   freeTestRegistry(registry);

#define SIMPLETEST_INTERNAL_FREE_TESTS()                                \
    SIMPLETEST_INTERNAL_FREE_TESTS_OF(registry)

#define SIMPLETEST_INTERNAL_RUN_TESTS_OF(registry)                      \
    runTestRegistry(registry);

#define SIMPLETEST_INTERNAL_RUN_TESTS()                                 \
    SIMPLETEST_INTERNAL_RUN_TESTS_OF(registey)

#define SIMPLETEST_END_MAIN()                                           \
    SIMPLETEST_INTERNAL_RUN_TESTS();                                    \
    printf("\n");                                                       \
    SIMPLETEST_INTERNAL_FREE_TESTS()                                    \
  }

#define SIMPLETEST_MAIN_SINGLE_SUITE(suiteName)                         \
  SIMPLETEST_START_MAIN();                                              \
  SIMPLETEST_RUN_SUITE(suiteName);                                      \
  SIMPLETEST_END_MAIN();

#define SIMPLETEST_SUIT(suiteName)                                      \
SimpleTestSuite* TinySuit_##suiteName()                                 \
{                                                                       \
    static SimpleTestSuite* suite = NULL;                               \
    if( suite == NULL ) {                                               \
    suite = (SimpleTestSuite*)malloc(sizeof(SimpleTestSuite));          \
    suite->m_name = #suiteName;                                         \
    suite->m_headTest = NULL;                                           \
    suite->m_tailTest = NULL;                                           \
    suite->m_next = NULL;                                               \
    }                                                                   \
    return suite;                                                       \
}                                                                       \
static const ST_BOOL suiteName##_register = addTestSuite(TinySuit_##suiteName())

#define SIMPLETEST_CASE_DECLARE(suiteName, testCase)                    \
void TinySuitCase_##suiteName##_##testCase(SimpleTest* __test)

#define SIMPLETEST_CASE_ADD(suiteName, testCase)                        \
static const ST_BOOL suiteName##_##testCase##_register = addTestSuiteCase(TinySuit_##suiteName(), TinySuitCase_##suiteName##_##testCase, #testCase)

#define SIMPLETEST_CASE(suiteName, testCase)                            \
SIMPLETEST_CASE_DECLARE(suiteName, testCase);                           \
SIMPLETEST_CASE_ADD(suiteName, testCase);                               \
SIMPLETEST_CASE_DECLARE(suiteName, testCase)

#else // SIMPLETEST_NOTESTING

#define SIMPLETEST_TRUE()

#define SIMPLETEST_EQUAL_MSG(expected, actual, msg) (void)0
#define SIMPLETEST_EQUAL(expected, actual) (void)0 
#define SIMPLETEST_STR_EQUAL_MSG(expected, actual, msg) (void)0
#define SIMPLETEST_STR_EQUAL(expected, actual) (void)0
#define SIMPLETEST_ASSERT_MSG(assertion, msg) (void)0
#define SIMPLETEST_ASSERT(assertion) (void)0

#define SIMPLETEST_START_SUITE(suiteName)
#define SIMPLETEST_ADD_TEST(test)
#define SIMPLETEST_END_SUITE()
#define SIMPLETEST_START_MAIN()
#define SIMPLETEST_RUN_SUITE(suiteName)
#define SIMPLETEST_INTERNAL_FREE_TESTS()
#define SIMPLETEST_INTERNAL_RUN_TESTS_OF(registry)
#define SIMPLETEST_INTERNAL_RUN_TESTS()
#define SIMPLETEST_END_MAIN()
#define SIMPLETEST_MAIN_SINGLE_SUITE(suiteName)

#define SIMPLETEST_SUIT(suiteName)
#define SIMPLETEST_CASE_DECLARE(suiteName, testCase)
#define SIMPLETEST_CASE_ADD(suiteName, testCase)
#define SIMPLETEST_CASE(suiteName, testCase)
#endif // SIMPLETEST_NOTESTING

#define SP_SUIT SIMPLETEST_SUIT 
#define SP_CASE SIMPLETEST_CASE

#define SP_EQUAL_MSG SIMPLETEST_EQUAL_MSG
#define SP_EQUAL SIMPLETEST_EQUAL  
#define SP_STR_EQUAL_MSG SIMPLETEST_STR_EQUAL_MSG 
#define SP_STR_EQUAL SIMPLETEST_STR_EQUAL 
#define SP_ASSERT_MSG SIMPLETEST_ASSERT_MSG 
#define SP_ASSERT SIMPLETEST_ASSERT 
#define SP_TRUE SIMPLETEST_ASSERT
#define SP_FALSE(assertion) SIMPLETEST_ASSERT(!(assertion))

#endif

