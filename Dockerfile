FROM ubuntu:20.04
#FROM debian:testing
#we got to use our base docker image!!!
# ENV NR_INSTALL_SILENT 1
# ENV NR_INSTALL_USE_CP_NOT_LN 1

ARG DEBIAN_FRONTEND=noninteractive
RUN apt-get update \
    && apt-get install -y --fix-missing --no-install-recommends \
        apt-utils build-essential bash cmake git \
        libssl-dev gpg ca-certificates \
        libtiff-dev \
        libxerces-c-dev liblcms2-dev libpng-dev libz-dev \
        mediainfo \
    && apt -y autoremove \
    &&  mkdir -p video_task && cd video_task && pwd \
    && git clone https://github.com/cinecert/asdcplib.git \
    && git clone https://github.com/kat-spb/tjm.git \
    && git clone https://github.com/uclouvain/openjpeg.git \
    && echo 'Prepare and install asdcplib' \
    && cd asdcplib && pwd \
    && echo 'patching asdcplib' \
    && patch -p1 < ../tjm/asdcp/0001-TCFrameRate.patch \
    && patch -p1 < ../tjm/asdcp/0002-FloatTCFrameRate.patch \
    && rm -rf build; mkdir -p build; cd build && pwd \
    && cmake .. && make -j4 && make install && cd ../.. \
    && echo "Prepare and install openjpeg" && cd openjpeg && pwd \
    && rm -rf build; mkdir -p build; cd build && pwd \
    && cmake .. && make -j4 && make install && cd ../.. \
    && echo "Make links to the libraries" \
    && ln -s /usr/local/lib/libkumu.so.2 /lib/x86_64-linux-gnu/libkumu.so.2 \
    && ln -s /usr/local/lib/libasdcp.so.2 /lib/x86_64-linux-gnu/libasdcp.so.2 \
    && ln -s /usr/local/lib/libas02.so.2 /lib/x86_64-linux-gnu/libas02.so.2 \
    && ln -s /usr/local/lib/libopenjp2.so.7 /lib/x86_64-linux-gnu/libopenjp2.so.7 \
    && echo "Prepare and install TJM" && cd tjm  && pwd \
    && rm -rf build; mkdir -p build; cd build && pwd \
    && cmake .. && make -j4 && make install && cd .. 

WORKDIR /video_task/tjm
RUN  echo "Test the install....." && mkdir -p data
COPY DTC-HDR-TIF-f-ML7_0000000.tif data/
COPY DTC-HDR-TIF-f-ML7_0000178.tif data/
COPY DTC-HDR-TIF-f-ML7_0000296.tif data/
RUN  cd data/ && ../scripts/test_process.sh < input_40.txt && mediainfo output.mxf \
     && echo "Test is complete......." && pwd
