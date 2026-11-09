// Known-answer probe over the library code the harnesses fuzz. Prints one line per check; test.sh
// diffs the output against literal expected values.
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <sstream>
#include <string>
#include <vector>

#include "sherpa-onnx/csrc/wave-reader.h"

static void Put32(std::string *s, uint32_t v) { s->append(reinterpret_cast<char *>(&v), 4); }
static void Put16(std::string *s, uint16_t v) { s->append(reinterpret_cast<char *>(&v), 2); }

int main() {
  // 16-bit mono 16 kHz PCM WAVE holding {0, 16384, -16384, -32768}.
  std::string wav = "RIFF";
  Put32(&wav, 36 + 8);
  wav += "WAVEfmt ";
  Put32(&wav, 16);
  Put16(&wav, 1);
  Put16(&wav, 1);
  Put32(&wav, 16000);
  Put32(&wav, 32000);
  Put16(&wav, 2);
  Put16(&wav, 16);
  wav += "data";
  Put32(&wav, 8);
  for (int16_t v : {int16_t(0), int16_t(16384), int16_t(-16384), int16_t(-32768)}) {
    Put16(&wav, static_cast<uint16_t>(v));
  }
  {
    std::istringstream is(wav);
    int32_t sr = 0;
    bool ok = false;
    auto s = sherpa_onnx::ReadWave(is, &sr, &ok);
    printf("wave ok=%d sr=%d n=%zu", ok, sr, s.size());
    for (float x : s) printf(" %.4f", x);
    printf("\n");
    std::istringstream bad("RIFX not a wave file");
    ok = true;
    auto e = sherpa_onnx::ReadWave(bad, &sr, &ok);
    printf("badwave ok=%d n=%zu\n", ok, e.size());
  }
}
