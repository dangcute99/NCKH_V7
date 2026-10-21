FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"
SRC_URI:append = " file://nckh.cfg"
KERNEL_FEATURES:append = " nckh.cfg"
