# Known findings: fuzz_wave_reader

## Signed integer overflow in the byte-rate check (`sherpa-onnx/csrc/wave-reader.cc`, `ReadWaveImpl`)

`ReadWave`/`ReadWaveMultiChannel` compute `header.sample_rate * header.num_channels * header.bits_per_sample / 8`
in `int32_t` arithmetic before comparing it to the header's `byte_rate`. A WAVE header with a large
`sample_rate` overflows the multiplication:

    wave-reader.cc:170:49: runtime error: signed integer overflow: 2097152000 * 16 cannot be represented in type 'int'

Reproduce: `/mayhem/fuzz_wave_reader-standalone mayhem/fuzz_wave_reader/known-findings/byte-rate-int-overflow.riff`
(or pass it to `/mayhem/fuzz_wave_reader`). The file is 16-bit mono WAVE with `sample_rate` 2097152000 and a matching `byte_rate`; it is named `.riff` because upstream's `.gitignore` ignores `*.wav`. Reported behaviour is UBSan (`-fno-sanitize-recover=all`) aborting.
Not fixed here: the fork only adds the Mayhem layer and does not edit upstream sources.

## Negative `data` chunk size throws `std::length_error` (`ReadWaveImpl`)

The `data` chunk size is read into an `int32_t` and used unchecked as a vector size, for example
`std::vector<int16_t> samples(header.subchunk2_size / 2)`. A negative size (`0xFFFFFFFC` here) converts to a
huge `size_t`, so the constructor throws and nothing catches it:

    terminate called after throwing an instance of 'std::length_error'
      what():  cannot create std::vector larger than max_size()

Reproduce: `/mayhem/fuzz_wave_reader-standalone mayhem/fuzz_wave_reader/known-findings/negative-data-size-length-error.riff`.
Found by the Mayhem run on the seeded WAV corpus. Not fixed here (upstream sources are not edited).

## Interleave loops read and write out of bounds when the sample count is not a multiple of `num_channels` (`ReadWaveImpl`)

For PCM8, PCM16 and PCM32 data, each `ans[c]` is sized `samples.size() / num_channels` (rounded down), but the
interleave loop runs `for (i = 0, k = 0; i < samples.size(); i += num_channels, ++k)` and reads
`samples[i + c]` for every channel `c`. When `samples.size()` is not a multiple of `num_channels`, the last
iteration reads past `samples` and writes `ans[c][k]` one element past the end of `ans[c]`:

    ERROR: AddressSanitizer: heap-buffer-overflow ... WRITE of size 4
        #0 ReadWaveImpl sherpa-onnx/csrc/wave-reader.cc:249

Reproduce: `/mayhem/fuzz_wave_reader-standalone mayhem/fuzz_wave_reader/known-findings/interleave-oob.riff`.
The file is 16-bit PCM, 3 channels, with a `data` chunk of 14 bytes (7 samples, 7 % 3 = 1). The same loop shape
exists in the PCM8 branch (`wave-reader.cc:278`) and the other sample formats, so Mayhem lists it at several
sites. Depending on where the allocator places the buffers, the same bug shows up as the heap overflow at
`:249` or `:278` or as a NULL dereference in `std::vector<float>::operator[]` under `ReadWaveImpl`.
Not fixed here (upstream sources are not edited).

## `data` chunk size that is not a multiple of the sample size: NULL or overrunning `istream::read` (`ReadWaveImpl`)

Every sample branch sizes its buffer as `subchunk2_size / sizeof(sample)` and then reads `subchunk2_size`
bytes into it. For float32 that is `std::vector<float> samples(header.subchunk2_size / 4)` followed by
`is.read(reinterpret_cast<char *>(samples.data()), header.subchunk2_size)`. When `subchunk2_size` is 1 to 3 the
vector is empty, so `samples.data()` is NULL and `read` copies into NULL. When the size is 4 or more but not a
multiple of 4, `read` overruns the buffer by up to 3 bytes. The PCM16 and PCM32 branches use the same
pattern with `/ 2` and `/ 4`, so an odd size (PCM16) or a size not divisible by 4 (PCM32) overruns the same way;
the PCM8 branch divides by 1, so its read is exact.

    ERROR: AddressSanitizer: SEGV on unknown address 0x000000000000 ... WRITE memory access
        #1 std::basic_streambuf<char>::xsgetn
        #2 std::istream::read
        #3 ReadWaveImpl sherpa-onnx/csrc/wave-reader.cc:313
        #4 sherpa_onnx::ReadWave(std::istream&, int*, bool*) sherpa-onnx/csrc/wave-reader.cc:365

Reproduce: `/mayhem/fuzz_wave_reader-standalone mayhem/fuzz_wave_reader/known-findings/read-size-not-multiple-null.riff`.
The file is float32 (`audio_format` 3, 32 bits), mono, 16000 Hz, with a `data` chunk of 2 bytes. Mayhem reports it
as a NULL Pointer Dereference at `wave-reader.cc:313:8`, in `memcpy` via `std::istream::read`.
Not fixed here (upstream sources are not edited).

## Defect count

The 5-minute run of `fuzz_wave_reader` reports 7 defects. They come from 4 root causes, not 7 independent bugs:

| Root cause | Defects | Reproducer |
| --- | --- | --- |
| Signed overflow in the byte-rate check (`wave-reader.cc:170`, columns 27 and 49) | 2 | `byte-rate-int-overflow.riff` |
| Negative `data` chunk size throws `std::length_error` | 1 | `negative-data-size-length-error.riff` |
| Interleave loops overrun (`wave-reader.cc:249`, `:278`, and a `vector<float>::operator[]` NULL dereference) | 3 | `interleave-oob.riff` |
| `data` size not a multiple of the sample size: NULL `istream::read` (`wave-reader.cc:313:8`) | 1 | `read-size-not-multiple-null.riff` |

## Throughput

This target runs far slower than the others (about 81 tests/s in the first live run). The `data` and skipped
chunk sizes are used unchecked, so a header that claims a large `data` chunk makes `ReadWaveImpl` allocate and
zero-fill a vector of that size before the read fails, and the harness runs the parser twice per input
(`ReadWave` and `ReadWaveMultiChannel`, which share `ReadWaveImpl` but exercise different callers). The
harness is left as is, so both entry points stay covered.
