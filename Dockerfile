FROM ubuntu:latest
LABEL authors="Hank"

RUN apt-get update && \
    apt-get install -y protobuf-compiler

WORKDIR /usr/src/app

COPY ./cmake-build-debug ./

CMD ["./andromeda"]