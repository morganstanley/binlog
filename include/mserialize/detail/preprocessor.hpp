#ifndef MSERIALIZE_DETAIL_PREPROCESSOR_HPP
#define MSERIALIZE_DETAIL_PREPROCESSOR_HPP

/** Get the first element from __VA_ARGS__ */
#define MSERIALIZE_FIRST(...) MSERIALIZE_FIRST_I(__VA_ARGS__, 0)
#define MSERIALIZE_FIRST_I(first, ...) first

/** Turn v into "v", expanding v once, if possible */
#define MSERIALIZE_STRINGIZE(v) MSERIALIZE_STRINGIZE_I(v)
#define MSERIALIZE_STRINGIZE_I(v) #v

/** Concatenate a and b, expanding them once, if possible */
#define MSERIALIZE_CAT(a, b) MSERIALIZE_CAT_I(a, b)
#define MSERIALIZE_CAT_I(a, b) a ## b

#endif // MSERIALIZE_DETAIL_PREPROCESSOR_HPP
