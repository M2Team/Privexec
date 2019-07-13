///
#include <bela/pe.hpp>
#include <bela/stdwriter.hpp>

int wmain(int argc, wchar_t **argv) {
  if (argc < 2) {
    bela::FPrintF(stderr, L"usage: %s pefile\n", argv[0]);
    return 1;
  }
  bela::error_code ec;
  auto pm = bela::PESimpleDetailsAze(argv[1], ec);
  if (!pm) {
    bela::FPrintF(stderr, L"Unable parse pe file: %s\n", ec.message);
    return 1;
  }
  bela::FPrintF(stderr, L"File %s depends: \n", argv[1]);
  for (const auto &d : pm->depends) {
    bela::FPrintF(stderr, L"    %s\n", d);
  }
  return 0;
}