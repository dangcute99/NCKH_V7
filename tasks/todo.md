# Task list (PetaLinux 2021.1, E316, làm từ đầu)

Plan: [plan.md](plan.md). Quy trình: [docs/quy-trinh-xay-dung.md](../docs/quy-trinh-xay-dung.md). Mọi lệnh chạy code dự án (`petalinux-*`, Vivado batch, ghi thẻ SD, chạy trên board) đều phải hỏi người dùng trước.

`_ZynQ_Study/` **chỉ để tham khảo**: được đọc để đối chiếu thông số, không chép file, XSA hay image từ đó.

Quy ước: `$P` = `plnx/e316`; mỗi terminal PetaLinux chạy `source /opt/pkg/petalinux/2021.1/settings.sh /opt/pkg/petalinux/2021.1` và `stty rows 40 cols 120` trước menuconfig. Vivado: `source /tools/Xilinx/Vivado/2021.1/settings64.sh`.

---

## Phase 1: Công cụ + đường boot tối thiểu

## Task 1: Máy build sẵn sàng *(người dùng)*

**Description:** Đổi `/bin/sh` sang bash; cài 29 gói host của PetaLinux (danh sách đã kiểm, xem [docs/lo-trinh.md](../docs/lo-trinh.md) mục 2) và 4 gói của Vivado (`libtinfo5 libncurses5 default-jre libcanberra-gtk-module`); tạo `/opt/pkg` và `/tools/Xilinx` thuộc user; thêm user vào `dialout`. Tải `petalinux-v2021.1-final-installer.run` và `Xilinx_Unified_2021.1_0610_2318_Lin64.bin` (web installer) từ AMD Archive 2021.1; tạo token đăng nhập bằng `xsetup -b AuthTokenGen`.

**Acceptance criteria:**
- [x] `readlink -f /bin/sh` ra `/usr/bin/bash` (23/10)
- [x] Đủ 29 gói host của PetaLinux (23/10)
- [x] Đủ 4 gói của Vivado (`libtinfo5 libncurses5 default-jre libcanberra-gtk-module`) (23/10)
- [x] Có 2 bộ cài; `/opt/pkg/petalinux/2021.1` thuộc user (23/10)
- [x] `/tools/Xilinx` thuộc user; user trong nhóm `dialout` (23/10; `dialout` có hiệu lực sau khi đăng nhập lại)
- [x] `xsetup -b AuthTokenGen` báo tạo token thành công (23/10, hạn tới 30/10/2026)

**Verification:**
- [x] Manual check: vòng `dpkg-query` kiểm 33 gói; `ls -ld /opt/pkg/petalinux/2021.1 /tools/Xilinx`; `getent group dialout` (23/10)

**Dependencies:** None

**Files likely touched:** không có file trong repo

**Estimated scope:** XS

## Task 2: Cài PetaLinux 2021.1

**Description:** `./petalinux-v2021.1-final-installer.run -d /opt/pkg/petalinux/2021.1` (không root).

**Acceptance criteria:**
- [x] `source …/settings.sh …` không lỗi (23/10; chỉ cảnh báo thiếu tftp server — không cần vì boot SD)
- [x] `petalinux-create --help` chạy được

**Verification:**
- [x] Manual check: `which petalinux-build petalinux-create petalinux-package` (cài tại `/opt/pkg/petalinux/2021.1`, 13 GB)

**Dependencies:** T1

**Files likely touched:** không có file trong repo

**Estimated scope:** XS

## Task 3: Cài Vivado 2021.1 (ML Standard, Zynq-7000)

**Description:** `xsetup -b ConfigGen` sinh file cấu hình; sửa để chỉ chọn Vivado, edition ML Standard, họ thiết bị Zynq-7000, đích `/tools/Xilinx`. Cài ở chế độ batch, chạy nền (1–2 giờ). Người dùng chạy script cài driver cáp JTAG bằng sudo sau khi cài xong. Không chạy cùng lúc với `petalinux-build`.

**Tiến độ 23/10:** đã sinh `~/.Xilinx/install_config.txt` (Vivado ML Standard, chỉ `Zynq-7000:1`, không DocNav/Model Composer/shortcut). Lệnh cài: `~/Downloads/xil2021.1/xsetup -a XilinxEULA,3rdPartyEULA,WebTalkTerms -b Install -c ~/.Xilinx/install_config.txt` — chờ `/tools/Xilinx` (T1). Token hết hạn 30/10: cài trước ngày đó, quá hạn thì chạy lại `AuthTokenGen`.

**Acceptance criteria:**
- [x] `vivado -version` in `v2021.1` (23/10)
- [x] `vivado -mode batch -source <tcl tạo project rỗng xc7z020clg400-1>` chạy không lỗi (tạo được project + cell `processing_system7`)
- [x] Dung lượng `/tools/Xilinx` ghi vào nhật ký (27 GB)

**Verification:**
- [x] Manual check: lệnh trên + `du -sh /tools/Xilinx`
- [x] Driver cáp JTAG (23/10): 3 rule `52-xilinx-*.rules` trong `/etc/udev/rules.d/`
- [ ] `hw_server` thấy cáp khi cắm board — kiểm khi có board, không chặn T4/T5 (boot SD)

**Dependencies:** T1

**Files likely touched:** không có file trong repo

**Estimated scope:** S

## Task 4: XSA tối thiểu cho E316 (PS: DDR, UART1, SD0)

**Description:** Tạo project Vivado `hw/e316/vivado/` bằng script Tcl `hw/e316/create_project.tcl` (để tái lập được, không commit thư mục sinh ra). Block design chỉ có ZYNQ7 Processing System: part `xc7z020clg400-1`; DDR3 `MT41J256M16 RE-125`, bus 32-bit (thông số delay/board đối chiếu `system_bd.tcl` vàng trong `_ZynQ_Study/`, chỉ đọc); UART1 MIO 12–13 115200; SD0; tắt TTC0 và ngoại vi không dùng. Generate bitstream, Export Hardware kèm bitstream → `hw/e316/system.xsa`.

**Acceptance criteria:**
- [ ] `hw/e316/create_project.tcl` chạy lại từ đầu ra cùng XSA (tái lập được)
- [ ] `unzip -l hw/e316/system.xsa` có `.bit`, `ps7_init.c/.tcl`, `.hwh`
- [ ] Bảng đối chiếu thông số DDR/MIO (của mình vs tham khảo) ghi trong `hw/e316/README.md`

**Verification:**
- [ ] Build succeeds: `vivado -mode batch -source hw/e316/create_project.tcl` tới bước write_hw_platform
- [ ] Manual check: `grep -E 'PCW_UART1|PCW_UIPARAM_DDR_PARTNO|PCW_SD0' ` trong `.hwh` khớp bảng đối chiếu

**Dependencies:** T3

**Files likely touched:**
- `hw/e316/create_project.tcl`
- `hw/e316/system.xsa`
- `hw/e316/README.md`
- `.gitignore`

**Estimated scope:** M

## Task 5: E316 boot tới login qua UART (INITRAMFS)

**Description:** Trong `plnx/`: `petalinux-create -t project --template zynq -n e316`; trong `$P`: `petalinux-config --get-hw-description=../../hw/e316/system.xsa`, console `ps7_uart_1`, giữ rootfs INITRAMFS; thêm `INHERIT += "rm_work"`. `petalinux-build` (lần đầu 1–2 giờ); `petalinux-package --boot --fsbl images/linux/zynq_fsbl.elf --fpga images/linux/system.bit --u-boot --force`. Chép `BOOT.BIN`, `image.ub`, `boot.scr` lên phân vùng FAT, jumper SD, `picocom -b 115200 /dev/ttyUSB0`.

**Acceptance criteria:**
- [ ] Log UART đi qua FSBL → U-Boot 2021.01 → kernel 5.10 → login
- [ ] Đăng nhập `root`/`root`; `grep -c processor /proc/cpuinfo` ra 2; `free -m` khớp DDR thiết kế
- [ ] `cat /sys/class/fpga_manager/fpga0/state` là `operating`

**Verification:**
- [ ] Build succeeds: `petalinux-build` + `petalinux-package --boot`
- [ ] Manual check: log `tasks/logs/t5-boot.log`; `bootgen -arch zynq -read images/linux/BOOT.BIN`; snapshot vào `$P/deploy/01_<ngày>_initramfs/`

**Dependencies:** T2, T4

**Files likely touched:**
- `plnx/e316/project-spec/configs/config`
- `plnx/e316/project-spec/meta-user/conf/petalinuxbsp.conf`

**Estimated scope:** M

## Checkpoint 1
- [ ] E316 login UART bằng XSA + image tự làm
- [ ] Ghi thời gian build đầu, dung lượng `$P/build`; người dùng duyệt

---

## Phase 2: Hệ thống dùng được

## Task 6: Ethernet + SSH (Vivado + DT)

**Description:** Vivado (sửa `create_project.tcl`): ENET0 qua EMIO + MDIO EMIO, IP `gmii_to_rgmii` với clock FCLK_CLK2 200 MHz, ràng buộc chân RGMII trong `hw/e316/e316.xdc` (đối chiếu `system.xdc` tham khảo, chỉ đọc). Xuất XSA mới. PetaLinux: `--get-hw-description`, `system-user.dtsi` cho PHY RTL8211F @ MDIO 1 `rgmii-id`, converter `gmiitorgmii@8`, tắt `gmii_to_rgmii_0`; bật dropbear; `petalinux-package --boot` (FPGA đổi).

**Acceptance criteria:**
- [ ] `ip link show eth0` UP, có link
- [ ] PC ping board; `ssh root@<IP>` vào được

**Verification:**
- [ ] Build succeeds: Vivado batch tới XSA; `petalinux-build` + `petalinux-package --boot --force`
- [ ] Manual check: `dtc -I dtb -O dts images/linux/system.dtb | grep -B2 -A6 phy-mode`; ping + ssh; snapshot `deploy/02_<ngày>_eth/`

**Dependencies:** T5

**Files likely touched:**
- `hw/e316/create_project.tcl`
- `hw/e316/e316.xdc`
- `hw/e316/system.xsa`
- `plnx/e316/project-spec/meta-user/recipes-bsp/device-tree/files/system-user.dtsi`

**Estimated scope:** M

## Task 7: Rootfs EXT4 trên thẻ SD (`.wic`)

**Description:** `petalinux-config`: rootfs EXT4, SD device `/dev/mmcblk0p2`, bootargs `console=ttyPS0,115200 earlycon root=/dev/mmcblk0p2 rw rootwait uio_pdrv_genirq.of_id=generic-uio`. Build, `petalinux-package --boot … --force`, `petalinux-package --wic`. Người dùng xác nhận `/dev/sdX` rồi ghi `dd`.

**Acceptance criteria:**
- [ ] `VFS: Mounted root (ext4 filesystem)`; `/` là `/dev/mmcblk0p2` rw
- [ ] File `/root/t7-ok` còn sau reboot
- [ ] Ethernet + SSH của T6 vẫn chạy

**Verification:**
- [ ] Build succeeds: `petalinux-build` + `--boot` + `--wic`
- [ ] Manual check: `sfdisk -l images/linux/petalinux-sdimage.wic` thấy 2 phân vùng; log `tasks/logs/t7-ext4.log`

**Dependencies:** T6

**Files likely touched:**
- `plnx/e316/project-spec/configs/config`

**Estimated scope:** S

## Task 8: Layer `meta-modem` + kernel fragment

**Description:** User Layers thêm `${PROOT}/../../meta-modem`; build lại kernel với `modem.cfg`. Lần đầu kiểm `LAYERSERIES_COMPAT = "gatesgarth"`, `_append`, license `GPL-2.0`.

**Acceptance criteria:**
- [ ] Build có `meta-modem`, không cảnh báo compat
- [ ] Mọi option của `modem.cfg` đúng trong `.config` kernel
- [ ] Board boot lại; có `/sys/class/fpga_manager/fpga0`, `/dev/i2c-*`

**Verification:**
- [ ] Build succeeds: `petalinux-build -c kernel && petalinux-build`
- [ ] Manual check: grep từng `CONFIG_*` của `modem.cfg` trong `.config` (`find build/tmp/work -path '*linux-xlnx*' -name .config`)

**Dependencies:** T7

**Files likely touched:**
- `plnx/e316/project-spec/configs/config`
- `meta-modem/conf/layer.conf`, `meta-modem/recipes-kernel/linux/*` (nếu cần sửa)

**Estimated scope:** S

## Checkpoint 2
- [ ] Boot từ `.wic`, SSH được, dữ liệu còn sau reboot, kernel có option modem
- [ ] `git status` chỉ còn file nguồn (`project-spec/`, `hw/e316/*.tcl|*.xdc|*.xsa|README.md`)
- [ ] Commit + push; người dùng duyệt

---

## Phase 3: Kiểm thử và driver modem

## Task 9: `zynq-selftest` chạy trên board

**Description:** Bật `zynq-selftest`, ethtool, iperf3, i2c-tools, spitools, mtd-utils, memtester trong rootfs (thiếu trong menu thì thêm vào `user-rootfsconfig`). Chạy với `EXPECT_CPUS=2 EXPECT_MEM_MB=<DDR thiết kế> PING_HOST=<IP PC>`.

**Acceptance criteria:**
- [ ] PASS: CPU, RAM, kernel log, rootfs rw, UART, SD, eth0 + ping, PL `operating`
- [ ] Mục modem báo WARN (chưa có IP); ép FAIL thì mã thoát ≠ 0

**Verification:**
- [ ] Build succeeds: `petalinux-build`
- [ ] Manual check: `zynq-selftest; echo $?`, log `tasks/logs/t9-selftest.log`

**Dependencies:** T8

**Files likely touched:**
- `plnx/e316/project-spec/configs/rootfs_config`
- `plnx/e316/project-spec/meta-user/conf/user-rootfsconfig`

**Estimated scope:** S

## Task 10: IP AXI-Lite (version + scratch) + `modem-regs`

**Description:** Vivado: tạo IP AXI4-Lite (thanh ghi 0 = version cố định, thanh ghi 1 = scratch), nối GP0, gán địa chỉ, xuất XSA. PetaLinux: `--get-hw-description`, node `compatible = "modem,modem-regs"` trong `system-user.dtsi`, bật gói `modem-regs`, `petalinux-package --boot`; cập nhật mục modem của `zynq-selftest` kiểm giá trị version.

**Acceptance criteria:**
- [ ] `/dev/modem_<địa chỉ>` có; đọc thanh ghi 0 đúng version; scratch ghi/đọc khớp
- [ ] Truy cập lệch căn lề trả `EINVAL`; `rmmod`/`modprobe` sạch
- [ ] `zynq-selftest` mục modem PASS

**Verification:**
- [ ] Build succeeds: Vivado batch tới XSA; `petalinux-build` + `--boot --force` + `--wic`
- [ ] Manual check: `dd if=/dev/modem_<addr> bs=4 count=1 | hexdump -C`; log `tasks/logs/t10-ip.log`

**Dependencies:** T8 (T9 để cập nhật selftest)

**Files likely touched:**
- `hw/e316/create_project.tcl`, `hw/e316/ip/` (nguồn IP), `hw/e316/system.xsa`
- `plnx/e316/project-spec/meta-user/recipes-bsp/device-tree/files/system-user.dtsi`
- `meta-modem/recipes-apps/zynq-selftest/files/zynq-selftest`

**Estimated scope:** M

## Checkpoint 3
- [ ] selftest PASS PS + PL + modem; driver kiểm trên phần cứng thật
- [ ] Commit + push; người dùng duyệt

---

## Phase 4: Đo đạc

## Task 11: Đo đạc + báo cáo kiểm thử

**Description:** Đo theo README §6.3: boot time (`printk.time=1`, mốc log UART), `iperf3` hai chiều, dung lượng rootfs, RAM trống; chạy ~12 giờ + reboot 20 lần. Ghi `docs/test-report.md`.

**Acceptance criteria:**
- [ ] Số đo thật cho từng chỉ số, so với mục tiêu (boot < 10 s, iperf3 > 500 Mbit/s, rootfs < 64 MB, RAM trống > 80%)
- [ ] 20/20 lần reboot tới login, hoặc ghi rõ lần lỗi

**Verification:**
- [ ] Manual check: log thô `tasks/logs/t11-*`; báo cáo trích đúng số

**Dependencies:** T7, T9 (T10 nếu đo cả IP)

**Files likely touched:**
- `docs/test-report.md`, `tasks/logs/`

**Estimated scope:** S

## Task 12: Boot QSPI *(tuỳ chọn)*

**Description:** Bật QSPI trong PS (Vivado), khai báo phân vùng flash trong `petalinux-config`, ghi `BOOT.BIN` bằng `flashcp`, đổi jumper boot QSPI (cần schematic E316).

**Acceptance criteria:**
- [ ] `cat /proc/mtd` đúng phân vùng
- [ ] Boot mode QSPI tới login

**Verification:**
- [ ] Manual check: log `tasks/logs/t12-qspi.log`

**Dependencies:** T7

**Files likely touched:**
- `hw/e316/create_project.tcl`, `plnx/e316/project-spec/configs/config`

**Estimated scope:** S

## Checkpoint 4
- [ ] Đạt tiêu chí README §6 (bản rút gọn)
- [ ] Commit + push; người dùng duyệt
