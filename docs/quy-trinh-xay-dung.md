# Quy trình xây dựng và khởi động Linux trên Zynq-7000 (PetaLinux 2023.2)

Tài liệu đi qua **toàn bộ chuỗi**: từ thiết kế phần cứng và build bằng PetaLinux trên máy tính, tới lúc BootROM trong chip chạy lệnh đầu tiên, rồi tới ứng dụng modem chạy trên Linux. Mỗi bước nêu rõ đầu vào, đầu ra, **PetaLinux làm gì bên dưới**, cách kiểm tra và cách khoanh vùng khi bước đó lỗi.

Bối cảnh, mục tiêu và phiên bản phần mềm xem trong [README.md](../README.md). Tiến độ từng task xem trong [tasks/todo.md](../tasks/todo.md).

Quy ước: `$P` = thư mục project PetaLinux của board (vd. `plnx/e316`); lệnh `petalinux-*` chạy trong `$P` sau khi `source ~/petalinux/2023.2/settings.sh`.

```
          TRÊN MÁY TÍNH (build)                                       TRÊN BOARD (boot)
 ┌────────────────────────────────────────────┐        ┌────────────────────────────────────────────┐
 │ B0 Cài PetaLinux 2023.2                    │        │ R1 Bật nguồn, đọc chân boot mode           │
 │ B1 Vivado ──► system.xsa                   │        │ R2 BootROM ──► nạp FSBL vào OCM            │
 │ B2 petalinux-create + --get-hw-description │        │ R3 FSBL ──► DDR, bitstream, U-Boot, DTB    │
 │ B3 petalinux-config (hệ thống)             │        │ R4 U-Boot ──► boot.scr ──► image.ub        │
 │ B4 U-Boot      B5 Kernel                   │  thẻ   │ R5 Kernel ──► driver, mount rootfs         │
 │ B6 Device Tree B7 RootFS + driver + app    │ ─SD──► │ R6 init (BusyBox) ──► dịch vụ, login       │
 │ B8 petalinux-build + package (BOOT.BIN,wic)│        │ R7 Ứng dụng modem ──► /dev/modem_*, UIO     │
 │ B9 Ghi thẻ SD / JTAG / QEMU                │        │                                            │
 └────────────────────────────────────────────┘        └────────────────────────────────────────────┘
```

Thứ tự ở trục build (B) **ngược** với trục boot (R). File được dùng sớm nhất khi boot (FSBL, ở R2–R3) lại phụ thuộc vào bước làm sớm nhất khi build (thiết kế phần cứng, ở B1–B2). Còn file được dùng muộn nhất (rootfs, ở R5–R6) chỉ xong ở gần cuối quá trình build.

---

## Phần A — Build trên máy tính (PetaLinux)

### B0. Cài PetaLinux 2023.2

| | |
|---|---|
| **Đầu vào** | `petalinux-v2023.2-*-installer.run` (tải từ trang AMD, cần tài khoản), Ubuntu 22.04 |
| **Làm gì** | Cài gói host theo UG1144 (script `plnx-env-setup.sh` của AMD). Cài **không dùng root**: `./petalinux-v2023.2-*-installer.run -d ~/petalinux/2023.2`. Mỗi terminal: `source ~/petalinux/2023.2/settings.sh` |
| **Đầu ra** | `~/petalinux/2023.2/` (~10 GB): các lệnh `petalinux-*`, toolchain, XSCT nội bộ, QEMU của Xilinx |
| **Kiểm tra** | `which petalinux-build`; `petalinux-create --help` không lỗi |
| **Trạng thái** | Chưa làm (Task 1) |

Nếu installer báo thiếu gói thì cài theo đúng danh sách nó in ra. Nếu nó báo lỗi về `/bin/sh` thì chạy `sudo dpkg-reconfigure dash` và chọn **No**, để `/bin/sh` trỏ tới bash.

### B1. Thiết kế phần cứng (Vivado)

| | |
|---|---|
| **Đầu vào** | Schematic board E316 (DDR, MIO, clock, PHY) |
| **Làm gì** | Block design `ZYNQ7 Processing System`: cấu hình DDR, gán MIO cho UART/SD/QSPI/ENET/I2C, clock. Thêm IP modem trên PL nếu có. Generate bitstream → Export Hardware (kèm bitstream) |
| **Đầu ra** | `hw/e316/system.xsa` (zip gồm `.bit`, `ps7_init.c/.tcl`, mô tả phần cứng `.hwh`) |
| **Kiểm tra** | `unzip -l hw/e316/system.xsa` thấy `.bit` và `ps7_init*` |
| **Trạng thái** | Chưa làm (Task 6) |

Mọi thông số phần cứng của board đều đi vào hệ thống qua file này. Nếu cấu hình DDR sai ở đây thì board sẽ chết ở R3, kể cả khi mọi bước build phía sau đều thành công. **Vivado phải cùng phiên bản với PetaLinux (2023.2).**

**Vivado không cài trên máy build** (để tiết kiệm ổ đĩa). Bước B1 làm ở máy khác có Vivado 2023.2, hoặc dùng `.xsa` do nhà cung cấp board phát hành (phải đúng bản 2023.2). Kết quả chỉ cần chép file `system.xsa` sang `hw/<board>/` trên máy build.

### B2. Tạo project và nhập phần cứng

| | |
|---|---|
| **Đầu vào** | `system.xsa` |
| **Làm gì** | `petalinux-create -t project --template zynq -n e316` rồi `petalinux-config --get-hw-description=../../hw/e316/system.xsa` |
| **PetaLinux làm gì bên dưới** | Chép `.xsa` vào `project-spec/hw-description/`; chạy XSCT để đọc phần cứng: danh sách ngoại vi PS, IP trên PL, địa chỉ, UART, bộ nhớ → sinh `project-spec/configs/config` và mở menu cấu hình hệ thống |
| **Đầu ra** | Khung project `$P/` (`project-spec/`, `components/`, `config.project`) |
| **Kiểm tra** | `ls $P/project-spec/hw-description/` thấy `.xsa`; menu hiện đúng tên UART/ethernet của thiết kế |
| **Trạng thái** | Chưa làm (Task 7). Trước đó thử bằng BSP ZC702: `petalinux-create -t project -s xilinx-zc702-v2023.2-final.bsp` (Task 2) |

### B3. Cấu hình hệ thống (`petalinux-config`)

| Mục trong menu | Đặt thành | Vì sao |
|----------------|-----------|--------|
| `Subsystem AUTO Hardware Settings → Serial Settings` | UART ra console của E316 (`ps7_uart_0` hoặc `_1`) | log FSBL/U-Boot/kernel ra đúng cổng |
| `Subsystem AUTO Hardware Settings → Ethernet Settings` | `ps7_ethernet_0`, DHCP | mạng cho SSH, iperf3 |
| `Image Packaging Configuration → Root filesystem type` | **EXT4 (SD/eMMC/SATA/USB)** | rootfs nằm trên phân vùng 2 của thẻ SD, ghi được và giữ dữ liệu khi tắt nguồn (mặc định là INITRAMFS, mất dữ liệu khi tắt) |
| `Image Packaging Configuration → Device node of SD device` | `/dev/mmcblk0p2` | để kernel biết mount rootfs ở đâu (`root=`) |
| `DTG Settings → Kernel Bootargs` | bỏ chọn *generate boot args automatically*, đặt `console=ttyPS0,115200 earlycon root=/dev/mmcblk0p2 rw rootwait uio_pdrv_genirq.of_id=generic-uio` | thêm `generic-uio` cho IP modem |
| `Yocto Settings → User Layers` | `${PROOT}/../../meta-nckh` | nạp kernel fragment, driver, ứng dụng dùng chung |

| | |
|---|---|
| **Đầu ra** | `$P/project-spec/configs/config` |
| **Kiểm tra** | `grep -E 'ROOTFS_EXT4|SDROOT_DEV|USER_LAYER' project-spec/configs/config` |

### B4. U-Boot

| | |
|---|---|
| **Đầu vào** | Cấu hình sinh từ `.xsa` + `project-spec/meta-user/recipes-bsp/u-boot/files/{platform-top.h,bsp.cfg}` |
| **Làm gì** | Thường không cần sửa. Chỉnh tương tác: `petalinux-config -c u-boot` |
| **PetaLinux làm gì bên dưới** | Build recipe `u-boot-xlnx` (2023.01, defconfig `xilinx_zynq_virt`), sinh thêm `boot.scr` từ mẫu `u-boot-zynq-scr` |
| **Đầu ra** | `images/linux/u-boot.elf`, `images/linux/boot.scr` |
| **Kiểm tra** | `file images/linux/u-boot.elf` là ARM ELF |
| **Trạng thái** | Chưa làm (Task 2 trên ZC702, Task 8 trên E316) |

U-Boot của Xilinx bật `CONFIG_OF_BOARD`: khi chạy, nó lấy Device Tree ở địa chỉ `0x100000` (`CONFIG_XILINX_OF_BOARD_DTB_ADDR`), nơi FSBL đặt sẵn từ `BOOT.BIN`. Nhờ vậy một bản build U-Boot dùng chung được cho mọi board Zynq-7000.

### B5. Linux kernel

| | |
|---|---|
| **Đầu vào** | `linux-xlnx` 6.1 + `xilinx_zynq_defconfig` + `meta-nckh/recipes-kernel/linux/linux-xlnx/nckh.cfg` |
| **Làm gì** | Thêm option vào `nckh.cfg`, được `linux-xlnx_%.bbappend` nạp vào (`SRC_URI += "file://nckh.cfg"`). Muốn thử tương tác: `petalinux-config -c kernel` (thay đổi được lưu thành fragment trong `meta-user`; giữ lại thì chép sang `nckh.cfg`) |
| **PetaLinux làm gì bên dưới** | Yocto trộn defconfig với các fragment, biên dịch kernel, đóng gói kernel và `system.dtb` vào ảnh FIT `image.ub` |
| **Đầu ra** | `images/linux/zImage`, `images/linux/image.ub` |
| **Kiểm tra** | Grep từng option của `nckh.cfg` trong `.config` của kernel (`find build/tmp/work -path '*linux-xlnx*' -name .config`) |
| **Trạng thái** | Chưa làm (Task 3) |

### B6. Device Tree

| | |
|---|---|
| **Đầu vào** | `.xsa` + `project-spec/meta-user/recipes-bsp/device-tree/files/system-user.dtsi` (viết tay) |
| **PetaLinux làm gì bên dưới** | Chạy XSCT + `device-tree-xlnx` để sinh `system-top.dts`, `zynq-7000.dtsi`, `pcw.dtsi` (ngoại vi PS), `pl.dtsi` (IP trên PL) vào `components/plnx_workspace/device-tree/device-tree/`; ghép thêm `system-user.dtsi`; biên dịch bằng `dtc` |
| **Đầu ra** | `images/linux/system.dtb` (nằm trong `image.ub` và trong `BOOT.BIN`) |
| **Kiểm tra** | `dtc -I dtb -O dts images/linux/system.dtb | grep -A3 chosen`; có node ethernet, node IP modem |
| **Trạng thái** | Chưa làm (Task 7) |

Nguyên tắc: **không sửa file trong `components/`**, vì chúng bị sinh lại mỗi lần build. Mọi chỉnh tay đặt trong `system-user.dtsi`, nhờ vậy xuất lại `.xsa` không làm mất phần đã sửa.

### B7. Root File System, driver và ứng dụng

| | |
|---|---|
| **Đầu vào** | `petalinux-config -c rootfs` + recipe trong `meta-nckh` |
| **Làm gì** | Bật trong menu: dropbear, ethtool, iperf3, i2c-tools, spitools, mtd-utils, memtester, `modem-regs`, `zynq-selftest`. Gói không có trong menu thì thêm dòng `CONFIG_<tên>` vào `project-spec/meta-user/conf/user-rootfsconfig` |
| **Tạo recipe mới** | Driver: `petalinux-create -t modules --name modem-regs` (sinh `.bb` + `.c` mẫu, sau đó chuyển sang `meta-nckh/recipes-modules/`). Ứng dụng: `petalinux-create -t apps --template install --name zynq-selftest` |
| **PetaLinux làm gì bên dưới** | Yocto build từng gói (BusyBox, glibc, ...), cài vào ảnh rootfs |
| **Đầu ra** | `images/linux/rootfs.ext4`, `rootfs.tar.gz` |
| **Kiểm tra** | `debugfs -R 'ls -l /usr/bin' images/linux/rootfs.ext4`; tìm `modem-regs.ko` trong `/lib/modules/` |
| **Trạng thái** | Chưa làm (Task 4, Task 5) |

### B8. Build và đóng gói

Máy build chỉ còn ~80 GB, nên trước lần build đầu thêm dòng sau vào `project-spec/meta-user/conf/petalinuxbsp.conf` để Yocto xóa thư mục tạm của từng gói ngay khi build xong (thư mục `build/` giảm từ ~40–50 GB còn ~15–25 GB; đổi lại khó xem log khi một gói build lỗi — tạm bỏ dòng này khi cần gỡ lỗi):

```
INHERIT += "rm_work"
```

```sh
petalinux-build                                    # lần đầu 1–2 giờ, cần mạng
petalinux-package --boot --fsbl images/linux/zynq_fsbl.elf \
                  --fpga images/linux/system.bit --u-boot --force
petalinux-package --wic
```

| | |
|---|---|
| **PetaLinux làm gì bên dưới** | `petalinux-build` chạy `bitbake` cho FSBL, U-Boot, DT, kernel, rootfs. `--boot` sinh file `.bif` rồi gọi `bootgen`. `--wic` dùng Yocto `wic` tạo ảnh thẻ SD từ các file trong `images/linux/` |
| **Đầu ra** | `BOOT.BIN`, `petalinux-sdimage.wic` (cùng các file ở bảng dưới) |
| **Kiểm tra** | `bootgen -arch zynq -read images/linux/BOOT.BIN` liệt kê FSBL, bitstream, U-Boot, DTB; `sfdisk -l images/linux/petalinux-sdimage.wic` thấy 2 phân vùng |
| **Trạng thái** | Chưa làm (Task 2 trên ZC702, Task 8 trên E316) |

```
BOOT.BIN (bootgen)                         petalinux-sdimage.wic
┌──────────────────────────────┐           ┌─────────────────────────────────┐
│ [bootloader] zynq_fsbl.elf   │ ← R2 nạp  │ p1 FAT32 (boot)                 │
│ system.bit                   │ ← R3 nạp  │   BOOT.BIN                      │
│ u-boot.elf                   │ ← R3 nạp  │   boot.scr          ← R4 chạy    │
│ system.dtb @ 0x100000        │ ← R4 đọc  │   image.ub (kernel+DTB) ← R4 nạp│
└──────────────────────────────┘           ├─────────────────────────────────┤
                                           │ p2 ext4: rootfs     ← R5 mount  │
                                           └─────────────────────────────────┘
```

> **Cần kiểm tra lại:** việc PetaLinux tự đưa `system.dtb @ 0x100000` vào `BOOT.BIN`, và danh sách file trong phân vùng FAT của `.wic`, mình ghi theo hành vi mặc định của 2023.2. Xác nhận bằng `bootgen -read` và mount thử phân vùng 1. Nếu cần chỉnh thì dùng `--add`/`--bootfiles`.

### B9. Triển khai

| Cách | Lệnh | Dùng khi |
|------|------|----------|
| Thẻ SD | (1) chạy `lsblk -d -o NAME,SIZE,MODEL,TRAN` **trước và sau** khi cắm thẻ; thiết bị mới xuất hiện (cột `TRAN` là `usb`, dung lượng bằng thẻ) là `/dev/sdX` (2) `sudo dd if=images/linux/petalinux-sdimage.wic of=/dev/sdX bs=4M conv=fsync status=progress` (3) đặt boot mode **SD** | cách chính; ghi nhầm thiết bị sẽ **xóa sạch ổ đó** |
| JTAG | `petalinux-boot --jtag --kernel --fpga` (cần `hw_server` và cáp JTAG; máy build không có Vivado nên cài riêng gói *Hardware Server* của AMD) | bring-up board lần đầu, chưa tin thẻ SD |
| QEMU | `petalinux-boot --qemu --kernel` | không cần board; mô phỏng PS, **không có PL** |

Trước khi cấp nguồn, cắm UART-USB và mở `picocom -b 115200 /dev/ttyUSB0` để xem log từ R3.

---

## Phần B — Khởi động trên board (từ BootROM tới ứng dụng)

### R1. Bật nguồn và reset

- Nguồn ổn định thì PS thoát reset (`PS_POR_B`). CPU0 bắt đầu chạy mã trong **BootROM**, phần mã nằm cố định trong chip và không sửa được. CPU1 chờ.
- BootROM đọc **chân boot mode** (`MIO[5:3]`) để biết nạp từ đâu:

| Boot mode | MIO[5:3] |
|-----------|----------|
| JTAG | 000 |
| NOR | 001 |
| NAND | 010 |
| QSPI | 100 |
| **SD** | **110** |

> **Cần kiểm tra lại:** bảng trên lấy theo tài liệu Zynq-7000 (UG585). Vị trí jumper cụ thể trên E316 phải xem trong tài liệu của board.

**Lỗi ở bước này:** UART không in gì cả. Kiểm tra nguồn, đèn `DONE`/`INIT`, jumper boot mode, cáp UART và cổng UART console.

### R2. BootROM nạp FSBL

- Với boot mode SD, BootROM khởi tạo bộ điều khiển SD0 rồi tìm file `BOOT.BIN` trên **phân vùng FAT đầu tiên**.
- Đọc boot header, chép FSBL vào **OCM** (bộ nhớ trên chip 256 KB; FSBL tối đa khoảng 192 KB). Nếu ảnh được mã hóa hoặc ký số thì giải mã/xác thực ở bước này.
- Nhảy vào FSBL. Lúc này **DDR chưa được khởi tạo**.

**Lỗi ở bước này:** UART không in gì. Nguyên nhân thường gặp: phân vùng 1 không phải FAT, tên file khác `BOOT.BIN`, thẻ SD hỏng, sai boot mode. Thử lại bằng thẻ khác, hoặc boot qua JTAG (B9) để loại trừ thẻ SD.

### R3. FSBL: cấu hình PS, nạp PL và U-Boot

1. Chạy `ps7_init` (sinh từ `.xsa`): cấu hình PLL/clock, MIO, **khởi tạo DDR**.
2. Nạp `system.bit` vào **PL** qua PCAP. Đèn `DONE` sáng nghĩa là FPGA đã được cấu hình.
3. Chép `u-boot.elf` vào DDR và `system.dtb` vào địa chỉ `0x100000`.
4. Chuyển quyền cho U-Boot.

**Log mong đợi:** `Xilinx First Stage Boot Loader`, `Release 2023.2`, `Devcfg driver initialized`, `Handoff Address: ...`.

**Lỗi ở bước này:**
- Treo sau dòng banner: thường do sai cấu hình DDR ở B1.
- Lỗi khi nạp bitstream: `.bit` không khớp chip.
- Không thấy log FSBL: kiểm tra UART đã được bật trong PS chưa (B1), và cổng console chọn ở B3.

### R4. U-Boot: chạy `boot.scr`, nạp `image.ub`

1. `board_fdt_blob_setup()` thấy DTB hợp lệ ở `0x100000` và dùng nó cho chính U-Boot (UART, SD, Ethernet).
2. Chạy *distro boot* (`bootcmd`): quét `mmc 0:1` và tìm **`boot.scr`**.
3. `boot.scr` nạp **`image.ub`** (ảnh FIT gồm kernel + `system.dtb`) từ FAT vào DDR, rồi `bootm` khởi động kernel kèm DTB lấy từ FIT. `bootargs` lấy từ node `chosen` của DTB (đặt ở B3).

**Log mong đợi:** `U-Boot 2023.01`, `Model: ...`, `DRAM: ...`, `Found U-Boot script /boot.scr`, `## Loading kernel from FIT Image`, `Starting kernel ...`.

**Lỗi ở bước này:**
- Không vào được U-Boot: DTB ở `0x100000` hỏng, hoặc chưa được đóng gói vào `BOOT.BIN`.
- Dừng ở dấu nhắc `Zynq>`: không tìm thấy `boot.scr` hoặc `image.ub`; gõ `ls mmc 0:1` để xem.
- Treo ngay sau `Starting kernel`: thêm `earlycon` vào bootargs (B3); kiểm tra `stdout-path`/`aliases` của UART trong DTB.

### R5. Linux kernel: driver và rootfs

1. Giải nén, bật MMU, đọc Device Tree. Bật CPU1 (SMP).
2. **Probe driver** theo các node DT có `status = "okay"`: `xuartps` (`ttyPS0`), `sdhci-of-arasan` (`mmcblk0`), `macb` (`eth0`), `spi-zynq-qspi`, `i2c-cadence`, `zynq-fpga`, `xilinx_dma`, `modem-regs` hoặc `uio_pdrv_genirq` cho IP modem.
3. Mount `root=/dev/mmcblk0p2` (ext4), sau đó chạy `/sbin/init`.

**Log mong đợi:** `Booting Linux on physical CPU 0x0`, `Machine model: ...`, `mmcblk0: p1 p2`, `macb ... eth0`, `VFS: Mounted root (ext4 filesystem)`, `Run /sbin/init`.

**Lỗi ở bước này:**
- `Kernel panic - not syncing: VFS: Unable to mount root fs`: rootfs type chưa đặt EXT4, sai `root=` (B3), hoặc thiếu `rootwait`.
- Treo không có log: thêm `earlycon` vào bootargs.
- Driver không probe: kiểm tra node DT (`/proc/device-tree`) và `dmesg | grep -i probe`.

### R6. init (BusyBox): dịch vụ và đăng nhập

1. `/etc/inittab` chạy các script khởi động (sysvinit/BusyBox):
   - `udev` hoặc `mdev`: tạo `/dev`, tự nạp module theo modalias.
   - Mạng: DHCP cho `eth0`.
   - `dropbear`: SSH.
2. Chạy `getty` trên `ttyPS0` và hiện dấu nhắc login (PetaLinux 2023.2 mặc định: user `petalinux`, phải đặt mật khẩu ở lần đăng nhập đầu).

**Kiểm tra:** chạy `zynq-selftest`. Lệnh in PASS/WARN/FAIL cho CPU, RAM, kernel log, rootfs, UART, SD, Ethernet, QSPI, PL và driver modem, và trả mã thoát khác 0 nếu có FAIL (Task 4).

**Lỗi ở bước này:** không có dấu nhắc login thì xem lại cổng serial chọn ở B3. Không có mạng thì xem `ip link`, `ethtool eth0` và PHY trong DT.

### R7. Ứng dụng modem

- Ứng dụng user-space điều khiển IP modem trên PL theo một trong hai cách:
  - **Driver `modem-regs`:** dùng `/dev/modem_<địa_chỉ>`, đọc/ghi thanh ghi 32-bit căn lề, offset trong file chính là offset thanh ghi.
  - **`generic-uio`:** dùng `/dev/uioN`, `mmap` vùng thanh ghi và chờ ngắt bằng `read()`.
- Mẫu tín hiệu đi giữa PL và DDR qua AXI DMA (`xilinx_dma`).
- Cấu hình front-end RF qua SPI/I2C (`spidev`, `i2c-dev`) hoặc driver IIO (ví dụ AD9361, ngoài phạm vi giai đoạn này).
- Ứng dụng mới tạo bằng `petalinux-create -t apps` rồi đưa vào `meta-nckh`.
- Có thể nạp lại bitstream khi đang chạy qua `fpga_manager` và DT overlay.

---

## Phụ lục — Bảng tra nhanh

### File ↔ bước sinh ra ↔ bước sử dụng

Mọi file (trừ `.xsa`) nằm trong `$P/images/linux/`.

| File | Sinh ở | Dùng ở | Nằm trên thẻ SD ở |
|------|--------|--------|-------------------|
| `system.xsa` | B1 | B2 | — |
| `zynq_fsbl.elf` | B8 (từ `.xsa`) | R2–R3 | trong `BOOT.BIN` |
| `system.bit` | B1 (tách từ `.xsa`) | R3 | trong `BOOT.BIN` |
| `u-boot.elf` | B4/B8 | R4 | trong `BOOT.BIN` |
| `system.dtb` | B6/B8 | R4 (0x100000), R5 | trong `BOOT.BIN` + trong `image.ub` |
| `boot.scr` | B4/B8 | R4 | p1 FAT |
| `image.ub` | B5/B8 | R4–R5 | p1 FAT |
| `rootfs.ext4` | B7/B8 | R5–R7 | p2 ext4 |
| `BOOT.BIN` | B8 (`--boot`) | R2–R4 | p1 FAT |
| `petalinux-sdimage.wic` | B8 (`--wic`) | B9 | (ảnh của cả thẻ) |

### Thấy gì trên UART → lỗi ở đâu

| Hiện tượng | Bước lỗi | Hướng xử lý đầu tiên |
|------------|----------|----------------------|
| Không in gì | R1–R2 | Jumper boot mode, nguồn, cổng/cáp UART, `BOOT.BIN` trên FAT; thử boot JTAG |
| Có banner FSBL rồi treo | R3 | Cấu hình DDR trong Vivado (B1) |
| Dừng ở `Zynq>` | R4 | `boot.scr`, `image.ub` trên FAT; `ls mmc 0:1` |
| Treo sau `Starting kernel ...` | R4–R5 | `earlycon`, `stdout-path`, cổng serial (B3) |
| `Unable to mount root fs` | R5 | Rootfs type EXT4, `root=/dev/mmcblk0p2`, `rootwait` (B3) |
| Không có dấu nhắc login | R6 | Cổng serial chọn ở B3 |
| Thiếu `/dev/modem_*` | R5/R7 | node DT, `compatible`, `dmesg`, gói `modem-regs` đã bật ở B7 |

### Lệnh PetaLinux hay dùng

| Lệnh | Việc |
|------|------|
| `petalinux-config` | cấu hình hệ thống (B3) |
| `petalinux-config -c u-boot / kernel / rootfs` | cấu hình từng thành phần (B4, B5, B7) |
| `petalinux-build -c <recipe>` | build lại một thành phần, vd. `-c kernel`, `-c device-tree`, `-c modem-regs` |
| `petalinux-build -x mrproper` | xóa sạch kết quả build (cẩn thận: build lại từ đầu mất 1–2 giờ) |
| `petalinux-package --boot ... / --wic` | đóng gói `BOOT.BIN` / ảnh thẻ SD (B8) |
| `petalinux-boot --qemu / --jtag` | chạy thử trên QEMU / nạp qua JTAG (B9) |
