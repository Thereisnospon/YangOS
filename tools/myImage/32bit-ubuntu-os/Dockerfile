FROM 32bit/ubuntu:16.04 
RUN buildDeps='gcc libc6-dev make' \
    && apt-get update \
    && apt-get install -y $buildDeps \
    && apt-get install nasm
    