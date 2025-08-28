ARG VERSION=latest

# Stage: build healthcheck tools
# -------------------------------------
FROM gcc:12.2.0 AS builder

RUN apt-get update && \
    apt-get install -y cmake && \
    rm -rf /var/lib/apt/lists/*

COPY ./code /code

WORKDIR /build
RUN cmake ../code && make TCP UDP TCP6 UDP6

RUN mkdir -p /healthcheck \
 && mv TCP* UDP* /healthcheck \
 && chmod +x /healthcheck/*

# Stage: prepare image
# -------------------------------------
FROM scratch

ARG VERSION

LABEL version=${VERSION}
LABEL description="Docker healthcheck tools"
LABEL maintainer="Dmitry Korobkov [https://github.com/DK6v/docker-healthcheck]"

COPY --from=builder /healthcheck /healthcheck
