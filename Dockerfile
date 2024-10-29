FROM ubuntu:22.04
LABEL authors="Hank"

RUN apt-get update && \
    apt-get install -y protobuf-compiler libprotobuf-dev

WORKDIR /usr/src/app

COPY ./cmake-build-debug ./

# 創建群組，GID 1000
RUN groupadd -g 1000 andromeda

# 創建用戶，UID 1000，並加入上面創建的群組
RUN useradd -u 1000 -g andromeda -m han

# 切換到新用戶
USER han

EXPOSE 9000

CMD ["./andromeda"]

#CMD ["sh", "-c", "while :; do echo 'Container is running'; sleep 3600; done"]

