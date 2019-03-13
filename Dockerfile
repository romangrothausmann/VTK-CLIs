################################################################################
# base system
################################################################################
FROM ubuntu:18.04 as system


################################################################################
# builder
################################################################################
FROM system as builder

RUN apt-get update && apt-get install -y --no-install-recommends \
    curl \
    git \
    ca-certificates `# essential for git over https` \
    build-essential \
    automake pkgconf libtool bison flex python python-mako zlib1g-dev libexpat1-dev llvm-dev gettext \
    libboost-dev

### OSMesa
RUN git clone -b mesa-18.3.4 --depth 1 https://gitlab.freedesktop.org/mesa/mesa

RUN cd mesa && \
    NOCONFIGURE=1 ./autogen.sh && \
    ./configure \
    	  --prefix=/opt/mesa \
  	  --enable-opengl --disable-gles1 --disable-gles2   \
  	  --disable-va --disable-xvmc --disable-vdpau       \
  	  --enable-shared-glapi                             \
  	  --disable-texture-float                           \
  	  --enable-gallium-llvm --enable-llvm-shared-libs   \
  	  --with-gallium-drivers=swrast,swr                 \
  	  --disable-dri --with-dri-drivers=                 \
  	  --disable-egl --with-egl-platforms= --disable-gbm \
  	  --disable-glx                                     \
  	  --disable-osmesa --enable-gallium-osmesa && \
    make -j"$(nproc)" && \
    make -j"$(nproc)" install

ENV LD_LIBRARY_PATH "${LD_LIBRARY_PATH}:/opt/mesa/lib/"

### cmake independent of distro version
RUN curl -s https://cmake.org/files/v3.11/cmake-3.11.4-Linux-x86_64.sh -o cmake.sh
RUN sh cmake.sh --prefix=/usr --exclude-subdir --skip-license

### VTK
RUN git clone -b v8.1.2 --depth 1 https://gitlab.kitware.com/vtk/vtk.git

RUN mkdir -p VTK_build && \
    cd VTK_build && \
    cmake \
    	  -DCMAKE_INSTALL_PREFIX=/opt/vtk/ \
	  -DCMAKE_BUILD_TYPE=Release \
	  -DBUILD_SHARED_LIBS=ON \
	  -DBUILD_TESTING=OFF \
	  -DVTK_Group_Qt=OFF \
	  -DVTK_Group_StandAlone=ON \
	  -DVTK_RENDERING_BACKEND=OpenGL2 \
	  -DVTK_OPENGL_HAS_OSMESA=ON \
	  -DOSMESA_INCLUDE_DIR=/opt/mesa/include \
	  -DOSMESA_LIBRARY=/opt/mesa/lib/libOSMesa.so \
	  -DVTK_USE_X=OFF \
	  -DModule_vtkInfovisBoostGraphAlgorithms=ON \
	  -DModule_vtkIOExport=ON \
	  ../vtk && \
    make -j"$(nproc)" && \
    make -j"$(nproc)" install


### VTK-CLIs
COPY . /code/

RUN mkdir -p /build/ && \
    cd /build/ && \
    cmake \
    	  -DCMAKE_INSTALL_PREFIX=/opt/VTK-CLIs/ \
	  -DCMAKE_PREFIX_PATH=/opt/vtk/lib/cmake/ \
	  -DCMAKE_BUILD_TYPE=Release \
	  /code/ && \
    make -j"$(nproc)" && \
    make -j"$(nproc)" install


################################################################################
# install
################################################################################
FROM system as install

COPY --from=builder /opt/vtk/ /opt/vtk/
COPY --from=builder /opt/mesa/ /opt/mesa/
COPY --from=builder /opt/VTK-CLIs/ /opt/VTK-CLIs/

RUN apt-get update && apt-get install -y --no-install-recommends \
    libllvm6.0

ENV LD_LIBRARY_PATH "${LD_LIBRARY_PATH}:/opt/vtk/lib/"
ENV LD_LIBRARY_PATH "${LD_LIBRARY_PATH}:/opt/mesa/lib/"

ENV PATH "/opt/VTK-CLIs/bin/:${PATH}"

WORKDIR /data
