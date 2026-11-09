// Fuzz FastClustering (cosine hierarchical clustering used by speaker diarization).
#include <cstdint>
#include <cstddef>
#include <vector>

#include <fuzzer/FuzzedDataProvider.h>

#include "sherpa-onnx/csrc/fast-clustering.h"

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
  FuzzedDataProvider fdp(data, size);
  int32_t rows = fdp.ConsumeIntegralInRange<int32_t>(0, 48);
  int32_t cols = fdp.ConsumeIntegralInRange<int32_t>(1, 16);
  int32_t num_clusters = fdp.ConsumeIntegralInRange<int32_t>(-1, 10);
  float threshold = fdp.ConsumeFloatingPointInRange<float>(0.0f, 2.0f);
  bool confidence = fdp.ConsumeBool();

  std::vector<float> feats(static_cast<size_t>(rows) * cols);
  for (auto &x : feats) x = fdp.ConsumeFloatingPointInRange<float>(-5.0f, 5.0f);

  sherpa_onnx::FastClusteringConfig config(num_clusters, threshold, confidence);
  sherpa_onnx::FastClustering clustering(config);
  std::vector<float> silhouettes;
  clustering.Cluster(feats.data(), rows, cols, confidence ? &silhouettes : nullptr);
  return 0;
}
