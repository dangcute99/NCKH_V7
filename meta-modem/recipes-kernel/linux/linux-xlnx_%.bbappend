FILESEXTRAPATHS_prepend := "${THISDIR}/${PN}:"
SRC_URI_append = " file://modem.cfg"
KERNEL_FEATURES_append = " modem.cfg"
