
// DbgHelpers.h
// Defines and functions to help in debugging
// Shishir Bhat (http://www.shishirbhat.com)
// History
//      06/23/13 Initial version
//

#ifndef _DBGHELPERS_H
#define _DBGHELPERS_H

#ifdef __cplusplus
extern "C" {  
#endif

/**
 * Original work from http://c.learncodethehardway.org/book/ex20.html
 * Modified/additions to my liking
 */

#define logtrace(M, ...) fprintf(stderr, "[TRACE]  " M "\n", ##__VA_ARGS__)
#define logerr(M, ...)  fprintf(stderr, "[ERROR] " M "(lastError: %u)\n", ##__VA_ARGS__, GetLastError())
#define logwarn(M, ...) fprintf(stderr, "[WARN]  " M "\n", ##__VA_ARGS__)
#define loginfo(M, ...) fprintf(stderr, "[INFO]  " M "\n", ##__VA_ARGS__)

#ifdef _DEBUG

    #define logdbg(M, ...)  fprintf(stdout, "[DEBUG] " M "\n", ##__VA_ARGS__)

    #define DBG_MEMSET(mem, size) ( memset(mem, 0xCE, size) )

#else
    #define logdbg(M, ...)
    
    #define NDBG_MEMSET(mem, size) ( memset(mem, 0x00, size) )
    #define DBG_MEMSET(mem, size)   NDBG_MEMSET(mem, size)
#endif

#ifdef __cplusplus
}
#endif

#endif // _DBGHELPERS_H
