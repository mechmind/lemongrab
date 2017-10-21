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

RUN apt install -y g++-7 git lcov libcurl4-openssl-dev libgloox-dev libgtest-dev libgoogle-glog-dev libevent-dev libboost-locale-dev wget sudo unzip libsqlite3-dev

COPY travis /root/travis

RUN chmod +x /root/travis/*.sh

RUN /root/travis/install_all.sh

ENTRYPOINT ["/bin/bash"]
