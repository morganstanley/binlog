#ifndef BINLOG_CREATE_SOURCE_HPP
#define BINLOG_CREATE_SOURCE_HPP

#include <cstdint>
#include <cstdlib>

/**
 * BINLOG_CREATE_SOURCE(id, severity, category, format, argumenttags)
 *
 * Create a static EventSource in the binary compile time.
 *
 * Defines a variable named `id`, that will have a unique value (even for DSOs) runtime.
 */

#ifdef __clang__

#ifdef __APPLE__
  #define BINLOG_SECTION_ATTR(name) __attribute__((section("__DATA_CONST," name), used))
#else
  #define BINLOG_SECTION_ATTR(name) __attribute__((section(name), used))
#endif

// The static_casts are needed to avoid section type conflicts with clang.
#define BINLOG_CREATE_SOURCE(id, severity, category, format, argumenttags) \
  BINLOG_SECTION_ATTR(".binlog.esrc")                                      \
  static constexpr const char* _binlog_esrc[] = {                          \
    severity,                                                              \
    #category,                                                             \
    static_cast<const char*>(__func__),                                    \
    __FILE__,                                                              \
    MSERIALIZE_STRINGIZE(__LINE__),                                        \
    format,                                                                \
    static_cast<const char*>(argumenttags),                                \
    nullptr                                                                \
  };                                                                       \
  const std::uint64_t id = std::uint64_t(&_binlog_esrc) >> 6               \
  /**/

#else // probably GCC

// Use inline ASM to store data in custom ELF sections
// The simpler solution above cannot be used because of section type conflicts:
// https://stackoverflow.com/questions/35091862/

/**
 * Get the runtime virtual address of a label.
 *
 * Taking the address of a label is simple:
 * > lea label, %%(r|e)ax
 * For example on x86:
 * > leal 1b, %0 ; takes the address of label '1', looking _b_ackwards
 *
 * This construct is also valid on x86_64, however, it produces
 * an R_X86_64_32S relocation, which is incompatible with -fPIC,
 * thus cannot be used in shared objects. To workaround this,
 * we are using RIP relative addressing instead:
 * > leaq 1b(%%rip), %0 ; works well
 *
 * The implementation below supports both AT&T and Intel ASM
 * dialects. (i.e: { att | intel })
 */
#if defined __x86_64__
  #define BINLOG_GET_LABEL_ADDR(label, output) \
    "lea{q} {" label "(%%rip)," output " | " output "," label "(%%rip)}"
#else
  #define BINLOG_GET_LABEL_ADDR(label, output) \
    "lea{l} {" label "," output " | " output "," label "}"
#endif

/**
 * Put 8 pointers into the current section, promote them if needed.
 *
 * On non 64-bit architecture, the upper 32 bits are set to 0.
 */
#if defined __x86_64__
  #define BINLOG_EVENT_SOURCE_FIELDS_ASM \
    ".quad %c1, %c2, %c3, %c4, %c5, %c6, %c7, 0"
#else
  #define BINLOG_EVENT_SOURCE_FIELDS_ASM \
    ".long %a1, 0, %a2, 0, %a3, 0, %a4, 0, %a5, 0, %a6, 0, %a7, 0, 0, 0"
#endif

/** Different architectures require different asm input argument constraints */
#if defined __x86_64__
  #define BINLOG_ASM_CONSTR "i"
#else
  #define BINLOG_ASM_CONSTR ""
#endif

#define BINLOG_CREATE_SOURCE(id, severity, category, format, argumenttags) \
  std::uint64_t id;                                                        \
  __asm__ __volatile__(                                                    \
    ".pushsection \".binlog.esrc\",\"wa?\",@progbits""\n"                  \
    "0:"                                            "\n"                   \
    BINLOG_EVENT_SOURCE_FIELDS_ASM                  "\n"                   \
    ".popsection"                                   "\n"                   \
    BINLOG_GET_LABEL_ADDR("0b", "%0")               "\n"                   \
    : "=r" (id)                                                            \
    : BINLOG_ASM_CONSTR (severity),                                        \
      BINLOG_ASM_CONSTR (#category),                                       \
      BINLOG_ASM_CONSTR (__func__),                                        \
      BINLOG_ASM_CONSTR (__FILE__),                                        \
      BINLOG_ASM_CONSTR (MSERIALIZE_STRINGIZE(__LINE__)),                  \
      BINLOG_ASM_CONSTR (format),                                          \
      BINLOG_ASM_CONSTR (argumenttags)                                     \
  );                                                                       \
  id >>= 6                                                                 \
  /**/

#endif

#endif // BINLOG_CREATE_SOURCE_HPP
