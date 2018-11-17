//////////
#ifndef PRIVEXEC_CONSOLE_ADAPTER_HPP
#define PRIVEXEC_CONSOLE_ADAPTER_HPP

namespace priv {
namespace details {
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

private:
  adapter();
  bool initialize();
};
} // namespace details
} // namespace priv

#endif