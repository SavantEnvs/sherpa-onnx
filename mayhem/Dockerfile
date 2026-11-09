# syntax=docker/dockerfile:1.7
#
# Commit image for sherpa-onnx: builds ON TOP of the org C/C++ base image.
# Build (from the repo root):
#   DOCKER_BUILDKIT=1 docker build -f mayhem/Dockerfile --build-arg COMMIT=$(git rev-parse HEAD) -t mytag .
FROM ghcr.io/savantenvs/base:20261023

ARG COMMIT
ARG REPO_URL
ARG SOURCE_DATE_EPOCH
ARG SANITIZER_FLAGS
ARG COVERAGE_FLAGS=
ARG MAYHEM_JOBS=

# googletest (upstream's CMake would FetchContent it), cached under /opt/toolchains/deps (SPEC §6.2 item 19),
# built here as a static, uninstrumented library.
USER root
ARG GTEST_VER=1.13.0
ARG GTEST_SHA256=ad7fdba11ea011c1d925b3289cf4af2c66a352e18d4c7264392fead75e919363
RUN set -eux; \
    d=/opt/toolchains/deps/googletest-$GTEST_VER; mkdir -p "$d/src" "$d/lib"; \
    curl -fsSL --retry 3 -o /tmp/dep.tgz "https://github.com/google/googletest/archive/refs/tags/v${GTEST_VER}.tar.gz"; \
    echo "$GTEST_SHA256  /tmp/dep.tgz" | sha256sum -c -; \
    tar -xzf /tmp/dep.tgz --strip-components=1 -C "$d/src"; rm -f /tmp/dep.tgz; \
    for f in gtest-all gtest_main; do \
      clang++ -std=c++17 -O2 -g -gdwarf-3 -c "$d/src/googletest/src/$f.cc" \
        -I"$d/src/googletest/include" -I"$d/src/googletest" -o "/tmp/$f.o"; \
    done; \
    ar rcs "$d/lib/libgtest.a" /tmp/gtest-all.o; ar rcs "$d/lib/libgtest_main.a" /tmp/gtest_main.o; \
    rm -f /tmp/*.o; \
    chmod -R a+rX,a-w /opt/toolchains/deps
USER mayhem
WORKDIR /mayhem
COPY --chown=2000:2000 . /mayhem

RUN if [ -x /opt/mayhem/ccache/prefill ]; then /opt/mayhem/ccache/prefill bash mayhem/build.sh; \
    else bash mayhem/build.sh; fi

RUN if [ -f mayhem/test.sh ]; then bash mayhem/test.sh; else echo "no mayhem/test.sh — repo ships no functional tests"; fi

LABEL org.opencontainers.image.source="${REPO_URL}" \
      org.opencontainers.image.revision="${COMMIT}" \
      rlenv.tier="commit" \
      rlenv.source_date_epoch="${SOURCE_DATE_EPOCH}" \
      rlenv.user="mayhem" \
      rlenv.fix="35b71076ce04e1c938b90a8e1008f2df6a5d43ec"

USER mayhem
WORKDIR /mayhem
