FROM ubuntu:20.04

RUN dpkg --add-architecture i386 && \
    apt update && apt install -y --no-install-recommends \
    libxau6 libxau6:i386 \
    libxdmcp6 libxdmcp6:i386 \
    libxcb1 libxcb1:i386 \
    libxext6 libxext6:i386 \
    libx11-6 libx11-6:i386

# nvidia-container-runtime
ENV NVIDIA_VISIBLE_DEVICES \
    ${NVIDIA_VISIBLE_DEVICES:-all}
ENV NVIDIA_DRIVER_CAPABILITIES \
    ${NVIDIA_DRIVER_CAPABILITIES:+$NVIDIA_DRIVER_CAPABILITIES,}graphics,compat32,utility

RUN echo "/usr/local/nvidia/lib" >> /etc/ld.so.conf.d/nvidia.conf && \
    echo "/usr/local/nvidia/lib64" >> /etc/ld.so.conf.d/nvidia.conf

# Required for non-glvnd setups.
ENV LD_LIBRARY_PATH /usr/lib/x86_64-linux-gnu:/usr/lib/i386-linux-gnu${LD_LIBRARY_PATH:+:${LD_LIBRARY_PATH}}:/usr/local/nvidia/lib:/usr/local/nvidia/lib64

RUN apt install -y --no-install-recommends \
    libglvnd0 libglvnd0:i386 \
    libgl1 libgl1:i386 \
    libglx0 libglx0:i386 \
    libegl1 libegl1:i386\
    libgles2 libgles2:i386

RUN apt install -y\
    mesa-utils \
    x11-apps

RUN rm -rf /var/lib/apt/lists/*

COPY 10_nvidia.json /usr/share/glvnd/egl_vendor.d/10_nvidia.json

ENV DEBIAN_FRONTEND noninteractive 

RUN apt update && \
    apt install -y \
    bash-completion \
    build-essential \
    cmake \
    cmake-curses-gui \
    coinor-libipopt-dev \
    freeglut3-dev \
    git \
    libace-dev \
    libboost-filesystem-dev \
    libboost-system-dev \
    libboost-thread-dev \
    libdc1394-22-dev \
    libedit-dev \
    libeigen3-dev \
    libgsl0-dev \
    libjpeg-dev \
    liblua5.1-dev \
    libode-dev \
    libopencv-dev \
    libsdl1.2-dev \
    libtinyxml-dev \
    libv4l-dev \
    libxml2-dev \
    lua5.1 \
    portaudio19-dev \
    qml-module-qt-labs-folderlistmodel \
    qml-module-qt-labs-settings \
    qml-module-qtmultimedia \
    qml-module-qtquick-controls \
    qml-module-qtquick-dialogs \
    qml-module-qtquick-window2 \
    qml-module-qtquick2 \
    qtbase5-dev \
    qtdeclarative5-dev \
    qtmultimedia5-dev \
    swig \
    libmatio-dev \
    libirrlicht-dev \
    libspdlog-dev \
    libblas-dev \
    liblapack-dev \
    wget \
    lsb-core

RUN echo "deb http://packages.osrfoundation.org/gazebo/ubuntu-stable `lsb_release -cs` main" > /etc/apt/sources.list.d/gazebo-stable.list && \
    wget https://packages.osrfoundation.org/gazebo.key -O - | apt-key add - && \
    apt update &&  \
    apt install -y \
    gazebo11 \
    libgazebo11-dev &&\
    apt autoremove && \
    apt clean && \
    rm -rf /tmp/* /var/lib/apt/lists/* /var/tmp/*

RUN git config --global user.name "iCub" && \
    git config --global user.email "icub@example.com"

RUN cd /usr/local/src && \
    git clone https://github.com/robotology/robotology-superbuild.git && \
    cd robotology-superbuild && \
    mkdir build && \
    cd build/ && \
    cmake .. && \
    make -j `nproc`

RUN cd /usr/local/src && \
    git clone https://github.com/2103simon/icub_haptic_exploration_environment.git && \
    mkdir build && \
    cd build/ && \
    cmake .. -DCMAKE_INSTALL_PREFIX=/usr/local/src/robotology-superbuild/build/install && \
    make -j `nproc` install

ENV YARP_ROBOT_NAME iCubGazeboV2_5_hapticmanip

SHELL ["/bin/bash", "-c"]

RUN ln -s /usr/local/src/robotology-superbuild/build/install/share/bash-completion/completions/yarp /etc/bash_completion.d/ && \
    echo $' \n\
    source /usr/local/src/robotology-superbuild/build/install/share/robotology-superbuild/setup.sh \n\
    if [ -f /etc/bash_completion ] && ! shopt -oq posix; then \n\
    . /etc/bash_completion \n\
    fi ' >> $HOME/.bashrc

RUN apt install -y \
    gedit

COPY icubGazeboApp.xml /usr/local/src/robotology-superbuild/build/install/share/yarp/applications/
