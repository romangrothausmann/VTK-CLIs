################################################################################
# base system
################################################################################
FROM ubuntu:16.04 as system


################################################################################
# builder
################################################################################
FROM system as builder

RUN apt-get update && apt-get install -y --no-install-recommends \
    git \
    ca-certificates `# essential for git over https` \
    cmake \
    build-essential \
    libboost-dev libosmesa6-dev
    
ENV LD_LIBRARY_PATH "${LD_LIBRARY_PATH}:/usr/lib/x86_64-linux-gnu/"

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
	  -DOSMESA_LIBRARY=/usr/lib/x86_64-linux-gnu/libOSMesa.so \
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
COPY --from=builder /opt/VTK-CLIs/ /opt/VTK-CLIs/

RUN apt-get update && apt-get install -y --no-install-recommends \
    libosmesa6

ENV PATH "/opt/VTK-CLIs/bin/:${PATH}"

WORKDIR /data
