#ifndef BINLOG_DEFAULT_SESSION_HPP
#define BINLOG_DEFAULT_SESSION_HPP

#include <binlog/Session.hpp>
#include <binlog/SessionWriter.hpp>

namespace binlog {

/**
 * Get a global session.
 *
 * The session is shared with the whole program,
 * without any restrictions. This is useful
 * if different components do not want to
 * or cannot aggree on a common global session.
 *
 * This session is also used by basic
 * log macros, which hide the Session
 * concept from users.
 */
inline Session& default_session()
{
  static Session s_session;
  return s_session;
}

/**
 * Get a thread-local writer for default_session().
 *
 * The writer properties are defaulted,
 * see the SessionWriter constructor.
 *
 * This writer is used by basic log macros.
 */
inline SessionWriter& default_thread_local_writer()
{
  static thread_local SessionWriter s_writer(default_session());
  return s_writer;
}

/**
 * Call default_session().consume.
 *
 * This simple shortcut makes sense if the Session
 * concept is hidden in the application, e.g:
 * if only basic log macros are used.
 *
 * @return description of the work done by consume
 */
template <typename OutputStream>
Session::ConsumeResult consume(OutputStream& out)
{
  return default_session().consume(out);
}

} // namespace binlog

#endif // BINLOG_DEFAULT_SESSION_HPP
