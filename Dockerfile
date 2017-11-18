FROM ubuntu:zesty

ENV DEBIAN_FRONTEND noninteractive
ENV TRAVIS_BUILD_DIR /root
ENV CXX "g++-7"

WORKDIR /root

RUN \
        apt update && \
        apt install -y build-essential

RUN apt install -y software-properties-common

RUN \
        add-apt-repository ppa:ubuntu-toolchain-r/test

RUN apt update

RUN apt install -y cmake g++-7 git lcov libcurl4-openssl-dev libgloox-dev libgtest-dev libgoogle-glog-dev libevent-dev libboost-system-dev libboost-locale-dev wget sudo unzip libsqlite3-dev

ENTRYPOINT ["/root/build-in-docker.sh"]
