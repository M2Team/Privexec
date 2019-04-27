//////////
#ifndef PRIVEXEC_CONSOLE_ADAPTER_HPP
#define PRIVEXEC_CONSOLE_ADAPTER_HPP
#include <base.hpp>
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
  ssize_t adapterwrite(int color, std::wstring_view sv) {
    switch (at) {
    case AdapterFile:
      return writefile(color, sv);
    case AdapterConsole:
      return writeoldconsole(color, sv);
    case AdapterConsoleTTY:
      return writeconsole(color, sv);
    case AdapterTTY:
      return writetty(color, sv);
    }
    return -1;
  }
  bool changeout(bool isstderr) {
    out = isstderr ? stderr : stdout;
    return out == stderr;
  }

private:
  adapter();
  ssize_t writefile(int color, std::wstring_view sv);
  ssize_t writeoldconsole(int color, std::wstring_view sv);
  ssize_t writeconsole(int color, std::wstring_view sv);
  ssize_t writetty(int color, std::wstring_view sv);
  //
  int WriteConsoleInternal(std::wstring_view sv);
  adaptermode_t at{AdapterConsole};
  HANDLE hConsole{nullptr};
  FILE *out{stdout};
};
} // namespace details
} // namespace priv

#endif