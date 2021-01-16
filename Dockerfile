FROM debian:testing
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
RUN    scripts/clone.sh data/DTC-HDR-TIF-f-ML7_0000000.tif data/input 0 10 \
    && scripts/clone.sh data/DTC-HDR-TIF-f-ML7_0000178.tif data/input 11 20 \
    && scripts/clone.sh data/DTC-HDR-TIF-f-ML7_0000296.tif data/input 21 30 \
    && scripts/clone.sh data/DTC-HDR-TIF-f-ML7_0000000.tif data/input 31 40 \
    && mkdir -p data/input_j2k \
    && j2c -i data/input -o data/input_j2k \
    && mkdir -p data/output_j2k \
    && mxf -i data/input_j2k -o data/output_j2k/output.mxf \
    && mediainfo data/output_j2k/output.mxf \
    && echo "Test is complete......."  \
    && pwd
