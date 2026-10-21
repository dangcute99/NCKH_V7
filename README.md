# Linux nhúng trên SoC Zynq cho modem vệ tinh

Nghiên cứu, tùy biến hệ điều hành Linux trên nền tảng SoC Zynq-7000 ứng dụng cho modem vệ tinh: xây dựng phương pháp và quy trình triển khai Linux nhúng phù hợp với cấu hình phần cứng, gồm Bootloader, Linux Kernel, Device Tree, Root File System, trình điều khiển thiết bị, tạo Linux image, triển khai và kiểm thử. Kết quả là nền tảng để phát triển các chức năng phần mềm của modem vệ tinh.

Quy trình dùng công cụ **AMD PetaLinux 2023.2** và được viết **chung cho mọi board Zynq-7000**; board **E316** là ví dụ áp dụng.

Chi tiết từng bước từ build tới BootROM → ứng dụng: [docs/quy-trinh-xay-dung.md](docs/quy-trinh-xay-dung.md).

---

## 1. Mục tiêu

| # | Mục tiêu | Sản phẩm |
|---|----------|----------|
| 1 | Quy trình build tái lập được cho Zynq-7000 | Project PetaLinux + layer Yocto `meta-nckh` dùng chung |
| 2 | Bootloader: FSBL + U-Boot | `BOOT.BIN` |
| 3 | Linux Kernel tùy biến cho modem | `image.ub` + kernel config fragment |
| 4 | Device Tree sinh từ thiết kế phần cứng (`.xsa`) | `system.dtb` |
| 5 | Root File System tối giản | `rootfs.ext4` |
| 6 | Tích hợp trình điều khiển thiết bị (có sẵn + driver mẫu cho IP modem trên PL) | module `modem-regs.ko` |
| 7 | Linux image triển khai được | `petalinux-sdimage.wic` (thẻ SD), tùy chọn QSPI |
| 8 | Kiểm thử trên QEMU và trên board thật | `zynq-selftest` + báo cáo |

## 2. Phần cứng mục tiêu

### 2.1 Zynq-7000 (chung)

- PS: ARM Cortex-A9 (1 hoặc 2 nhân), DDR3, các ngoại vi cứng: UART, GEM Ethernet, SD/SDIO, QSPI, I2C, SPI, GPIO, USB, XADC.
- PL: FPGA, nơi đặt các khối IP xử lý tín hiệu của modem (điều chế/giải điều chế, FEC, DMA...). PS điều khiển các khối này qua AXI.

### 2.2 Board E316 (ví dụ)

> **Cần xác nhận:** chưa có tài liệu chính thức của board E316. Các giá trị dưới đây là **giả định** và cần đối chiếu với schematic/tài liệu của nhà sản xuất trước khi build.

| Thông số | Giá trị giả định | Ảnh hưởng tới |
|----------|------------------|---------------|
| SoC | XC7Z020 (Zynq-7000, 2× Cortex-A9) | thiết kế Vivado, FSBL |
| DDR | DDR3, 512 MB – 1 GB | cấu hình PS trong Vivado |
| Boot | thẻ SD, QSPI flash | boot mode, phân vùng QSPI |
| Console | UART (`ttyPS0`, 115200 8N1) | `petalinux-config` → serial |
| Mạng | GEM0 + PHY RGMII | Device Tree (`phy-handle`) |

Toàn bộ thông số phần cứng đi vào project qua **một file `.xsa`** xuất từ Vivado. Đổi board thì tạo project PetaLinux mới từ `.xsa` của board đó và dùng lại layer `meta-nckh`.

## 3. Kiến trúc

### 3.1 Trình tự khởi động

```
 BootROM ──► FSBL ──► (nạp bitstream PL) ──► U-Boot ──► boot.scr ──► Linux kernel ──► init (BusyBox) ──► ứng dụng modem
  (chip)    BOOT.BIN       system.bit         BOOT.BIN    (FAT)       image.ub (kernel + DTB)   rootfs.ext4
```

1. **BootROM** đọc chân boot mode và nạp `BOOT.BIN` từ SD hoặc QSPI.
2. **FSBL** khởi tạo clock, DDR và MIO (mã `ps7_init` sinh từ Vivado), nạp bitstream vào PL rồi chuyển quyền cho U-Boot.
3. **U-Boot** chạy *distro boot*, tìm `boot.scr` trên phân vùng FAT; script này nạp `image.ub` (ảnh FIT chứa kernel + DTB) rồi khởi động kernel.
4. **Kernel** mount rootfs ext4 ở phân vùng 2 của thẻ SD.

### 3.2 Công cụ

| Thành phần | Công cụ |
|------------|---------|
| Thiết kế phần cứng, `.xsa` | Vivado 2023.2 — **không cài trên máy build**; `.xsa` làm ở máy khác hoặc lấy từ nhà cung cấp board, rồi chép vào `hw/<board>/` |
| FSBL, Device Tree từ `.xsa` | PetaLinux (gọi XSCT + `device-tree-xlnx` bên trong) |
| Toolchain, U-Boot, Kernel, RootFS, driver, ứng dụng | PetaLinux (Yocto bên dưới) |
| `BOOT.BIN` | `petalinux-package --boot` (gọi bootgen) |
| Ảnh thẻ SD | `petalinux-package --wic` |
| Chạy thử không cần board | `petalinux-boot --qemu` (QEMU của Xilinx) |

**Vì sao PetaLinux:** công cụ chính thức của AMD cho Zynq, đi cùng phiên bản với Vivado; tự sinh FSBL, Device Tree, cấu hình U-Boot/kernel từ `.xsa`, nên giảm nhiều bước thủ công và khớp với tài liệu, BSP của AMD.

**Đánh đổi:** build nặng (Yocto: ~50–100 GB, lần đầu 1–2 giờ, cần mạng); nhiều bước bị ẩn sau `petalinux-*`. Để giữ tính minh bạch cho báo cáo, mỗi bước trong [docs/quy-trinh-xay-dung.md](docs/quy-trinh-xay-dung.md) ghi rõ PetaLinux làm gì bên dưới và file kết quả nằm ở đâu.

### 3.3 Tùy biến nằm ở đâu

| Loại tùy biến | Nơi đặt | Dùng chung giữa các board? |
|---------------|---------|---------------------------|
| Kernel config fragment, driver `modem-regs`, ứng dụng `zynq-selftest` | layer `meta-nckh/` (thêm vào project qua *User Layers*) | Có |
| Device Tree riêng board (`system-user.dtsi`) | `plnx/<board>/project-spec/meta-user/recipes-bsp/device-tree/files/` | Không |
| Cấu hình hệ thống (console, rootfs type, bootargs) | `plnx/<board>/project-spec/configs/config` (qua `petalinux-config`) | Không |
| Danh sách gói rootfs | `plnx/<board>/project-spec/configs/rootfs_config` (qua `petalinux-config -c rootfs`) | Không |

### 3.4 Phiên bản

| Thành phần | Phiên bản |
|------------|-----------|
| Vivado | **2023.2** |
| PetaLinux | **2023.2** (phải cùng phiên bản với Vivado) |
| Linux | `linux-xlnx` 6.1 LTS (đi kèm PetaLinux 2023.2) |
| U-Boot | `u-boot-xlnx` 2023.01 (đi kèm PetaLinux 2023.2) |
| Máy build | Ubuntu 22.04 LTS |

## 4. Cấu trúc project (dự kiến)

```
NCKH/
├── README.md
├── docs/quy-trinh-xay-dung.md     # quy trình chi tiết từng bước
├── tasks/                          # plan + task list
├── hw/
│   └── e316/system.xsa             # xuất từ Vivado (người dùng cung cấp)
├── meta-nckh/                      # layer Yocto dùng chung mọi board Zynq
│   ├── conf/layer.conf
│   ├── recipes-kernel/linux/
│   │   ├── linux-xlnx_%.bbappend
│   │   └── linux-xlnx/nckh.cfg     # kernel config fragment cho modem
│   ├── recipes-modules/modem-regs/ # driver mẫu cho IP thanh ghi modem
│   └── recipes-apps/zynq-selftest/ # script kiểm thử trên board
└── plnx/
    ├── zc702/                      # project từ BSP ZC702 — chỉ để thử QEMU (không commit)
    └── e316/                       # project PetaLinux cho E316
        ├── project-spec/           # cấu hình + meta-user (commit)
        ├── components/             # sinh tự động (không commit)
        ├── build/                  # Yocto tmp/sstate (không commit)
        └── images/linux/           # đầu ra: BOOT.BIN, image.ub, rootfs.ext4, *.wic
```

## 5. Quy trình triển khai (tóm tắt)

Bảng dưới chỉ ghi lệnh chính. Giá trị cấu hình cụ thể, PetaLinux làm gì bên dưới, cách kiểm tra và lỗi thường gặp của từng bước chỉ nằm ở **một nơi**: [docs/quy-trinh-xay-dung.md](docs/quy-trinh-xay-dung.md), mục B0–B9.

| Bước | Việc | Lệnh chính | Chi tiết |
|------|------|------------|----------|
| 0 | Cài PetaLinux 2023.2 | `./petalinux-v2023.2-*-installer.run -d ~/petalinux/2023.2`, rồi `source ~/petalinux/2023.2/settings.sh` | B0 |
| 1 | Thiết kế phần cứng | Vivado → Export Hardware (kèm bitstream) → `hw/e316/system.xsa` | B1 |
| 2 | Tạo project, nhập `.xsa` | trong `plnx/`: `petalinux-create -t project --template zynq -n e316`; trong `plnx/e316/`: `petalinux-config --get-hw-description=../../hw/e316/system.xsa` | B2 |
| 3 | Cấu hình hệ thống | `petalinux-config`: console, rootfs EXT4, bootargs, user layer `meta-nckh` | B3 |
| 4 | U-Boot | sinh tự động từ `.xsa`; chỉnh nếu cần: `petalinux-config -c u-boot` | B4 |
| 5 | Kernel | fragment `meta-nckh/recipes-kernel/linux/linux-xlnx/nckh.cfg` (§5.1); chỉnh tương tác: `petalinux-config -c kernel` | B5 |
| 6 | Device Tree | chỉ sửa `project-spec/meta-user/recipes-bsp/device-tree/files/system-user.dtsi` (§5.2) | B6 |
| 7 | Rootfs, driver, ứng dụng | `petalinux-config -c rootfs` | B7 |
| 8 | Build và đóng gói | `petalinux-build`; `petalinux-package --boot --fsbl images/linux/zynq_fsbl.elf --fpga images/linux/system.bit --u-boot --force`; `petalinux-package --wic` | B8 |
| 9 | Triển khai | ghi `petalinux-sdimage.wic` ra thẻ SD; hoặc `petalinux-boot --jtag` / `petalinux-boot --qemu --kernel` | B9 |

### 5.1 Kernel fragment cho modem (`nckh.cfg`)

| Nhóm | Tùy chọn | Mục đích |
|------|----------|----------|
| Truy cập IP trên PL | `UIO`, `UIO_PDRV_GENIRQ`, `UIO_DMEM_GENIRQ` | điều khiển IP từ user space, nhận ngắt |
| DMA | `XILINX_DMA` | truyền mẫu tín hiệu giữa PL và DDR qua AXI DMA |
| FPGA | `FPGA_MGR_ZYNQ_FPGA`, `OF_OVERLAY`, `OF_CONFIGFS` | nạp lại bitstream và DT overlay khi đang chạy |
| Ngoại vi | `SPI_SPIDEV`, `I2C_CHARDEV`, `MTD_SPI_NOR` | cấu hình front-end RF/synthesizer, truy cập QSPI |
| Giám sát | `XILINX_XADC`, `IIO` | nhiệt độ, điện áp; nền cho driver RF dùng IIO (vd. AD9361) |
| Thời gian thực | `PREEMPT`, `HIGH_RES_TIMERS` | giảm độ trễ cho vòng điều khiển modem |

### 5.2 Device Tree cho IP modem

PetaLinux sinh `system-top.dts`, `pcw.dtsi`, `pl.dtsi` trong `components/` — **không sửa tay**. Ví dụ gán driver cho IP modem trong `system-user.dtsi`:

```dts
&modem_ctrl_0 {                       /* label do pl.dtsi sinh ra theo tên IP trong Vivado */
    compatible = "nckh,modem-regs";   /* dùng driver modem-regs */
};
/* hoặc: compatible = "generic-uio";  để truy cập trực tiếp từ user space qua /dev/uioN */
```

### 5.3 Trình điều khiển thiết bị

**Driver có sẵn trong kernel**, chỉ cần bật và khai báo trong DT:

| Ngoại vi | Driver | Thiết bị trong Linux |
|----------|--------|----------------------|
| UART | `xuartps` | `/dev/ttyPS0` |
| Ethernet GEM | `macb` | `eth0` |
| SD | `sdhci-of-arasan` | `/dev/mmcblk0` |
| QSPI | `spi-zynq-qspi` + `spi-nor` | `/dev/mtdN` |
| I2C / SPI | `i2c-cadence` / `spi-cadence` | `/dev/i2c-N`, `/dev/spidevX.Y` |
| GPIO | `gpio-zynq` | `/dev/gpiochipN` |
| PL (FPGA) | `zynq-fpga` | `/sys/class/fpga_manager/fpga0` |
| AXI DMA | `xilinx_dma` | dmaengine |

**Driver mẫu `modem-regs`** (recipe `meta-nckh/recipes-modules/modem-regs`, dựng từ mẫu `petalinux-create -t modules`):
- platform driver, khớp `compatible = "nckh,modem-regs"`;
- ánh xạ vùng thanh ghi của IP, tạo `/dev/modem_<địa_chỉ>`;
- đọc/ghi theo word 32-bit căn lề, offset trong file = offset thanh ghi;
- khung để phát triển tiếp: ngắt, DMA, sysfs, ioctl.

| Hướng tích hợp IP modem | Khi nào dùng |
|-------------------------|--------------|
| `generic-uio` | thử nghiệm nhanh, logic điều khiển nằm hoàn toàn ở user space |
| driver kernel (`modem-regs`) | cần xử lý ngắt/DMA trong kernel, kiểm soát truy cập, API ổn định cho ứng dụng |

## 6. Kiểm thử

### 6.1 Các mức kiểm thử

| Mức | Môi trường | Nội dung |
|-----|------------|----------|
| Build | máy build | `petalinux-build` sạch từ đầu thành công; đủ các file trong `images/linux/` |
| Boot | QEMU | kernel boot tới login prompt, mount được rootfs |
| Chức năng | board E316 | `zynq-selftest` đạt hết các mục |
| Hiệu năng | board E316 | thời gian boot, thông lượng Ethernet, băng thông DDR |
| Ổn định | board E316 | chạy liên tục 24 giờ, khởi động lại 100 lần |

### 6.2 `zynq-selftest` (chạy trên board)

| Mục kiểm tra | Tiêu chí đạt |
|--------------|--------------|
| CPU | số nhân đúng với chip (XC7Z020: 2) |
| Bộ nhớ | `MemTotal` khớp với dung lượng DDR; `memtester` không lỗi (chế độ `--full`) |
| Kernel log | không có `Oops`, `BUG:`, `Kernel panic`, `Call trace` |
| RootFS | `/` được mount `rw` |
| UART | có `/dev/ttyPS0` |
| SD | có `/dev/mmcblk0` |
| Ethernet | có `eth0`, link up; ping được tới host (nếu có khai báo `PING_HOST`) |
| QSPI | `/proc/mtd` liệt kê đủ các phân vùng |
| PL | `fpga_manager` ở trạng thái `operating` |
| Driver modem | `/dev/modem_*` tồn tại; đọc thanh ghi version ra đúng giá trị mong đợi |

Kết quả in ra dạng `PASS` / `WARN` / `FAIL`; mã thoát khác 0 nếu có mục `FAIL`.

### 6.3 Chỉ số đo đạc

| Chỉ số | Cách đo | Mục tiêu tham khảo |
|--------|---------|--------------------|
| Thời gian boot (bật nguồn → login) | đánh dấu thời gian trên log UART | < 10 s |
| Thông lượng Ethernet | `iperf3 -c <host>` | > 500 Mbit/s |
| Dung lượng rootfs | `du` trên ảnh rootfs | < 64 MB |
| Bộ nhớ trống sau boot | `free` | > 80% DDR |

## 7. Áp dụng cho board Zynq-7000 khác

1. Xuất `.xsa` của board mới vào `hw/<board>/system.xsa`.
2. `petalinux-create -t project --template zynq -n <board>` trong `plnx/`, rồi `petalinux-config --get-hw-description=...`.
3. Trong `petalinux-config`: chọn console, rootfs EXT4, thêm user layer `meta-nckh`.
4. Viết `system-user.dtsi` cho board (PHY, flash, IP).
5. `petalinux-build` → `petalinux-package --boot` → `petalinux-package --wic`.

Layer `meta-nckh` (kernel fragment, driver, ứng dụng kiểm thử) dùng lại nguyên vẹn.

## 8. Rủi ro và việc cần làm rõ

| Vấn đề | Ảnh hưởng | Cách xử lý |
|--------|-----------|------------|
| Chưa có tài liệu E316 (chip, DDR, UART console, PHY) | sai cấu hình PS, không boot được | lấy schematic/tài liệu từ nhà sản xuất; cấu hình PS theo đó |
| Vivado và PetaLinux phải cùng phiên bản | `.xsa` lệch phiên bản → lỗi khi `--get-hw-description` | dùng 2023.2 cho cả hai |
| Ổ đĩa: PetaLinux (~10 GB) + mỗi project build (~40–50 GB) + downloads/sstate (~10–20 GB) | máy build còn ~80 GB, không đủ cho 2 project cùng lúc | không cài Vivado trên máy build; bật `rm_work`; chỉ giữ một project tại một thời điểm (xóa `plnx/zc702/` trước khi tạo `plnx/e316/`) |
| Không có Vivado trên máy build | không tự sửa được thiết kế phần cứng, không có `hw_server` cho JTAG | mỗi lần đổi thiết kế (vd. thêm IP modem) phải làm `.xsa` ở máy có Vivado; JTAG dùng bản cài riêng *Hardware Server* (nhỏ) nếu cần |
| Lần build đầu cần mạng và lâu (1–2 giờ) | trễ lịch | chạy nền qua đêm; giữ `build/` và thư mục downloads giữa các lần build |
| AMD đã thay PetaLinux bằng EDF (Yocto) từ 2025.1; bản PetaLinux cuối dự kiến cuối 2026 | quy trình không còn được phát triển tiếp ở các bản mới | dùng 2023.2 (khớp Vivado 2023.2); tùy biến để trong layer Yocto `meta-nckh` nên chuyển sang EDF/Yocto được |
| Tên/label IP modem trong `pl.dtsi` phụ thuộc thiết kế Vivado | `system-user.dtsi` tham chiếu sai label thì build DT lỗi | giữ phần IP dạng mẫu có chú thích, chỉ bật khi IP đã có trong thiết kế |
| QEMU không mô phỏng PL | không kiểm thử được IP modem trên QEMU | kiểm thử IP trên board thật |
| Front-end RF (vd. AD9361) cần driver và `libiio` riêng | ngoài phạm vi giai đoạn này | bổ sung ở giai đoạn phát triển chức năng modem |

## 9. Kế hoạch thực hiện

Xem [tasks/plan.md](tasks/plan.md) (lịch 1 tuần, 21/10 – 28/10/2026) và [tasks/todo.md](tasks/todo.md).
