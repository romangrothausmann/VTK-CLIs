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
    libboost-dev

### VTK
RUN git clone -b v8.2.0 https://gitlab.kitware.com/vtk/vtk.git

RUN mkdir -p VTK_build && \
    cd VTK_build && \
    cmake \
    	  -DCMAKE_INSTALL_PREFIX=/opt/vtk/ \
	  -DCMAKE_BUILD_TYPE=Release \
	  -DBUILD_SHARED_LIBS=ON \
	  -DBUILD_TESTING=OFF \
	  -DVTK_Group_Qt=OFF \
	  -DVTK_Group_Rendering=OFF \
	  -DVTK_Group_StandAlone=ON \
	  -DVTK_RENDERING_BACKEND=None \
	  -DModule_vtkInfovisBoostGraphAlgorithms=ON \
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

ENV PATH "/opt/VTK-CLIs/bin/:${PATH}"

WORKDIR /data
