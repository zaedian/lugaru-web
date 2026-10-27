FROM fedora:latest

RUN dnf -y install \
    sudo \
    curl \
    git \
    make \
    cmake \
    pkg-config \
    procps-ng \
    findutils \
    gawk \
    tar \
    wget \
    unzip \
    && dnf clean all

RUN dnf -y install \
    gcc \
    gcc-c++ \
    mesa-libGL-devel \
    mesa-libGLU-devel \
    libjpeg-turbo-devel \
    libpng-devel \
    SDL2-devel \
    libvorbis-devel \
    libogg-devel \
    openal-soft-devel \
    jsoncpp-devel \
    && dnf clean all

RUN dnf -y install \
    mingw64-gcc \
    mingw64-gcc-c++ \
    mingw64-binutils \
    mingw64-headers \
    mingw64-libjpeg-turbo \
    mingw64-libpng \
    mingw64-SDL2 \
    mingw64-libvorbis \
    mingw64-libogg \
    mingw64-openal-soft \
    mingw64-jsoncpp \
    && dnf clean all

RUN dnf -y install \
    mingw32-gcc \
    mingw32-gcc-c++ \
    mingw32-binutils \
    mingw32-headers \
    mingw32-libjpeg-turbo \
    mingw32-libpng \
    mingw32-SDL2 \
    mingw32-libvorbis \
    mingw32-libogg \
    mingw32-openal-soft \
    mingw32-jsoncpp \
    && dnf clean all

ARG USERNAME=vscode
ARG USER_UID=1000
ARG USER_GID=$USER_UID

RUN groupadd --gid $USER_GID $USERNAME \
    && useradd --uid $USER_UID --gid $USER_GID -m $USERNAME \
    && echo $USERNAME ALL=\(root\) NOPASSWD:ALL > /etc/sudoers.d/$USERNAME \
    && chmod 0440 /etc/sudoers.d/$USERNAME

WORKDIR /workspace
USER $USERNAME
