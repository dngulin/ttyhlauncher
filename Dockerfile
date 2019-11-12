FROM alpine:latest as builder
WORKDIR /ttyhlauncher
COPY . /ttyhlauncher/
RUN set -ex \
 && apk add --no-cache git cmake make gcc g++ qt5-qtbase-dev qt5-qttools-dev libzip-dev \
 && cmake . \
 && make

FROM alpine:latest
RUN apk add --no-cache qt5-qtbase-x11 libzip openjdk9-jre ttf-freefont
COPY --from=builder /ttyhlauncher/ttyhlauncher /usr/local/bin/ttyhlauncher
CMD ttyhlauncher
