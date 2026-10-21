# Task list (PetaLinux 2023.2)

Plan: [plan.md](plan.md). Quy trình: [docs/quy-trinh-xay-dung.md](../docs/quy-trinh-xay-dung.md). Mọi lệnh chạy code dự án (`petalinux-*`, script, qemu, ghi thẻ SD) đều phải hỏi người dùng trước.

---

## Phase 1: Môi trường + QEMU

## Task 1: Cài PetaLinux 2023.2

**Description:** Người dùng tải `petalinux-v2023.2-*-installer.run` và BSP `xilinx-zc702-v2023.2-final.bsp` từ trang AMD. Cài gói host theo UG1144, rồi cài PetaLinux vào `~/petalinux/2023.2` (không dùng root). Thêm `plnx/*/build/`, `plnx/*/images/`, `plnx/*/components/`, `plnx/zc702/`, `*.bsp`, `*.run` vào `.gitignore`.

**Acceptance criteria:**
- [ ] `source ~/petalinux/2023.2/settings.sh` không lỗi
- [ ] `petalinux-create --help` chạy được

**Verification:**
- [ ] Manual check: `which petalinux-build petalinux-create petalinux-boot`

**Dependencies:** None (bộ cài do người dùng tải)

**Files likely touched:** `.gitignore`

**Estimated scope:** S

## Task 2: Project BSP ZC702 → build → QEMU

**Description:** `petalinux-create -t project -s xilinx-zc702-v2023.2-final.bsp -n zc702` trong `plnx/`. Chạy `petalinux-build` (lần đầu 1–2 giờ, chạy nền), rồi `petalinux-boot --qemu --kernel`. Mục đích: xác nhận máy build và QEMU hoạt động trước khi tùy biến.

**Acceptance criteria:**
- [ ] `petalinux-build` thành công; `images/linux/` có `image.ub`, `u-boot.elf`, `zynq_fsbl.elf`
- [ ] QEMU in log kernel trên `ttyPS0` và tới dấu nhắc login; đăng nhập được

**Verification:**
- [ ] Build succeeds: `petalinux-build` (hỏi người dùng trước khi chạy)
- [ ] Manual check: `petalinux-boot --qemu --kernel`, đăng nhập, chạy `uname -a; cat /proc/cpuinfo | grep -c processor`

**Dependencies:** T1

**Files likely touched:** không có file commit (project ZC702 nằm trong `.gitignore`); ghi chú kết quả vào `tasks/logs/`

**Estimated scope:** S

## Checkpoint 1
- [ ] QEMU ZC702 boot tới login
- [ ] Ghi lại thời gian build và dung lượng `plnx/zc702/build`
- [ ] Người dùng duyệt trước khi sang Phase 2

---

## Phase 2: Layer meta-nckh

## Task 3: Layer `meta-nckh` + kernel fragment

**Description:** Tạo layer `meta-nckh/` (`conf/layer.conf` với `LAYERSERIES_COMPAT` khớp Yocto của PetaLinux 2023.2). Thêm `recipes-kernel/linux/linux-xlnx_%.bbappend` + `linux-xlnx/nckh.cfg` (UIO, UIO_PDRV_GENIRQ, XILINX_DMA, FPGA_MGR_ZYNQ_FPGA, OF_OVERLAY, SPI_SPIDEV, I2C_CHARDEV, MTD_SPI_NOR, XILINX_XADC, PREEMPT, HIGH_RES_TIMERS). Thêm layer vào project ZC702 qua `petalinux-config → Yocto Settings → User Layers`.

**Acceptance criteria:**
- [ ] `petalinux-build -c kernel` thành công
- [ ] Mọi option trong `nckh.cfg` có giá trị đúng trong `.config` của kernel
- [ ] Kernel vẫn boot được trên QEMU

**Verification:**
- [ ] Build succeeds: `petalinux-build -c kernel && petalinux-build`
- [ ] Manual check: grep từng option trong `.config`; boot QEMU

**Dependencies:** T2

**Files likely touched:** `meta-nckh/conf/layer.conf`, `meta-nckh/recipes-kernel/linux/linux-xlnx_%.bbappend`, `meta-nckh/recipes-kernel/linux/linux-xlnx/nckh.cfg`

**Estimated scope:** S

## Task 4: Gói rootfs + `zynq-selftest`

**Description:** Tạo recipe `meta-nckh/recipes-apps/zynq-selftest/` (dựng từ mẫu `petalinux-create -t apps --template install`) cài script POSIX sh vào `/usr/bin/zynq-selftest`. Script kiểm tra các mục ở README §6.2, in PASS/WARN/FAIL và trả mã thoát khác 0 nếu có FAIL. Bật trong `petalinux-config -c rootfs`: dropbear, ethtool, iperf3, i2c-tools, spitools, mtd-utils, memtester, `zynq-selftest`; gói nào không có trong menu thì thêm vào `user-rootfsconfig`.

**Acceptance criteria:**
- [ ] Các tool có trên rootfs; SSH được vào QEMU
- [ ] `zynq-selftest` trên QEMU: không FAIL ở CPU/RAM/kernel log/rootfs/UART/eth0; QSPI/PL/modem báo WARN
- [ ] Mã thoát đúng (0 khi không có FAIL)

**Verification:**
- [ ] Manual check: trên QEMU chạy `zynq-selftest; echo $?`
- [ ] Self-check: ép một mục FAIL, xác nhận mã thoát khác 0

**Dependencies:** T3

**Files likely touched:** `meta-nckh/recipes-apps/zynq-selftest/{zynq-selftest.bb,files/zynq-selftest}`, `plnx/zc702/project-spec/configs/rootfs_config`, `plnx/zc702/project-spec/meta-user/conf/user-rootfsconfig`

**Estimated scope:** S

## Task 5: Driver mẫu `modem-regs`

**Description:** Tạo recipe `meta-nckh/recipes-modules/modem-regs/` (dựng từ mẫu `petalinux-create -t modules`). Đây là platform driver khớp `compatible = "nckh,modem-regs"`: ioremap vùng thanh ghi, tạo misc device `/dev/modem_<addr>`, read/write theo word 32-bit căn lề, `llseek` giới hạn trong vùng thanh ghi. Bật trong `petalinux-config -c rootfs`.

**Acceptance criteria:**
- [ ] Module build không có warning; `modinfo modem-regs` có alias `of:*nckh,modem-regs*`
- [ ] `modprobe modem-regs` trên QEMU thành công, không lỗi trong `dmesg`; `rmmod` sạch
- [ ] Đọc/ghi lệch căn lề trả `-EINVAL` (kiểm tra trên board ở T9)

**Verification:**
- [ ] Build succeeds: `petalinux-build -c modem-regs && petalinux-build`
- [ ] Manual check: trên QEMU chạy `modprobe modem-regs && dmesg | tail && rmmod modem-regs`

**Dependencies:** T3

**Files likely touched:** `meta-nckh/recipes-modules/modem-regs/{modem-regs.bb,files/modem-regs.c,files/Makefile,files/COPYING}`

**Estimated scope:** M

## Checkpoint 2
- [ ] QEMU ZC702 với `meta-nckh`: `zynq-selftest` không FAIL ở các mục PS mà QEMU mô phỏng
- [ ] Commit + push `meta-nckh/`
- [ ] Người dùng duyệt trước khi sang phần cứng

---

## Phase 3: Board E316

## Task 6: Thiết kế Vivado chỉ có PS cho E316 *(người dùng làm, Claude hỗ trợ)*

**Description:** Trong Vivado 2023.2, tạo block design ZYNQ7 PS theo schematic E316 (DDR, MIO: UART, SD0, QSPI, ENET0, I2C), chưa có IP trên PL. Generate bitstream, export hardware kèm bitstream vào `hw/e316/system.xsa`.

**Acceptance criteria:**
- [ ] Có `hw/e316/system.xsa` chứa `.bit` và `ps7_init`
- [ ] Ghi cấu hình thật (DDR part, UART console, PHY, jumper boot mode) vào README §2.2

**Verification:**
- [ ] Manual check: `unzip -l hw/e316/system.xsa`

**Dependencies:** None (làm song song Phase 1–2; **hạn chót 23/10**)

**Files likely touched:** `hw/e316/system.xsa`, `README.md`

**Estimated scope:** S

## Task 7: Project PetaLinux E316

**Description:** `petalinux-create -t project --template zynq -n e316` trong `plnx/`, `petalinux-config --get-hw-description=../../hw/e316/system.xsa`. Cấu hình theo docs B3: UART console, rootfs EXT4 `/dev/mmcblk0p2`, bootargs có `generic-uio`, user layer `meta-nckh`. Chép cấu hình rootfs đã chốt ở T4. Viết `system-user.dtsi` (PHY Ethernet, flash QSPI + phân vùng, node IP modem dạng chú thích).

**Acceptance criteria:**
- [ ] `project-spec/configs/config` có `ROOTFS_EXT4`, `SDROOT_DEV=/dev/mmcblk0p2`, user layer `meta-nckh`
- [ ] `petalinux-build -c device-tree` thành công; `system.dtb` có `chosen/bootargs` đúng

**Verification:**
- [ ] Build succeeds: `petalinux-build -c device-tree`
- [ ] Manual check: `dtc -I dtb -O dts images/linux/system.dtb | grep -A3 chosen`

**Dependencies:** T5, T6

**Files likely touched:** `plnx/e316/project-spec/configs/{config,rootfs_config}`, `plnx/e316/project-spec/meta-user/recipes-bsp/device-tree/files/system-user.dtsi`, `plnx/e316/project-spec/meta-user/conf/user-rootfsconfig`

**Estimated scope:** M

## Task 8: Build, đóng gói, boot E316 từ thẻ SD

**Description:** `petalinux-build` → `petalinux-package --boot --fsbl ... --fpga ... --u-boot --force` → `petalinux-package --wic`. Ghi `.wic` ra thẻ SD (người dùng xác nhận đúng `/dev/sdX`), đặt boot mode SD, boot E316. Nếu không lên, thử `petalinux-boot --jtag` để khoanh vùng.

**Acceptance criteria:**
- [ ] `bootgen -arch zynq -read BOOT.BIN` liệt kê FSBL, bitstream, U-Boot, DTB@0x100000
- [ ] E316 in log FSBL → U-Boot → kernel trên UART, tới dấu nhắc login
- [ ] `zynq-selftest` PASS: CPU, RAM, kernel log, rootfs, UART, SD, eth0 có link, PL `operating`

**Verification:**
- [ ] Build succeeds: `petalinux-build` + 2 lệnh `petalinux-package`
- [ ] Manual check: lưu log UART vào `tasks/logs/`; chạy `zynq-selftest`

**Dependencies:** T4, T7

**Files likely touched:** `plnx/e316/project-spec/meta-user/recipes-bsp/device-tree/files/system-user.dtsi`, `tasks/logs/`

**Estimated scope:** S

## Checkpoint 3
- [ ] E316 boot tới login từ thẻ SD; `zynq-selftest` PASS các mục PS
- [ ] README §2.2 cập nhật thông số thật của E316
- [ ] Commit + push `plnx/e316/project-spec/`; người dùng duyệt

---

## Phase 4: Modem IP, QSPI, đo đạc

## Task 9: IP thanh ghi trên PL + `modem-regs` trên board

**Description:** Thêm vào Vivado một IP AXI-Lite (IP modem thật, hoặc IP thanh ghi giả có thanh ghi version cố định), xuất lại `.xsa`, chạy `petalinux-config --get-hw-description`, bật node `compatible = "nckh,modem-regs"` trong `system-user.dtsi`.

**Acceptance criteria:**
- [ ] `/dev/modem_*` xuất hiện khi boot
- [ ] Đọc thanh ghi version ra đúng giá trị; ghi rồi đọc lại thanh ghi scratch thấy khớp
- [ ] Đọc/ghi lệch căn lề trả `EINVAL`

**Verification:**
- [ ] Manual check: `dd if=/dev/modem_* bs=4 count=1 | hexdump`; mục modem trong `zynq-selftest` PASS

**Dependencies:** T5, T8

**Files likely touched:** `hw/e316/system.xsa`, `plnx/e316/project-spec/meta-user/recipes-bsp/device-tree/files/system-user.dtsi`, `meta-nckh/recipes-apps/zynq-selftest/files/zynq-selftest`

**Estimated scope:** S

## Task 10: Boot từ QSPI *(ngoài phạm vi lịch 1 tuần, chỉ làm nếu còn thời gian)*

**Description:** Khai báo phân vùng QSPI (`petalinux-config → Subsystem AUTO Hardware Settings → Flash Settings`), ghi `BOOT.BIN` bằng `flashcp` trên board, boot với boot mode QSPI; rootfs vẫn trên thẻ SD.

**Acceptance criteria:**
- [ ] `/proc/mtd` liệt kê đúng các phân vùng
- [ ] Boot mode QSPI: U-Boot chạy từ QSPI, boot tới login

**Verification:**
- [ ] Manual check: `cat /proc/mtd`; log UART khi boot từ QSPI

**Dependencies:** T8

**Files likely touched:** `plnx/e316/project-spec/configs/config`, `README.md`

**Estimated scope:** S

## Task 11: Đo đạc + báo cáo kiểm thử

**Description:** Đo các chỉ số ở README §6.3: boot time, iperf3, kích thước rootfs, RAM trống. Chạy liên tục khoảng 12 giờ qua đêm và khởi động lại 20 lần (bản rút gọn cho lịch 1 tuần; bản đầy đủ là 24 giờ và 100 lần). Ghi kết quả vào `docs/test-report.md`.

**Acceptance criteria:**
- [ ] Có số đo thật cho từng chỉ số, so với mục tiêu
- [ ] 20/20 lần reboot thành công, hoặc ghi rõ các lần lỗi

**Verification:**
- [ ] Manual check: log thô lưu trong `tasks/logs/`, báo cáo trích dẫn đúng số

**Dependencies:** T8 (T9, T10 nếu đo cả các phần đó)

**Files likely touched:** `docs/test-report.md`

**Estimated scope:** S

## Checkpoint 4
- [ ] Đạt tiêu chí README §6 (bản rút gọn)
- [ ] Commit + push; người dùng duyệt
