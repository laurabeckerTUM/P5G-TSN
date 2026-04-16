#!/usr/bin/env bash
set -e  # Exit immediately on any error

cd ~

echo "Installing dependencies..."
sudo apt-get update
sudo apt-get install -y build-essential clang lld gdb bison flex perl curl git \
    python3 python3-pip libpython3-dev qtbase5-dev qtchooser qt5-qmake qtbase5-dev-tools \
    libqt5opengl5-dev libxml2-dev zlib1g-dev doxygen graphviz \
    libwebkit2gtk-4.1-0 xdg-utils libdw-dev pkg-config python3-venv 

echo "Downloading OMNeT++ 6.1.0..."
curl -LO https://github.com/omnetpp/omnetpp/releases/download/omnetpp-6.1.0/omnetpp-6.1.0-linux-x86_64.tgz

echo "Extracting OMNeT++ archive..."
tar xvfz omnetpp-6.1.0-linux-x86_64.tgz

echo "Changing to OMNeT++ directory..."
cd ~/omnetpp-6.1

python3 -m venv .venv
source .venv/bin/activate

python -m ensurepip --upgrade
python -m pip install --upgrade pip wheel
python -m pip install "setuptools<82"
python -m pip install -r python/requirements.txt

python -c "import sys; print(sys.executable)"
python -c "import setuptools; print(setuptools.__version__)"
python -c "import pkg_resources; print(pkg_resources.__file__)"

echo "Sourcing OMNeT++ environment..."
source setenv

echo "Configuring OMNeT++..."
./configure WITH_OSG=no

echo "Building OMNeT++..."
make

echo "Cloning P5G-TSN repository..."
git clone --recurse-submodules https://github.com/laurabeckerTUM/P5G-TSN.git p5g-tsn

cd p5g-tsn

#git checkout Joint_Scheduler_NOMS to be replaced with tag
git submodule update --init --recursive

source setenv

echo "Building Inet4.5"
cd inet4.5
git apply --check ../5gtq.patch && git apply ../5gtq.patch || echo "Patch already applied or not cleanly applicable, skipping."

echo "Checking out Tag"
make makefiles
make
cd ..

echo "Building Simu5G"
cd Simu5G
make
cd ..

echo "Building tsnfivegcomm"
cd tsnfivegcomm
make makefiles
make
cd ..

echo "Installing evaluation script packages"

python -m pip install -r post_processing/requirements.txt
echo "Installation of P5G-TSN Simulator finished."
