///
#include <bela/strcat.hpp>
#include <bela/stdwriter.hpp>

int wmain(int argc, wchar_t **argv) {
  const auto ux =
      "\xf0\x9f\x98\x81 UTF-8 text \xE3\x8D\xA4 --> \xF0\xA0\x83\xA3 \x41 "
      "\xE7\xA0\xB4 \xE6\x99\x93"; // force encode UTF-8
  const wchar_t wx[] =
      L"Engine \xD83D\xDEE0 中国 \U0001F496 \x0041 \x7834 \x6653 \xD840\xDCE3";
  constexpr auto iscpp17 = __cplusplus >= 201703L;
  bela::FPrintF(stderr,
                L"Argc: %d Arg0: \x1b[32m%s\x1b[0m W: %s UTF-8: %s "
                L"__cplusplus: %d C++17: %b\n",
                argc, argv[0], wx, ux, __cplusplus, iscpp17);

  char32_t em = 0x1F603;     // 😃 U+1F603
  char32_t sh = 0x1F496;     //  💖
  char32_t blueheart = U'💙'; //💙
  char32_t se = 0x1F92A;     //🤪
  char32_t em2 = U'中';
  auto s = bela::StringCat(L"Look emoji -->", em, L" U+",
                           bela::AlphaNum(bela::Hex(em)));
  bela::FPrintF(stderr, L"emoji %c %c %c %c %U %U %s P: %p\n", em, sh,
                blueheart, se, em, em2, s, &em);
  bela::FPrintF(stderr, L"hStderr Mode:    %s.\nhStdin Mode:     %s.\n",
                bela::FileTypeName(stderr), bela::FileTypeName(stdin));
  auto es = bela::EscapeNonBMP(wx);
  bela::FPrintF(stderr, L"EscapeNonBMP: %s\n", es);
  bela::FPrintF(stderr, L"[%-20s]\n", bela::FileTypeName(stdout));
  bela::FPrintF(stderr, L"[%20s]\n", bela::FileTypeName(stdout));
  bela::FPrintF(stderr, L"[%-10d]\n", argc);
  bela::FPrintF(stderr, L"[%10d]\n", argc);
  bela::FPrintF(stderr, L"[%010d]\n", argc);
  int n = -1999;
  bela::FPrintF(stderr, L"[%-10d]\n", n);
  bela::FPrintF(stderr, L"[%10d]\n", n);
  bela::FPrintF(stderr, L"[%010d]\n", n);
  bela::FPrintF(stderr, L"[%-60d]\n", n);
  bela::FPrintF(stderr, L"[%60d]\n", n);
  bela::FPrintF(stderr, L"[%060d]\n", n);
  int n2 = 2999;
  bela::FPrintF(stderr, L"[%-10d]\n", n2);
  bela::FPrintF(stderr, L"[%10d]\n", n2);
  bela::FPrintF(stderr, L"[%010d]\n", n2);
  double ddd = 000192.15777411;
  bela::FPrintF(stderr, L"[%08.7f]\n", ddd);
  long xl = 18256444;
  bela::FPrintF(stderr, L"[%-16x]\n", xl);
  bela::FPrintF(stderr, L"[%016X]\n", xl);
  bela::FPrintF(stderr, L"[%16X]\n", xl);
  bela::FPrintF(stderr, L"%%pointer: [%p]\n", (void *)argv);
  return 0;
}
