#ifndef MSERIALIZE_DETAIL_PREPROCESSOR_HPP
#define MSERIALIZE_DETAIL_PREPROCESSOR_HPP

/** MSVC pastes __VA_ARGS__ as a single token, unless expanded once again: */
#define MSERIALIZE_EXPAND(x) x

/** Get the first element from __VA_ARGS__ */
#define MSERIALIZE_FIRST(...) MSERIALIZE_EXPAND(MSERIALIZE_FIRST_I(__VA_ARGS__, 0))
#define MSERIALIZE_FIRST_I(first, ...) first

/** Turn v into "v", expanding v once, if possible */
#define MSERIALIZE_STRINGIZE(v) MSERIALIZE_STRINGIZE_I(v)
#define MSERIALIZE_STRINGIZE_I(v) #v

/** Concatenate a and b, expanding them once, if possible */
#define MSERIALIZE_CAT(a, b) MSERIALIZE_CAT_I(a, b)
#define MSERIALIZE_CAT_I(a, b) a ## b

/** Remove the parens around the argument: (x) -> x */
#define MSERIALIZE_UNTUPLE(a) MSERIALIZE_UNTUPLE_I a
#define MSERIALIZE_UNTUPLE_I(...) __VA_ARGS__

#endif // MSERIALIZE_DETAIL_PREPROCESSOR_HPP
