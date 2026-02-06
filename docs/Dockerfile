#FROM ghcr.io/mattkretz/cplusplus-ci/gcc16
FROM ghcr.io/starsurgeon/clang_p2996_pure

ARG USERNAME=vscode
ARG USER_UID=1000
ARG USER_GID=1000

SHELL ["/bin/bash", "-euxo", "pipefail", "-c"]

RUN apt-get update \
 && apt-get install -y --no-install-recommends sudo \
 && rm -rf /var/lib/apt/lists/*

# Create group/user if they don't exist (robust against collisions)
RUN if getent group "${USER_GID}" >/dev/null; then \
      GROUP_NAME="$(getent group "${USER_GID}" | cut -d: -f1)"; \
    else \
      GROUP_NAME="${USERNAME}"; \
      groupadd --gid "${USER_GID}" "${GROUP_NAME}"; \
    fi \
 && if id -u "${USERNAME}" >/dev/null 2>&1; then \
      EFFECTIVE_USER="${USERNAME}"; \
    elif getent passwd "${USER_UID}" >/dev/null; then \
      EFFECTIVE_USER="$(getent passwd "${USER_UID}" | cut -d: -f1)"; \
    else \
      useradd -m -s /bin/bash --uid "${USER_UID}" --gid "${GROUP_NAME}" "${USERNAME}"; \
      EFFECTIVE_USER="${USERNAME}"; \
    fi \
 && echo "${EFFECTIVE_USER} ALL=(ALL) NOPASSWD:ALL" > "/etc/sudoers.d/${EFFECTIVE_USER}" \
 && chmod 0440 "/etc/sudoers.d/${EFFECTIVE_USER}"
