FROM ubuntu:20.04
LABEL maintainer="me@matsuuratomoya.com"

ENV DEBIAN_FRONTEND=noninteractive

RUN mkdir /mimium
WORKDIR /mimium
COPY . /mimium

RUN apt-get update && apt-get install --no-install-recommends -y git cmake build-essential llvm libbison-dev libfl-dev libclalsadrv-dev libz-dev libsndfile-dev libopus-dev libgtest-dev \
    && apt-get clean \
    && rm -rf /var/lib/apt/lists/*

RUN cmake -Bbuild
RUN cmake --build build -j
RUN cmake --build build --target install
RUN apt purge -y build-essential llvm libbison-dev libfl-dev  && apt autoremove -y

ENTRYPOINT [ "/usr/local/bin/mimium" ]