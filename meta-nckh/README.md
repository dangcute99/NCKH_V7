# meta-nckh

Layer Yocto dùng chung cho mọi project PetaLinux Zynq-7000 của dự án.

| Recipe | Nội dung |
|--------|----------|
| `recipes-kernel/linux/linux-xlnx_%.bbappend` + `linux-xlnx/nckh.cfg` | kernel config fragment cho modem (UIO, AXI DMA, FPGA manager, spidev, i2c-dev, IIO/XADC, PREEMPT) |
| `recipes-modules/modem-regs` | driver mẫu `modem-regs` cho khối thanh ghi trên PL (`compatible = "nckh,modem-regs"`) |
| `recipes-apps/zynq-selftest` | script `/usr/bin/zynq-selftest` kiểm tra nhanh board sau khi boot |

## Thêm vào một project PetaLinux

1. `petalinux-config` → `Yocto Settings` → `User Layers` → thêm `${PROOT}/../../meta-nckh`.
2. Thêm vào `project-spec/meta-user/conf/user-rootfsconfig` của project:
   ```
   CONFIG_modem-regs
   CONFIG_zynq-selftest
   CONFIG_memtester
   CONFIG_spitools
   ```
3. `petalinux-config -c rootfs` → `user packages` → bật các gói trên.
4. `petalinux-build`.
