// cLog.cpp - simple logging
//{{{  includes
#ifdef _WIN32
  #define _CRT_SECURE_NO_WARNINGS
  #define _SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING
  #define NOMINMAX
  #include "windows.h"
#endif

#ifdef __linux__
  #include <stdio.h>
  #include <stdlib.h>
  #include <stdint.h>
  #include <string.h>

  #include <stddef.h>
  #include <unistd.h>
  #include <cstdarg>
  #include <unistd.h>
  #include <sys/types.h>
  #include <sys/stat.h>
  #include <sys/syscall.h>
  #define gettid() syscall(SYS_gettid)

  #include <signal.h>
  #include <pthread.h>

  #include <execinfo.h>
  #include <cxxabi.h>
  #include <dlfcn.h>
  #include <bfd.h>
  #include <link.h>
#endif

#include <algorithm>
#include <string>
#include <mutex>
#include <deque>
#include <map>

#include "core.h"
#include "date.h"
#include "cLog.h"

#define remove_utf8   remove
#define rename_utf8   rename
#define fopen64_utf8  fopen
#define stat64_utf8   stat64

using namespace std;
using namespace fmt;
using namespace chrono;
//}}}

namespace { // anonymous
  const char kConsoleMidGrey[] =   "\033[38;5;243m";
  const char kConsoleLightGrey[] = "\033[38;5;249m";
  const char kConsoleWhite[] =     "\033[38;5;255m";
  //{{{
  const char kLevelColours[][13] = {
    "\033[38;5;255m", // 0 notice white
    "\033[38;5;208m", // 1 title  orange
    "\033[38;5;196m", // 2 error  light red
    "\033[38;5;220m", // 3 info   yellow
    "\033[38;5;112m", // 4 info1  green
    "\033[38;5;144m", // 5 info2  greenish yellow
    "\033[38;5;147m", // 6 info3  bluish
    };
  //}}}

  const int kMaxBuffer = 10000;
  enum eLogLevel mLogLevel = LOGERROR;

  map <uint64_t, string> mThreadNameMap;

  mutex mLinesMutex;
  deque <cLog::cLine> mLineDeque;

  FILE* mFile = NULL;
  bool mBuffer = false;
  int mDaylightSecs = 0;

  #ifdef _WIN32
  //{{{  windows console
  HANDLE hStdOut = 0;
  uint64_t getThreadId() { return GetCurrentThreadId(); }
  //}}}
  #endif

  #ifdef __linux__
  //{{{  linux post mortem not really working
  const int kMaxFrames = 16;
  const size_t kMemoryAlloc = 16384;
  char* memoryAlloc = NULL;
  typedef void (*sa_sigaction_handler) (int, siginfo_t*, void*);

  //{{{
  class FileMatch {
  public:
     FileMatch (void* addr) : mAddress(addr), mFile(NULL), mBase(NULL) {}

     void*       mAddress;
     const char* mFile;
     void*       mBase;
    };
  //}}}
  //{{{
  static int findMatchingFile (struct dl_phdr_info* info, size_t size, void* data) {

    FileMatch* match = (FileMatch*)data;

    for (uint32_t i = 0; i < info->dlpi_phnum; i++) {
      const ElfW(Phdr)& phdr = info->dlpi_phdr[i];

      if (phdr.p_type == PT_LOAD) {
        ElfW(Addr) vaddr = phdr.p_vaddr + info->dlpi_addr;
        ElfW(Addr) maddr = ElfW(Addr)(match->mAddress);
        if ((maddr >= vaddr) && ( maddr < vaddr + phdr.p_memsz)) {
          match->mFile = info->dlpi_name;
          match->mBase = (void*)info->dlpi_addr;
          return 1;
          }
        }
      }

    return 0;
    }
  //}}}

  //{{{
  class FileLineDesc {
  public:
    FileLineDesc (asymbol** syms, bfd_vma pc) : mPc(pc), mFound(false), mSyms(syms) {}

    void findAddressInSection (bfd* abfd, asection* section) {

      if (mFound)
        return;

    #ifdef HAS_BINUTILS_234
      if ((bfd_section_flags (section) & SEC_ALLOC) == 0)
        return;
      bfd_vma vma = bfd_section_vma (section);
      if (mPc < vma )
        return;
      bfd_size_type size = bfd_section_size (section);
    #else
      if ((bfd_get_section_flags (abfd, section) & SEC_ALLOC) == 0 )
        return;
      bfd_vma vma = bfd_section_vma (abfd, section);
      if (mPc < vma )
        return;
      bfd_size_type size = bfd_section_size (abfd, section);
    #endif

      if (mPc >= (vma + size))
        return;

      mFound = bfd_find_nearest_line (
        abfd, section, mSyms, (mPc - vma), (const char**)&mFilename, (const char**)&mFunctionname, &mLine);
      }

    bfd_vma      mPc;
    char*        mFilename;
    char*        mFunctionname;
    unsigned int mLine;
    int          mFound;
    asymbol**    mSyms;
    };
  //}}}
  //{{{
  static void findAddressInSection (bfd* abfd, asection* section, void* data) {

    FileLineDesc* desc = (FileLineDesc*)data;
    return desc->findAddressInSection (abfd, section);
    }
  //}}}

  //{{{
  static asymbol** slurpSymbolTable (bfd* abfd, const char* fileName) {

    if (!(bfd_get_file_flags (abfd) & HAS_SYMS)) {
      cLog::log (LOGERROR, "bfd file %s has no symbols", fileName);
      return NULL;
      }

    asymbol** syms;
    unsigned int size;

    long symcount = bfd_read_minisymbols (abfd, false, (void**)&syms, &size);
    if (symcount == 0)
      symcount = bfd_read_minisymbols (abfd, true, (void**)&syms, &size);

    if (symcount < 0) {
      cLog::log (LOGERROR, "bfd file %s found no symbols", fileName);
      return NULL;
      }

    return syms;
    }
  //}}}
  //{{{
  static char** translateAddressesBuf (bfd* abfd, bfd_vma* addr, int numAddr, asymbol** syms) {

    char** ret_buf = NULL;
    int32_t total = 0;

    char b;
    char* buf = &b;
    int32_t len = 0;

    for (uint32_t state = 0; state < 2; state++) {
      if (state == 1) {
        ret_buf = (char**)malloc (total + (sizeof(char*) * numAddr));
        buf = (char*)(ret_buf + numAddr);
        len = total;
        }

      for (int32_t i = 0; i < numAddr; i++) {
        FileLineDesc desc (syms, addr[i]);
        if (state == 1)
          ret_buf[i] = buf;
        bfd_map_over_sections (abfd, findAddressInSection, (void*)&desc);

        if (desc.mFound) {
          char* unmangledName = NULL;
          const char* name = desc.mFunctionname;
          if (name == NULL || *name == '\0')
             name = "??";
          else {
            int status;
            unmangledName = abi::__cxa_demangle (name, 0, 0, &status);
            if (status == 0)
              name = unmangledName;
            }
          if (desc.mFilename != NULL) {
            char* h = strrchr (desc.mFilename, '/');
            if (h != NULL)
              desc.mFilename = h + 1;
            }
          total += snprintf (buf, len, "%s:%u %s", desc.mFilename ? desc.mFilename : "??", desc.mLine, name) + 1;
          free (unmangledName);
          }
        else
          total += snprintf (buf, len, "[0x%llx] \?\? \?\?:0", (long long unsigned int) addr[i] ) + 1;
        }

      if (state == 1)
        buf = buf + total + 1;
      }

    return ret_buf;
    }
  //}}}
  //{{{
  static char** processFile (const char* fileName, bfd_vma* addr, int naddr) {

     bfd* abfd = bfd_openr (fileName, NULL);
     if (!abfd) {
       cLog::log (LOGERROR, "openng bfd file %s", fileName);
       return NULL;
       }

     if (bfd_check_format (abfd, bfd_archive)) {
       cLog::log (LOGERROR, "Cannot get addresses from archive %s", fileName);
       bfd_close (abfd);
       return NULL;
       }

     char** matching;
     if (!bfd_check_format_matches (abfd, bfd_object, &matching)) {
       cLog::log (LOGERROR, "Format does not match for archive %s", fileName);
       bfd_close (abfd);
       return NULL;
       }

     asymbol** syms = slurpSymbolTable (abfd, fileName);
     if (!syms) {
       cLog::log (LOGERROR, "Failed to read symbol table for archive %s", fileName);
       bfd_close (abfd);
       return NULL;
       }

     char** retBuf = translateAddressesBuf (abfd, addr, naddr, syms);

     free (syms);

     bfd_close (abfd);
     return retBuf;
    }
  //}}}
  //{{{
  char** backtraceSymbols (void* const* addrList, int numAddr) {

    char*** locations = (char***)alloca (sizeof(char**)*numAddr);

    // initialize the bfd library
    bfd_init();

    int total = 0;
    uint32_t idx = numAddr;
    for (int32_t i = 0; i < numAddr; i++) {
      // find which executable, or library the symbol is from
      FileMatch fileMatch (addrList[--idx] );
      dl_iterate_phdr (findMatchingFile, &fileMatch);

      // adjust the address in the global space of your binary to an
      // offset in the relevant library
      bfd_vma addr = (bfd_vma)(addrList[idx]);
      addr -= (bfd_vma)(fileMatch.mBase);

      // lookup the symbol
      if (fileMatch.mFile && strlen (fileMatch.mFile))
        locations[idx] = processFile (fileMatch.mFile, &addr, 1);
      else
        locations[idx] = processFile ("/proc/self/exe", &addr, 1);

      total += strlen (locations[idx][0]) + 1;
      }

    // return all the file and line information for each address
    char** final = (char**)malloc (total + (numAddr * sizeof(char*)));
    char* f_strings = (char*)(final + numAddr);

    for (int32_t i = 0; i < numAddr; i++ ) {
      strcpy (f_strings, locations[i][0] );
      free (locations[i]);
      final[i] = f_strings;
      f_strings += strlen (f_strings) + 1;
      }

    return final;
    }
  //}}}

  //{{{
  void handleSignal (int signal, void* info, void* secret) {

    // report signal type
    switch (signal) {
      case SIGSEGV:
        cLog::log (LOGERROR, "SIGSEGV thread:%lx pid:%d", pthread_self(), getppid());
        break;
      case SIGABRT:
        cLog::log (LOGERROR, "SIGABRT thread:%lx pid:%d", pthread_self(), getppid());
        break;
      case SIGFPE:
        cLog::log (LOGERROR, "SIGFPE thread:%lx pid:%d", pthread_self(), getppid());
        break;
      default:
        cLog::log (LOGERROR, "crashed signal:%d thread:%lx pid:%d", signal, pthread_self(), getppid());
        break;
      }

    // get backtrace with preallocated memory
    void** traces = reinterpret_cast<void**>(memoryAlloc);
    int traceSize = backtrace (traces, kMaxFrames + 2);
    char** symbols = backtraceSymbols (traces, traceSize);
    if (traceSize <= 2)
      abort();

    //  overwrite sigaction with caller's address
    ucontext_t* ucontext = reinterpret_cast<ucontext_t*>(secret);
    #if defined(__arm__)
      traces[1] = reinterpret_cast<void*>(ucontext->uc_mcontext.arm_pc);
    #elif defined(__aarch64__)
      traces[1] = reinterpret_cast<void*>(ucontext->uc_mcontext.pc);
    #elif defined(__x86_64__)
      traces[1] = reinterpret_cast<void*>(ucontext->uc_mcontext.gregs[REG_RIP]);
    #else
      traces[1] = reinterpret_cast<void*>(ucontext->uc_mcontext.gregs[REG_EIP]);
    #endif

    for (int trace = ((traces[2] == traces[1]) ? 2 : 1); trace < traceSize; trace++)
      cLog::log (LOGNOTICE, string("- ") + symbols[trace]);

    _Exit (EXIT_SUCCESS);
    }
  //}}}

  uint64_t getThreadId() { return gettid(); }
  //}}}
  #endif
  }

//{{{
cLog::~cLog() {
  close();

  #ifdef __linux__
    //{{{  Disable alternative signal handler stack
    stack_t altstack;

    altstack.ss_sp = NULL;
    altstack.ss_size = 0;
    altstack.ss_flags = SS_DISABLE;
    sigaltstack (&altstack, NULL);

    struct sigaction sa;

    sigaction (SIGSEGV, NULL, &sa);
    sa.sa_handler = SIG_DFL;
    sigaction (SIGSEGV, &sa, NULL);

    sigaction (SIGABRT, NULL, &sa);
    sa.sa_handler = SIG_DFL;
    sigaction (SIGABRT, &sa, NULL);

    sigaction (SIGFPE, NULL, &sa);
    sa.sa_handler = SIG_DFL;
    sigaction (SIGFPE, &sa, NULL);

    delete[] memoryAlloc;
    //}}}
  #endif
  }
//}}}

//{{{
bool cLog::init (enum eLogLevel logLevel, bool buffer, const string& logFilePath, const string& title) {

  #ifdef _WIN32
    hStdOut = GetStdHandle (STD_OUTPUT_HANDLE);
    DWORD consoleMode = ENABLE_VIRTUAL_TERMINAL_PROCESSING | ENABLE_PROCESSED_OUTPUT | ENABLE_WRAP_AT_EOL_OUTPUT;
    SetConsoleMode (hStdOut, consoleMode);

    TIME_ZONE_INFORMATION timeZoneInfo;
    if (GetTimeZoneInformation (&timeZoneInfo) == TIME_ZONE_ID_DAYLIGHT)
      mDaylightSecs = -timeZoneInfo.DaylightBias * 60;
  #endif

  #ifdef __linux__
    if (memoryAlloc == NULL)
      memoryAlloc = new char[kMemoryAlloc];

    struct sigaction sa;
    sa.sa_sigaction = (sa_sigaction_handler)handleSignal;
    sigemptyset (&sa.sa_mask);

    sa.sa_flags = SA_RESTART | SA_SIGINFO;
    if (sigaction (SIGSEGV, &sa, NULL) < 0)
      perror ("cCrash - sigaction(SIGSEGV)");
    if (sigaction (SIGABRT, &sa, NULL) < 0)
      perror ("cCrash - sigaction(SIGABBRT)");
    if (sigaction (SIGFPE, &sa, NULL) < 0)
      perror ("cCrash - sigaction(SIGFPE)");
    // !!! need linux daylight timezone !!!
  #endif

  mBuffer = buffer;

  mLogLevel = logLevel;
  if (mLogLevel > LOGNOTICE) {
    if (!logFilePath.empty() && !mFile) {
      string logFileString = logFilePath + "/log.txt";
      mFile = fopen (logFileString.c_str(), "wb");
      }
    }

  setThreadName ("main");

  // write title to the log
  log (LOGNOTICE, title);

  return mFile != NULL;
  }
//}}}
//{{{
void cLog::close() {

  if (mFile) {
    fclose (mFile);
    mFile = NULL;
    }
  }
//}}}

//{{{
enum eLogLevel cLog::getLogLevel() {
  return mLogLevel;
  }
//}}}
//{{{
string cLog::getThreadNameString (uint64_t threadId) {
  auto it = mThreadNameMap.find (threadId);
  if (it != mThreadNameMap.end())
    return it->second;
  else
    return fmt::format ("{4x}", threadId / 8);
  }
//}}}

//{{{
void cLog::cycleLogLevel() {
// cycle log level for L key presses in gui

  switch (mLogLevel) {
    case LOGTITLE:  setLogLevel (LOGNOTICE); break;
    case LOGNOTICE: setLogLevel(LOGERROR);   break;
    case LOGERROR:  setLogLevel(LOGINFO);    break;
    case LOGINFO:   setLogLevel(LOGINFO1);   break;
    case LOGINFO1:  setLogLevel(LOGINFO2);   break;
    case LOGINFO2:  setLogLevel(LOGINFO3);   break;
    case LOGINFO3:  setLogLevel(LOGERROR);   break;
    }
  }
//}}}
//{{{
void cLog::setLogLevel (enum eLogLevel logLevel) {
// limit to valid, change if different

  logLevel = max (LOGNOTICE, min (LOGINFO3, logLevel));
  if (mLogLevel != logLevel) {
    switch (logLevel) {
      case LOGTITLE:  cLog::log (LOGNOTICE, "setLogLevel to LOGTITLE"); break;
      case LOGNOTICE: cLog::log (LOGNOTICE, "setLogLevel to LOGNOTICE"); break;
      case LOGERROR:  cLog::log (LOGNOTICE, "setLogLevel to LOGERROR"); break;
      case LOGINFO:   cLog::log (LOGNOTICE, "setLogLevel to LOGINFO");  break;
      case LOGINFO1:  cLog::log (LOGNOTICE, "setLogLevel to LOGINFO1"); break;
      case LOGINFO2:  cLog::log (LOGNOTICE, "setLogLevel to LOGINFO2"); break;
      case LOGINFO3:  cLog::log (LOGNOTICE, "setLogLevel to LOGINFO3"); break;
      }
    mLogLevel = logLevel;
    }
  }
//}}}
//{{{
void cLog::setThreadName (const string& name) {

  auto it = mThreadNameMap.find (getThreadId());
  if (it == mThreadNameMap.end())
    mThreadNameMap.insert (map<uint64_t,string>::value_type (getThreadId(), name));

  log (LOGINFO, "start");
  }
//}}}

//{{{
void cLog::log (enum eLogLevel logLevel, const string& logStr) {

  if (!mBuffer && (logLevel > mLogLevel))
    return;

  lock_guard<mutex> lockGuard (mLinesMutex);

  time_point<system_clock> now = system_clock::now() + chrono::seconds (mDaylightSecs);

  if (mBuffer) {
    // buffer for widget display
    mLineDeque.push_front (cLine (logLevel, getThreadId(), now, logStr));
    if (mLineDeque.size() > kMaxBuffer)
      mLineDeque.pop_back();
    }

  else if (logLevel <= mLogLevel) {
    string timeString = date::format ("%T", date::floor<microseconds>(now - date::floor<date::days>(now)));
    fputs (format ("{}{} {}{} {}{}\n", kConsoleWhite, timeString,
                                       kConsoleLightGrey, getThreadNameString (getThreadId()),
                                       kLevelColours[logLevel], logStr).c_str(),
           stdout);

    if (mFile) {
      fputs (format ("{}{} {}{} {}{}\n", timeString, getThreadNameString (getThreadId()), logStr).c_str(), mFile);
      fflush (mFile);
      }
    }
  }
//}}}
//{{{
void cLog::log (enum eLogLevel logLevel, const char* format, ... ) {

  if (!mBuffer && (logLevel > mLogLevel)) // bomb out early without lock
    return;

  // form logStr
  va_list args;
  va_start (args, format);

  // get size of str
  size_t size = vsnprintf (nullptr, 0, format, args) + 1; // Extra space for '\0'

  // allocate buffer
  unique_ptr<char[]> buf (new char[size]);

  // form buffer
  vsnprintf (buf.get(), size, format, args);

  va_end (args);

  log (logLevel, string (buf.get(), buf.get() + size-1));
  }
//}}}

//{{{
void cLog::clearScreen() {

  string formatString (fmt::format ("\033[{};{}H\033[J\n", 1, 1));  // cursorPos, clear to end of screen
  fputs (formatString.c_str(), stdout);
  fflush (stdout);
  }
//}}}
//{{{
void cLog::status (int row, int colourIndex, const string& statusString) {
// send colour, pos row column 1, clear from cursor to end of line

  string formatString (fmt::format ("{}\033[{};{}H{}\033[K", kLevelColours[colourIndex], row+1, 1, statusString));
  fputs (formatString.c_str(), stdout);
  fflush (stdout);
  }
//}}}

//{{{
bool cLog::getLine (cLine& line, unsigned lineNum, unsigned& lastLineIndex) {
// still a bit too dumb, holding onto lastLineIndex between searches helps

  lock_guard<mutex> lockGuard (mLinesMutex);

  unsigned matchingLineNum = 0;
  for (auto i = lastLineIndex; i < mLineDeque.size(); i++)
    if (mLineDeque[i].mLogLevel <= mLogLevel)
      if (lineNum == matchingLineNum++) {
        line = mLineDeque[i];
        return true;
        }

  return false;
  }
//}}}
