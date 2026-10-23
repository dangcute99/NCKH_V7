SUMMARY = "Driver mẫu cho khối thanh ghi AXI-Lite của modem trên PL"
SECTION = "PETALINUX/modules"
LICENSE = "GPL-2.0"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/GPL-2.0;md5=801f80980d171dd6425610833a22dbe6"

inherit module

INHIBIT_PACKAGE_STRIP = "1"

SRC_URI = "file://Makefile file://modem-regs.c"
S = "${WORKDIR}"
