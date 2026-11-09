// Fuzz the hotwords Aho-Corasick ContextGraph: build from fuzzer-chosen token sequences, then walk it.
#include <cstdint>
#include <cstddef>
#include <string>
#include <vector>

#include <fuzzer/FuzzedDataProvider.h>

#include "sherpa-onnx/csrc/context-graph.h"

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
  FuzzedDataProvider fdp(data, size);
  float context_score = fdp.ConsumeFloatingPointInRange<float>(-5.0f, 5.0f);
  float ac_threshold = fdp.ConsumeFloatingPointInRange<float>(0.0f, 1.0f);
  size_t num_phrases = fdp.ConsumeIntegralInRange<size_t>(0, 16);
  int32_t alphabet = fdp.ConsumeBool() ? 8 : 255;

  std::vector<std::vector<int32_t>> token_ids(num_phrases);
  for (auto &ids : token_ids) {
    size_t len = fdp.ConsumeIntegralInRange<size_t>(1, 12);
    for (size_t i = 0; i < len; ++i) {
      ids.push_back(fdp.ConsumeIntegralInRange<int32_t>(0, alphabet));
    }
  }

  std::vector<float> scores, ac_thresholds;
  std::vector<std::string> phrases;
  if (fdp.ConsumeBool()) {
    for (size_t i = 0; i < num_phrases; ++i) {
      scores.push_back(fdp.ConsumeFloatingPointInRange<float>(-5.0f, 5.0f));
    }
  }
  if (fdp.ConsumeBool()) {
    for (size_t i = 0; i < num_phrases; ++i) {
      phrases.push_back(fdp.ConsumeRandomLengthString(8));
    }
  }
  if (fdp.ConsumeBool()) {
    for (size_t i = 0; i < num_phrases; ++i) {
      ac_thresholds.push_back(fdp.ConsumeFloatingPointInRange<float>(0.0f, 1.0f));
    }
  }

  sherpa_onnx::ContextGraph graph(token_ids, context_score, ac_threshold, scores, phrases,
                                  ac_thresholds);

  const sherpa_onnx::ContextState *state = graph.Root();
  while (fdp.remaining_bytes() > 0) {
    int32_t token = fdp.ConsumeIntegralInRange<int32_t>(0, alphabet);
    bool strict = fdp.ConsumeBool();
    auto [score, next, matched] = graph.ForwardOneStep(state, token, strict);
    (void)score;
    (void)matched;
    state = next;
    graph.IsMatched(state);
    if (fdp.ConsumeIntegralInRange<int>(0, 15) == 0) {
      state = graph.Finalize(state).second;
    }
  }
  graph.Finalize(state);
  return 0;
}
