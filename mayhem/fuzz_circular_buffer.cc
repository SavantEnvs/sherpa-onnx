// Fuzz CircularBuffer with a fuzzer-driven sequence of Push/Get/Pop/Resize/Reset.
//
// Arguments follow the documented contract of each call (circular-buffer.h), so the library's own
// error log, one stderr line per rejected call, stays rare. One out-of-contract call is allowed per
// input to keep the validation branches covered.
#include <algorithm>
#include <cstdint>
#include <cstddef>
#include <vector>

#include <fuzzer/FuzzedDataProvider.h>

#include "sherpa-onnx/csrc/circular-buffer.h"

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
  FuzzedDataProvider fdp(data, size);
  int32_t capacity = fdp.ConsumeIntegralInRange<int32_t>(1, 4096);
  int64_t initial = fdp.ConsumeIntegralInRange<int64_t>(0, int64_t(1) << 40);
  sherpa_onnx::CircularBuffer buf(capacity, initial);

  bool probed = false;
  int ops = 0;
  while (fdp.remaining_bytes() > 0 && ops++ < 256) {
    int32_t fill = buf.Size();
    switch (fdp.ConsumeIntegralInRange<int>(0, 7)) {
      case 0: {  // Push that fits
        int32_t n = fdp.ConsumeIntegralInRange<int32_t>(0, capacity - fill);
        std::vector<float> v(n);
        for (auto &x : v) x = fdp.ConsumeFloatingPointInRange<float>(-1.0f, 1.0f);
        buf.Push(v.data(), n);
        break;
      }
      case 1: {  // Push that overflows: the buffer grows itself
        if (capacity >= (1 << 20)) break;
        int32_t n = fdp.ConsumeIntegralInRange<int32_t>(capacity - fill + 1, capacity - fill + 8192);
        std::vector<float> v(n);
        for (auto &x : v) x = fdp.ConsumeFloatingPointInRange<float>(-1.0f, 1.0f);
        buf.Push(v.data(), n);
        capacity = static_cast<int32_t>(std::max<int64_t>(int64_t(capacity) * 2, int64_t(n) + fill));
        break;
      }
      case 2: {  // Get inside [Head, Tail)
        if (fill == 0) break;
        int64_t start = fdp.ConsumeIntegralInRange<int64_t>(buf.Head(), buf.Tail() - 1);
        buf.Get(start, fdp.ConsumeIntegralInRange<int32_t>(0, static_cast<int32_t>(buf.Tail() - start)));
        break;
      }
      case 3:
        buf.Pop(fdp.ConsumeIntegralInRange<int32_t>(0, fill));
        break;
      case 4: {  // Resize to a larger capacity
        int32_t new_capacity = fdp.ConsumeIntegralInRange<int32_t>(capacity + 1, capacity + 8192);
        buf.Resize(new_capacity);
        capacity = new_capacity;
        break;
      }
      case 5:
        buf.Reset();
        break;
      case 6:
        buf.Size();
        buf.Head();
        buf.Tail();
        break;
      case 7: {  // out-of-contract call, at most once per input
        if (probed) break;
        probed = true;
        switch (fdp.ConsumeIntegralInRange<int>(0, 2)) {
          case 0:
            buf.Get(fdp.ConsumeIntegralInRange<int64_t>(buf.Head() - 4, buf.Tail() + 4),
                    fdp.ConsumeIntegralInRange<int32_t>(-4, 8192));
            break;
          case 1:
            buf.Pop(fdp.ConsumeIntegralInRange<int32_t>(-4, 8192));
            break;
          case 2:
            buf.Resize(fdp.ConsumeIntegralInRange<int32_t>(1, capacity));
            break;
        }
        break;
      }
    }
  }
  return 0;
}
