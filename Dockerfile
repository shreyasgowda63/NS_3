# Copyright (c) 2023 Eduardo Nuno Almeida.
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License version 2 as
# published by the Free Software Foundation;
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
#
# Author: Eduardo Nuno Almeida <enmsa@outlook.pt> [INESC TEC and FEUP, Portugal]

###################################################
# BASE BUILD
###################################################
FROM ubuntu:latest AS base-image

# Install dependencies
RUN apt update && apt upgrade -y && DEBIAN_FRONTEND=noninteractive apt install -y \
    build-essential gcc cmake ninja-build python3
# To speed up future recompilations?
# ccache \
# Extra dependencies when enabling all modules (disabled to speed up building the images)
# python3-pip \
# gsl-bin libgsl-dev libgsl27 \
# libboost-all-dev \
# libgtk-3-dev \
# libfl-dev \
# libxml2 libxml2-dev \
# libopenmpi-dev openmpi-bin openmpi-common \
# libsqlite3-dev sqlite3 \
# libeigen3-dev \
# qtbase5-dev qtchooser qt5-qmake qtbase5-dev-tools \
# ssh

# Disabled for now, to speed up building the images
# RUN pip3 install cppyy

###################################################
# RUNTIME IMAGE
###################################################
FROM base-image

# Build arguments
ARG build_profile=default

# Copy ns-3-dev, configure and build it
WORKDIR /ns-3/

COPY . .
RUN ./ns3 configure \
    --enable-examples --enable-tests \
    # For initial tests, only enable the core module, to speed up Docker image building
    # TODO: Uncomment this line when the MR is ready
    # --enable-mpi --enable-python-bindings \
    --enable-modules=core \
    --build-profile=${build_profile} \
    && ./ns3 build

# Entrypoint
ENTRYPOINT ["bash"]
