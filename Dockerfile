FROM ubuntu:20.04
LABEL maintainer="me@matsuuratomoya.com"

ENV DEBIAN_FRONTEND=noninteractive

RUN mkdir /mimium
WORKDIR /mimium
COPY . /mimium

RUN apt-get update && apt-get install -y git cmake
RUN apt-get install -y build-essential llvm libbison-dev libfl-dev libclalsadrv-dev libz-dev libsndfile-dev libopus-dev 

RUN mkdir build && cd build && cmake .. 
RUN cd /mimium/build && make -j && make install && make clean
RUN apt purge -y build-essential llvm libbison-dev libfl-dev  && apt autoremove -y

ENTRYPOINT [ "/usr/local/bin/mimium" ]