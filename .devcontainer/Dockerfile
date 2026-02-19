FROM ghcr.io/starsurgeon/cpp_reflection_blog/cpp_reflection_blog_clang-p2996:latest

SHELL ["/bin/bash", "-euxo", "pipefail", "-c"]

RUN apt-get update \
 && apt-get install -y --no-install-recommends \
      cppcheck \
      gcovr \
 && (apt-get install -y --no-install-recommends include-what-you-use \
      || apt-get install -y --no-install-recommends iwyu) \
 && rm -rf /var/lib/apt/lists/*
