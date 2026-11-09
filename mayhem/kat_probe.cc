// Known-answer probe over the library code the harnesses fuzz. Prints one line per check; test.sh
// diffs the output against literal expected values.
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <sstream>
#include <string>
#include <vector>

#include "sherpa-onnx/csrc/circular-buffer.h"
#include "sherpa-onnx/csrc/context-graph.h"
#include "sherpa-onnx/csrc/speaker-embedding-manager.h"
#include "sherpa-onnx/csrc/text-utils.h"
#include "sherpa-onnx/csrc/wave-reader.h"

static std::string Hex(const std::string &s) {
  std::string r;
  char b[3];
  for (unsigned char c : s) {
    snprintf(b, sizeof(b), "%02x", c);
    r += b;
  }
  return r;
}

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
  printf("nfc %s\n", Hex(sherpa_onnx::NormalizeNfc(std::string("e\xcc\x81"))).c_str());
  printf("lower %s\n", sherpa_onnx::ToLowerCase("HeLLo \xc3\x89").c_str());
  printf("utf32 %zu %d\n", sherpa_onnx::Utf8ToUtf32("a\xe4\xbd\xa0").size(),
         sherpa_onnx::ContainsCJK("a\xe4\xbd\xa0") ? 1 : 0);
  printf("json %s\n", sherpa_onnx::QuoteJsonString("a\"b\n").c_str());

  {
    sherpa_onnx::CircularBuffer buf(8);
    float x[5] = {1, 2, 3, 4, 5};
    buf.Push(x, 5);
    buf.Pop(2);
    auto g = buf.Get(2, 3);
    printf("circ size=%d head=%lld tail=%lld get=%g,%g,%g\n", buf.Size(),
           static_cast<long long>(buf.Head()), static_cast<long long>(buf.Tail()), g[0], g[1],
           g[2]);
  }

  {
    sherpa_onnx::ContextGraph graph({{1, 2, 3}}, 1.0f);
    const sherpa_onnx::ContextState *st = graph.Root();
    for (int t : {1, 2, 3}) {
      auto [score, next, matched] = graph.ForwardOneStep(st, t, true);
      printf("ctx token=%d score=%g matched=%d\n", t, score, matched != nullptr);
      st = next;
    }
    printf("ctx is_matched=%d\n", graph.IsMatched(st).first);
    auto [score, next, matched] = graph.ForwardOneStep(graph.Root(), 9, true);
    printf("ctx miss score=%g matched=%d\n", score, matched != nullptr);
    (void)next;
  }

  {
    sherpa_onnx::SpeakerEmbeddingManager mgr(3);
    float a[3] = {1, 0, 0}, b[3] = {0, 1, 0}, q[3] = {0.9f, 0.1f, 0}, z[3] = {0, 0, 1};
    bool add_a = mgr.Add("alice", a);
    bool add_b = mgr.Add("bob", b);
    bool dup = mgr.Add("alice", a);
    printf("spk add=%d%d dup=%d n=%d search=[%s] none=[%s]\n", add_a, add_b, dup, mgr.NumSpeakers(),
           mgr.Search(q, 0.5f).c_str(), mgr.Search(z, 0.5f).c_str());
    mgr.Remove("alice");
    printf("spk after_remove n=%d search=[%s]\n", mgr.NumSpeakers(), mgr.Search(q, 0.1f).c_str());
  }
  return 0;
}
