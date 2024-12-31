FROM ubuntu
RUN apt update -y && apt upgrade -y
RUN apt install gcc valgrind make time -y
COPY . /tp
WORKDIR /tp
CMD make local
