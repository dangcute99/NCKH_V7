SUMMARY = "Kiểm thử nhanh board Zynq sau khi boot"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

SRC_URI = "file://zynq-selftest"
S = "${WORKDIR}"

do_install() {
    install -Dm 0755 ${WORKDIR}/zynq-selftest ${D}${bindir}/zynq-selftest
}
