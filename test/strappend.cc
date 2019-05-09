#include "../include/strcat.hpp"
#include <clocale>
#include <cstdio>

int wmain(int argc, wchar_t const *argv[]) {
  /* code */
  _wsetlocale(LC_ALL, L"");
  std::wstring s = L"nihao";
  base::StrAppend(&s, L" current command args: ", argc, L" first: ", argv[0]);
  if (argc >= 2) {
    base::StrAppend(&s, L" second: ", argv[1]);
  }
  wprintf(L"%s\n", s.data());
  return 0;
}