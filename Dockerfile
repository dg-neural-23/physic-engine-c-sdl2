FROM archlinux:latest

# Install MinGW cross-compiler and tools
RUN pacman -Syu --noconfirm && \
    pacman -S --noconfirm mingw-w64-gcc wget unzip && \
    pacman -Scc --noconfirm

WORKDIR /build

# Download SDL2 for Windows (MinGW)
RUN wget -q https://github.com/libsdl-org/SDL/releases/download/release-2.28.5/SDL2-devel-2.28.5-mingw.tar.gz && \
    tar -xzf SDL2-devel-2.28.5-mingw.tar.gz && \
    rm SDL2-devel-2.28.5-mingw.tar.gz

# Download SDL2_ttf for Windows (MinGW)
RUN wget -q https://github.com/libsdl-org/SDL_ttf/releases/download/release-2.20.2/SDL2_ttf-devel-2.20.2-mingw.tar.gz && \
    tar -xzf SDL2_ttf-devel-2.20.2-mingw.tar.gz && \
    rm SDL2_ttf-devel-2.20.2-mingw.tar.gz

# Copy source files
COPY engine3d.c .
COPY fonts ./fonts

# Compile for Windows 64-bit
RUN x86_64-w64-mingw32-gcc engine3d.c \
    -I./SDL2-2.28.5/x86_64-w64-mingw32/include/SDL2 \
    -L./SDL2-2.28.5/x86_64-w64-mingw32/lib \
    -I./SDL2_ttf-2.20.2/x86_64-w64-mingw32/include/SDL2 \
    -L./SDL2_ttf-2.20.2/x86_64-w64-mingw32/lib \
    -lmingw32 -lSDL2main -lSDL2 -lSDL2_ttf \
    -static-libgcc -static-libstdc++ \
    -o engine3d.exe \
    -mwindows

# Copy required DLLs
RUN mkdir -p /output && \
    cp engine3d.exe /output/ && \
    cp SDL2-2.28.5/x86_64-w64-mingw32/bin/SDL2.dll /output/ && \
    cp SDL2_ttf-2.20.2/x86_64-w64-mingw32/bin/SDL2_ttf.dll /output/ && \
    cp SDL2_ttf-2.20.2/x86_64-w64-mingw32/bin/*.dll /output/ 2>/dev/null || true && \
    cp -r fonts /output/

CMD ["sh", "-c", "cp -r /output/* /build/output/"]
