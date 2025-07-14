# author: B. Waidyawansa, A. Panta

# copy the almalinux 9 base image from docker repo
FROM almalinux:9 AS build
ADD http://pki.jlab.org/JLabCA.crt /etc/pki/ca-trust/source/anchors/
RUN update-ca-trust

RUN dnf update -y && \
    dnf -y install epel-release dnf-plugins-core 'dnf-command(config-manager)' && \
    dnf config-manager --set-enabled crb && \
    rm -rf /var/cache/yum/*

# update the base image and install the dependencies required to install and compile the analyzer including CERN ROOT
RUN dnf update -y  && \
    dnf install -y --setopt=install_weak_deps=False wget cmake gcc gcc-c++ git binutils make \
    openssl-devel libX11-devel libXt-devel libXpm-devel \
    boost-devel mariadb-server mariadb-connector-c-devel \
    python3 python3-pip tbb-devel libuv-devel giflib-devel root root-python3 root-mathcore root-montecarlo-eg \
    root-mathmore root-gui root-hist root-physics root-genvector && \
    dnf clean all && rm -rf /var/cache/yum/*

ENV JAPAN_MOLLER_HOME=/opt/japan-moller
ENV QW_PRMINPUT=$JAPAN_MOLLER_HOME/Parity/prminput
ENV PATH=$JAPAN_MOLLER_HOME/bin:$PATH
ENV LD_LIBRARY_PATH=$JAPAN_MOLLER_HOME/lib:$JAPAN_MOLLER_HOME/lib64:$LD_LIBRARY_PATH
ENV QWANALYSIS=/japan-MOLLER

# copy the analyzer source code from git into the workdir
COPY . ${QWANALYSIS}

RUN mkdir -p ${QWANALYSIS}/build && \
    cd ${QWANALYSIS}/build && \
    cmake .. -DCMAKE_INSTALL_PREFIX=$JAPAN_MOLLER_HOME && \
    make -j$(nproc) && \
    make install


# Optional: keep build artifacts for debugging
# Comment this out when we are ready for maturity
# RUN rm -rf ${QWANALYSIS}/build # Updated to use QWANALYSIS

RUN echo '#!/bin/bash' > /usr/local/bin/entrypoint.sh && \
    echo 'unset OSRELEASE' >> /usr/local/bin/entrypoint.sh && \
    echo 'exec "$@"' >> /usr/local/bin/entrypoint.sh && \
    chmod +x /usr/local/bin/entrypoint.sh

# Set the entrypoint
ENTRYPOINT ["/usr/local/bin/entrypoint.sh"]
WORKDIR $QWANALYSIS

CMD ["/bin/bash"]