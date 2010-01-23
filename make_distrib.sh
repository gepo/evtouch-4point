#!/bin/bash
DISTRI_FILES=".libs/evtouch_drv.so calibrator ChangeLog TODO README README.calibration INSTALL Makefile.am configure.ac"
SRC_DISTRI_FILES="*.[ch] Imakefile make_distrib.sh"
VERSION=$1
TGT_DIR="evtouch-$1"


rm -f evtouch*.tar.gz
mkdir $TGT_DIR

./autogen.sh --enable-calibrator
make
cp $DISTRI_FILES $TGT_DIR
tar czvf evtouch-$VERSION.tar.gz $TGT_DIR
rm -rf $TGT_DIR

mkdir $TGT_DIR
cp -v $DISTRI_FILES $SRC_DISTRI_FILES $TGT_DIR
tar czvf evtouch-$VERSION-src.tar.gz $TGT_DIR
make clean
rm -rf $TGT_DIR
