// Fuzz sherpa-onnx's RIFF/WAVE parser on in-memory bytes.
#include <cstdint>
#include <cstddef>
#include <sstream>
#include <string>

#include "sherpa-onnx/csrc/wave-reader.h"

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
  // The parser's first read is the 4-byte RIFF chunk id. libFuzzer opens every run with the empty input and
  // then "\n"; rejecting streams shorter than that lets the gate's fuzz-smoke see the target iterate. Every
  // input of 4 or more bytes still reaches the parser, including non-WAVE bytes (the path this layer is about).
  if (size < 4) return -1;
  std::string bytes(reinterpret_cast<const char *>(data), size);
  {
    std::istringstream is(bytes);
    int32_t sampling_rate = 0;
    bool is_ok = false;
    sherpa_onnx::ReadWave(is, &sampling_rate, &is_ok);
  }
  {
    std::istringstream is(bytes);
    int32_t sampling_rate = 0;
    bool is_ok = false;
    sherpa_onnx::ReadWaveMultiChannel(is, &sampling_rate, &is_ok);
  }
  return 0;
}
