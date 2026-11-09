// Fuzz SpeakerEmbeddingManager (Eigen-backed speaker registry and cosine search).
#include <cstdint>
#include <cstddef>
#include <string>
#include <vector>

#include <fuzzer/FuzzedDataProvider.h>

#include "sherpa-onnx/csrc/speaker-embedding-manager.h"

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
  FuzzedDataProvider fdp(data, size);
  int32_t dim = fdp.ConsumeIntegralInRange<int32_t>(1, 32);
  sherpa_onnx::SpeakerEmbeddingManager mgr(dim);

  auto embedding = [&]() {
    std::vector<float> v(dim);
    for (auto &x : v) x = fdp.ConsumeFloatingPointInRange<float>(-10.0f, 10.0f);
    return v;
  };
  auto name = [&]() { return std::string(1, 'a' + fdp.ConsumeIntegralInRange<int>(0, 7)); };

  int ops = 0;
  while (fdp.remaining_bytes() > 0 && ops++ < 128) {
    switch (fdp.ConsumeIntegralInRange<int>(0, 9)) {
      case 0:
        mgr.Add(name(), embedding().data());
        break;
      case 1: {
        std::vector<std::vector<float>> list;
        size_t n = fdp.ConsumeIntegralInRange<size_t>(0, 4);
        for (size_t i = 0; i < n; ++i) list.push_back(embedding());
        mgr.Add(name(), list);
        break;
      }
      case 2:
        mgr.Remove(name());
        break;
      case 3:
        mgr.Search(embedding().data(), fdp.ConsumeFloatingPointInRange<float>(-1.0f, 1.0f));
        break;
      case 4:
        mgr.GetBestMatches(embedding().data(), fdp.ConsumeFloatingPointInRange<float>(-1.0f, 1.0f),
                           fdp.ConsumeIntegralInRange<int32_t>(0, 6));
        break;
      case 5:
        mgr.Verify(name(), embedding().data(), fdp.ConsumeFloatingPointInRange<float>(-1.0f, 1.0f));
        break;
      case 6:
        mgr.Score(name(), embedding().data());
        break;
      case 7:
        mgr.Contains(name());
        mgr.NumSpeakers();
        mgr.Dim();
        break;
      case 8:
        mgr.GetAllSpeakers();
        break;
      case 9:
        mgr.GetEmbedding(name());
        break;
    }
  }
  return 0;
}
