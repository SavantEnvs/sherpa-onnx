// Known-answer probe over the library code the harnesses fuzz. Prints one line per check; test.sh
// diffs the output against literal expected values.
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <sstream>
#include <string>
#include <vector>

#include "sherpa-onnx/csrc/text-utils.h"

static std::string Hex(const std::string &s) {
  std::string r;
  char b[3];
  for (unsigned char c : s) {
    snprintf(b, sizeof(b), "%02x", c);
    r += b;
  }
  return r;
}

int main() {
  printf("trim [%s]\n", sherpa_onnx::Trim("  ab c \t").c_str());
  {
    std::vector<std::string> parts;
    sherpa_onnx::SplitStringToVector("a,b,,c", ",", true, &parts);
    printf("split %zu %s\n", parts.size(), sherpa_onnx::Join(parts, "|").c_str());
    std::vector<int32_t> ints;
    bool ok = sherpa_onnx::SplitStringToIntegers("1:22:-3", ":", false, &ints);
    printf("ints ok=%d %d %d %d\n", ok, ints[0], ints[1], ints[2]);
    int32_t v = 0;
    printf("conv %d %d\n", sherpa_onnx::ConvertStringToInteger("123", &v) ? v : -1,
           sherpa_onnx::ConvertStringToInteger("12x", &v) ? 1 : 0);
  }
  printf("lower %s\n", sherpa_onnx::ToLowerCase("HeLLo \xc3\x89").c_str());
  printf("utf32 %zu %d\n", sherpa_onnx::Utf8ToUtf32("a\xe4\xbd\xa0").size(),
         sherpa_onnx::ContainsCJK("a\xe4\xbd\xa0") ? 1 : 0);
  {
    auto cps = sherpa_onnx::SplitUtf8("\xe4\xbb\x8a\xe5\xa4\xa9ab");
    printf("utf8 %zu %s\n", cps.size(), sherpa_onnx::Join(cps, "|").c_str());
    printf("hex %s\n", Hex(cps[0]).c_str());
  }

  return 0;
}
