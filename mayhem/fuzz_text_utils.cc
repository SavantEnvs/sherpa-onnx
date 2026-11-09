// Fuzz sherpa-onnx's text utilities (UTF-8/NFC handling, splitters, number parsers, lexicon parsing).
#include <cstdint>
#include <cstddef>
#include <sstream>
#include <string>
#include <vector>

#include <fuzzer/FuzzedDataProvider.h>

#include "sherpa-onnx/csrc/text-utils.h"

using namespace sherpa_onnx;  // NOLINT

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
  FuzzedDataProvider fdp(data, size);
  uint8_t op = fdp.ConsumeIntegral<uint8_t>();
  size_t chunk = fdp.ConsumeIntegralInRange<size_t>(1, 64);
  std::string delim = fdp.ConsumeBytesAsString(fdp.ConsumeIntegralInRange<size_t>(1, 3));
  std::string s = fdp.ConsumeRemainingBytesAsString();

  switch (op % 12) {
    case 0: {
      QuoteJsonString(s);
      Trim(s);
      ToLowerCase(s);
      ToLowerAscii(s);
      ToUpperAscii(s);
      ToIntOrDefault(s, 0);
      ToFloatOrDefault(s, 0.0f);
      break;
    }
    case 1: {
      int32_t a;
      int64_t b;
      uint8_t c;
      ConvertStringToInteger(s, &a);
      ConvertStringToInteger(s, &b);
      ConvertStringToInteger(s, &c);
      float f;
      double d;
      ConvertStringToReal(s, &f);
      ConvertStringToReal(s, &d);
      break;
    }
    case 2: {
      std::vector<std::string> out;
      SplitStringToVector(s, delim.c_str(), (op & 0x80) != 0, &out);
      SplitStringAndTrim(s, delim[0]);
      std::vector<int32_t> ints;
      SplitStringToIntegers(s, delim.c_str(), (op & 0x80) != 0, &ints);
      std::vector<float> floats;
      SplitStringToFloats(s, delim.c_str(), (op & 0x80) != 0, &floats);
      break;
    }
    case 3: {
      auto cps = SplitUtf8(s);
      Join(cps, delim);
      Utf8ToUtf32(s);
      Utf8ToUtf16(s);
      RemoveInvalidUtf8Sequences(s);
      IsUtf8(s);
      IsGB2312(s);
      break;
    }
    case 4: {
      std::u32string u = Utf8ToUtf32(s);
      Utf32ToUtf8(u);
      std::u32string v = u;
      NormalizeNfc(&v);
      NormalizeNfc(s);
      NormalizeLexiconKey(s);
      CountLexiconPhraseWords(s);
      SplitLexiconPieces(u);
      for (char32_t c : u) {
        ToSimpleLower(c);
        IsLexiconWordChar(c);
        IsCJK(c);
        IsHangul(c);
        Utf32ToUtf8(c);
      }
      break;
    }
    case 5: {
      ToWideString(s);
      EndsWith(s, delim);
      Contains(s, delim);
      SplitString(s, static_cast<int32_t>(chunk));
      IsAlphaOrPunct(s.empty() ? 0 : static_cast<unsigned char>(s[0]));
      IsPunct(s);
      break;
    }
    case 6: {
      ContainsCJK(s);
      ContainsCJK(Utf8ToUtf32(s));
      RemoveSpaceBetweenCjk(s);
      RemoveLeadingSpaces(s);
      RemoveLeadingPunct(s);
      break;
    }
    case 7: {
      SplitByBlankLines(s);
      SplitByPunctuation(s);
      SplitByAllPunctuation(s);
      break;
    }
    case 8: {
      ChunkText(s, chunk);
      SplitLongSentence(s, chunk);
      CountWords(s);
      SplitLongSentenceByWords(s, chunk);
      break;
    }
    case 9: {
      auto sentences = SplitByPunctuation(s);
      MergeShortSentences(sentences, chunk);
      MergeShortSentencesByWords(sentences, chunk);
      break;
    }
    case 10: {
      std::istringstream is(s);
      int32_t max_len = 0;
      ParseLexiconFile(is, &max_len);
      break;
    }
    case 11: {
      std::vector<std::string> words = SplitStringAndTrim(s, delim[0]);
      int32_t start = static_cast<int32_t>(chunk % 7);
      GetWord(words, start, start + static_cast<int32_t>(chunk % 5));
      break;
    }
  }
  return 0;
}
