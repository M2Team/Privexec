//////////
#ifndef PRIVEXEC_CONSOLE_ADAPTER_HPP
#define PRIVEXEC_CONSOLE_ADAPTER_HPP
#ifndef _WINDOWS_
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN //
#endif
#include <Windows.h>
#endif

#include <functional>

namespace priv {
using ssize_t = SSIZE_T;
namespace details {
enum adaptermode_t : int {
  AdapterFile,
  AdapterConsole,
  AdapterConsoleTTY,
  AdapterTTY
};

class adapter {
public:
  adapter(const adapter &) = delete;
  adapter &operator=(const adapter &) = delete;
  ~adapter() {
    //
  }
  static adapter &instance() {
    static adapter adapter_;
    return adapter_;
  }
  ssize_t adapterwrite(int color, const wchar_t *data, size_t len) {
    switch (at) {
    case AdapterFile:
      return writefile(color, data, len);
    case AdapterConsole:
      return writeoldconsole(color, data, len);
    case AdapterConsoleTTY:
      return writeconsole(color, data, len);
    case AdapterTTY:
      return writetty(color, data, len);
    }
    return -1;
  }
  bool changeout(bool isstderr) {
    out = isstderr ? stderr : stdout;
    return out == stderr;
  }

private:
  adapter();
  ssize_t writefile(int color, const wchar_t *data, size_t len);
  ssize_t writeoldconsole(int color, const wchar_t *data, size_t len);
  ssize_t writeconsole(int color, const wchar_t *data, size_t len);
  ssize_t writetty(int color, const wchar_t *data, size_t len);
  //
  int WriteConsoleInternal(const wchar_t *buffer, size_t len);
  adaptermode_t at{AdapterConsole};
  HANDLE hConsole{nullptr};
  FILE *out{stdout};
};
} // namespace details
} // namespace priv

#endif