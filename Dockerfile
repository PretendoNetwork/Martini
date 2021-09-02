FROM devkitpro/devkitppc:20210726

WORKDIR /app
CMD mkdir -p build && cd build && cmake .. && make -j$(nproc)
