///
#include <cstdio>
#include <string_view>
#define PUGIXML_HEADER_ONLY 1
#include "../../vendor/pugixml/pugixml.hpp"

int main(int argc, char const *argv[]) {
  if (argc < 2) {
    fprintf(stderr, "usage: %s appmanifest\n", argv[0]);
    return 1;
  }
  pugi::xml_document doc;
  if (!doc.load_file(argv[1])) {
    fprintf(stderr, "unable load %s %s\n", argv[1], strerror(errno));
    return 1;
  }
  // constexpr std::string_view caps = "Capability";
  for (auto it : doc.child("Package").child("Capabilities")) {
    /* code */
    fprintf(stderr, "%s %s\n", it.name(), it.attribute("Name").as_string());
  }
  return 0;
}
