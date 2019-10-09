#!/bin/bash
cmake \
   -DCMAKE_INSTALL_PREFIX=/usr \
   -DFREERDP_INCLUDE_DIR=/usr/include/freerdp2/ \
   -DWINPR_INCLUDE_DIR=/usr/include/winpr2/
