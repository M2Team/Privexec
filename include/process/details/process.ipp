/// process base
#ifndef PRIVEXEC_PRCOESS_IPP
#define PRIVEXEC_PRCOESS_IPP
#include "processfwd.hpp"

namespace priv {
// execute only
bool process::execute() {
  //
  return true;
}
bool process::execute(int level) {
  //
  return false;
}

} // namespace priv

#endif