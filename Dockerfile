FROM alpine:3.10 as builder
WORKDIR /ttyhlauncher
COPY . /ttyhlauncher/
RUN set -ex \
 && apk add --no-cache git cmake make gcc g++ qt5-qtbase-dev qt5-qttools-dev libzip-dev 
 && cmake . && make

FROM alpine:3.10
RUN apk add --no-cache qt5-qtbase-x11 libzip openjdk9-jre ttf-freefont \
            mesa mesa-gl mesa-dri-intel mesa-dri-vmwgfx mesa-dri-virtio \
            mesa-dri-swrast mesa-dri-freedreno mesa-dri-nouveau mesa-dri-ati
COPY --from=builder /ttyhlauncher/ttyhlauncher /usr/local/bin/ttyhlauncher
CMD ttyhlauncher
