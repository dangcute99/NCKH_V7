# Linux nhúng trên SoC Zynq cho modem vệ tinh

Nghiên cứu, tùy biến hệ điều hành Linux trên nền tảng SoC Zynq-7000 ứng dụng cho modem vệ tinh: xây dựng phương pháp và quy trình triển khai Linux nhúng phù hợp với cấu hình phần cứng, gồm Bootloader, Linux Kernel, Device Tree, Root File System, trình điều khiển thiết bị, tạo Linux image, triển khai và kiểm thử. Kết quả là nền tảng để phát triển các chức năng phần mềm của modem vệ tinh.

Quy trình được viết **chung cho mọi board Zynq-7000**; board **E316** là ví dụ áp dụng.

---

## 1. Mục tiêu

| # | Mục tiêu | Sản phẩm |
|---|----------|----------|
| 1 | Quy trình build tái lập được cho Zynq-7000 | `Makefile` + cây `br2-external` |
| 2 | Bootloader: FSBL + U-Boot | `BOOT.BIN` |
| 3 | Linux Kernel tùy biến cho modem | `uImage` + kernel config fragment |
| 4 | Device Tree sinh từ thiết kế phần cứng (`.xsa`) | `system.dtb` |
| 5 | Root File System tối giản | `rootfs.ext4` |
| 6 | Tích hợp trình điều khiển thiết bị (có sẵn + driver mẫu cho IP modem trên PL) | module `modem-regs.ko` |
| 7 | Linux image triển khai được | `sdcard.img` (thẻ SD), tùy chọn QSPI |
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
| Console | UART (`ttyPS0`, 115200 8N1) | bootargs, `stdout-path` |
| Mạng | GEM0 + PHY RGMII | Device Tree (`phy-handle`) |

Toàn bộ thông số phần cứng đi vào project qua **một file `.xsa`** xuất từ Vivado. Đổi board thì chỉ đổi file `.xsa` và thư mục cấu hình của board, không sửa quy trình.

## 3. Kiến trúc

### 3.1 Trình tự khởi động

```
 BootROM ──► FSBL ──► (nạp bitstream PL) ──► U-Boot ──► Linux kernel ──► init (BusyBox) ──► ứng dụng modem
  (chip)    BOOT.BIN       system.bit         BOOT.BIN    uImage + system.dtb    rootfs.ext4
```

1. **BootROM** đọc chân boot mode và nạp `BOOT.BIN` từ SD hoặc QSPI.
2. **FSBL** khởi tạo clock, DDR và MIO (mã `ps7_init` sinh từ Vivado), nạp bitstream vào PL rồi chuyển quyền cho U-Boot.
3. **U-Boot** dùng DTB mà FSBL đặt ở địa chỉ `0x100000` (cơ chế `CONFIG_OF_BOARD` của U-Boot Xilinx), đọc `extlinux/extlinux.conf` trên phân vùng FAT rồi nạp kernel.
4. **Kernel** mount rootfs ext4 ở phân vùng 2 của thẻ SD.

U-Boot và kernel dùng **chung một file DTB**, nên hai bên luôn thấy cùng một mô tả phần cứng.

### 3.2 Phân chia công cụ

| Thành phần | Công cụ | Lý do |
|------------|---------|-------|
| FSBL, Device Tree | XSCT (Vivado/Vitis) + `device-tree-xlnx` | chỉ công cụ của Xilinx đọc được `.xsa` |
| Toolchain, U-Boot, Kernel, RootFS, driver, image | Buildroot (`BR2_EXTERNAL`) | build tái lập, gọn, minh bạch từng bước |
| `BOOT.BIN` | `bootgen` (gói `host-bootgen` của Buildroot) | không phụ thuộc bản cài Vitis |
| Ảnh thẻ SD | `genimage` | tạo ảnh phân vùng mà không cần quyền root |

Chọn Buildroot thay cho PetaLinux/Yocto vì từng bước build đều thấy rõ (phù hợp báo cáo nghiên cứu), rootfs nhỏ, thời gian build ngắn và quy trình không gắn với một phiên bản PetaLinux cụ thể.

### 3.3 Phiên bản (đã kiểm tra các tag tồn tại)

| Thành phần | Phiên bản |
|------------|-----------|
| Buildroot | `2024.02.13` (nhánh LTS) |
| U-Boot | `u-boot-xlnx` – `xlnx_rebase_v2023.01_2023.2` |
| Linux | `linux-xlnx` – `xlnx_rebase_v6.1_LTS_2023.2` (kernel 6.1 LTS) |
| Device Tree generator | `device-tree-xlnx` – `xilinx_v2023.2` |
| bootgen | `xilinx_v2023.2` |
| Vivado / XSCT | **2023.2** (phải cùng phiên bản với `device-tree-xlnx`) |

Bộ U-Boot/Linux trên trùng với cấu hình tham chiếu `zynq_zed_defconfig` của Buildroot 2024.02, tức là đã được kiểm chứng chạy cùng nhau.

## 4. Cấu trúc project (dự kiến)

```
NCKH/
├── README.md                     # tài liệu này
├── Makefile                      # điểm vào duy nhất: make BOARD=e316 ...
├── hw/
│   └── e316/system.xsa           # xuất từ Vivado (người dùng cung cấp)
├── scripts/
│   └── xsa-import.tcl            # XSCT: sinh FSBL + Device Tree từ .xsa
├── br2-external/                 # cây BR2_EXTERNAL của Buildroot
│   ├── external.desc / external.mk / Config.in
│   ├── configs/
│   │   └── e316_defconfig        # cấu hình Buildroot cho E316
│   ├── board/
│   │   ├── zynq-common/          # dùng chung cho mọi board Zynq-7000
│   │   │   ├── post-image.sh     # dtc → bootgen → genimage
│   │   │   ├── linux.fragment    # kernel config chung (UIO, DMA, FPGA mgr...)
│   │   │   └── rootfs-overlay/   # zynq-selftest, script khởi động
│   │   └── e316/                 # riêng cho board E316
│   │       ├── system-user.dtsi  # bổ sung/ghi đè Device Tree
│   │       ├── linux.fragment    # kernel config riêng của board
│   │       ├── extlinux.conf     # dòng lệnh kernel
│   │       └── genimage.cfg      # bố cục thẻ SD
│   └── package/
│       └── modem-regs/           # driver mẫu cho IP thanh ghi modem trên PL
└── build/<board>/                # đầu ra (không đưa vào git)
    ├── hw/                       # fsbl.elf, system.bit, dts/
    └── images/                   # BOOT.BIN, uImage, system.dtb, rootfs.ext4, sdcard.img
```

## 5. Quy trình triển khai

### Bước 0 – Chuẩn bị máy build

- Ubuntu 22.04/24.04, khoảng 30 GB trống.
- Các gói theo yêu cầu của Buildroot: `build-essential git wget cpio unzip rsync bc file patch perl python3`.
- Vivado/Vitis 2023.2, để có `xsct`.
- Tùy chọn: `qemu-system-arm` để chạy thử khi chưa có board.

### Bước 1 – Thiết kế phần cứng (Vivado)

1. Tạo block design gồm `ZYNQ7 Processing System`, cấu hình DDR, MIO (UART, SD, QSPI, ENET, I2C...) và clock theo schematic của E316.
2. Thêm các IP modem vào PL và nối với PS qua `M_AXI_GP0`, kèm ngắt `IRQ_F2P` nếu cần.
3. Chạy `Generate Bitstream`, sau đó `File → Export → Export Hardware` (chọn *Include bitstream*), rồi lưu file ra `hw/e316/system.xsa`.

### Bước 2 – Sinh FSBL và Device Tree từ `.xsa`

```sh
source /tools/Xilinx/Vitis/2023.2/settings64.sh
make BOARD=e316 hw
```

`scripts/xsa-import.tcl` mở file `.xsa` rồi:
- sinh và biên dịch ứng dụng `zynq_fsbl`, cho ra `build/e316/hw/fsbl.elf`;
- tạo Device Tree bằng `device-tree-xlnx`, cho ra `system-top.dts`, `zynq-7000.dtsi`, `pcw.dtsi` (ngoại vi PS) và `pl.dtsi` (IP trên PL);
- tách `system.bit` ra khỏi `.xsa`.

### Bước 3 – Bootloader (U-Boot)

- Dùng defconfig `xilinx_zynq_virt` của U-Boot Xilinx, xuất ra định dạng ELF (`u-boot.elf`) để `bootgen` đóng gói.
- U-Boot không nhúng DTB của riêng board: nó đọc DTB do FSBL nạp vào `0x100000`.
- Khởi động theo chuẩn *distro boot*: U-Boot tự tìm `extlinux/extlinux.conf` trên phân vùng FAT.

```sh
make BOARD=e316 uboot-rebuild      # build lại riêng U-Boot
```

### Bước 4 – Linux Kernel

- Cấu hình gốc là `xilinx_zynq_defconfig`, sau đó trộn thêm các fragment (`zynq-common/linux.fragment` rồi `e316/linux.fragment`).
- Các tùy chọn bổ sung cho modem:

| Nhóm | Tùy chọn | Mục đích |
|------|----------|----------|
| Truy cập IP trên PL | `UIO`, `UIO_PDRV_GENIRQ`, `UIO_DMEM_GENIRQ` | điều khiển IP từ user space, nhận ngắt |
| DMA | `XILINX_DMA` | truyền mẫu tín hiệu giữa PL và DDR qua AXI DMA |
| FPGA | `FPGA_MGR_ZYNQ_FPGA`, `OF_OVERLAY`, `OF_CONFIGFS` | nạp lại bitstream và DT overlay khi đang chạy |
| Ngoại vi | `SPI_SPIDEV`, `I2C_CHARDEV`, `MTD_SPI_NOR` | cấu hình front-end RF/synthesizer, truy cập QSPI |
| Giám sát | `XILINX_XADC`, `IIO` | nhiệt độ, điện áp; nền cho driver RF dùng IIO (vd. AD9361) |
| Thời gian thực | `PREEMPT`, `HIGH_RES_TIMERS` | giảm độ trễ cho vòng điều khiển modem |

```sh
make BOARD=e316 linux-menuconfig   # chỉnh cấu hình kernel
make BOARD=e316 linux-rebuild
```

### Bước 5 – Device Tree

DTB cuối cùng được ghép từ ba lớp rồi biên dịch bằng `cpp` + `dtc -@` (có symbol để dùng overlay):

```
system-top.dts  (sinh tự động từ .xsa – không sửa tay)
  └── pcw.dtsi, pl.dtsi, zynq-7000.dtsi
system-user.dtsi (viết tay, theo board) – PHY Ethernet, flash QSPI + phân vùng, gán driver cho IP modem
```

Ví dụ gán driver cho IP modem trong `system-user.dtsi`:

```dts
&modem_ctrl_0 {                       /* label do pl.dtsi sinh ra theo tên IP trong Vivado */
    compatible = "nckh,modem-regs";   /* dùng driver modem-regs */
};
/* hoặc: compatible = "generic-uio";  để truy cập trực tiếp từ user space qua /dev/uioN */
```

Nguyên tắc: **không sửa các file sinh tự động**; mọi tùy biến đặt trong `system-user.dtsi`, nhờ vậy xuất lại `.xsa` không làm mất phần chỉnh tay.

### Bước 6 – Root File System

- Toolchain: Arm GNU Toolchain dựng sẵn (ARMv7-A, hard-float, NEON).
- Init: BusyBox, `mdev` tự nạp module theo modalias.
- Hệ thống tệp: ext4.
- Gói cài sẵn:

| Gói | Dùng để |
|-----|---------|
| `dropbear` | SSH vào board |
| `ethtool`, `iperf3` | kiểm tra liên kết và thông lượng Ethernet |
| `i2c-tools`, `spi-tools` | kiểm tra bus I2C/SPI tới front-end RF |
| `mtd-utils` | ghi/xóa QSPI flash |
| `memtester` | kiểm tra DDR |
| `modem-regs` | driver mẫu cho IP modem |

```sh
make BOARD=e316 menuconfig         # chỉnh cấu hình Buildroot / thêm gói
```

### Bước 7 – Trình điều khiển thiết bị

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

**Driver mẫu `modem-regs`** (out-of-tree, đóng gói thành package của Buildroot) dùng cho khối thanh ghi AXI-Lite của modem trên PL:
- là platform driver, khớp với `compatible = "nckh,modem-regs"` trong DT;
- ánh xạ vùng thanh ghi của IP và tạo `/dev/modem_<địa_chỉ>`;
- đọc/ghi theo từng word 32-bit căn lề, offset trong file chính là offset thanh ghi;
- là khung để phát triển tiếp driver modem: ngắt, DMA, sysfs, ioctl.

Hai hướng tích hợp IP modem:

| Hướng | Khi nào dùng |
|-------|--------------|
| `generic-uio` | thử nghiệm nhanh, logic điều khiển nằm hoàn toàn ở user space |
| driver kernel (`modem-regs`) | cần xử lý ngắt/DMA trong kernel, kiểm soát truy cập, API ổn định cho ứng dụng |

### Bước 8 – Tạo Linux image

```sh
make BOARD=e316                    # build toàn bộ
```

Script `post-image.sh` chạy theo thứ tự:
1. biên dịch `system.dtb` từ `system-top.dts` + `system-user.dtsi`;
2. tạo `boot.bif` rồi chạy `bootgen` để ra `BOOT.BIN`:
   ```
   the_ROM_image:
   {
     [bootloader] fsbl.elf
     system.bit
     u-boot.elf
     [load=0x100000] system.dtb
   }
   ```
3. chạy `genimage` để ra `sdcard.img`:

| Phân vùng | Kiểu | Nội dung |
|-----------|------|----------|
| p1 `boot` | FAT32, 32 MB | `BOOT.BIN`, `uImage`, `system.dtb`, `extlinux/extlinux.conf` |
| p2 `rootfs` | ext4 | root file system |

### Bước 9 – Triển khai lên board

**Thẻ SD:**
```sh
make BOARD=e316 sdcard DEV=/dev/sdX   # ghi sdcard.img, sẽ hỏi xác nhận vì xóa toàn bộ thẻ
```
Chỉnh jumper boot mode của E316 sang **SD**, cắm UART (115200 8N1) rồi cấp nguồn.

**QSPI** (tùy chọn, sau khi đã boot được từ SD):
```sh
flashcp BOOT.BIN /dev/mtd0            # chạy trên board
```
Chỉnh boot mode sang **QSPI**. Rootfs vẫn có thể để trên SD hoặc chuyển sang eMMC/QSPI tùy dung lượng.

**Chạy thử không cần board (QEMU):**
```sh
make BOARD=e316 qemu                  # qemu-system-arm -M xilinx-zynq-a9, boot từ sdcard.img
```
QEMU giả lập phần PS (UART, GEM, SD) nhưng không giả lập PL. Vì vậy nó dùng để kiểm tra kernel và rootfs, chưa kiểm tra được IP modem.

## 6. Kiểm thử

### 6.1 Các mức kiểm thử

| Mức | Môi trường | Nội dung |
|-----|------------|----------|
| Build | máy build | build sạch từ đầu thành công; đủ các file trong `images/` |
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
2. Sao chép `br2-external/board/e316` thành `board/<board>`, rồi sửa `system-user.dtsi` (PHY, flash, IP) và `extlinux.conf` (console, rootfs).
3. Sao chép `configs/e316_defconfig` thành `configs/<board>_defconfig`, rồi đổi đường dẫn thư mục board.
4. Chạy `make BOARD=<board> hw && make BOARD=<board>`.

Phần dùng chung (`zynq-common`, `Makefile`, `xsa-import.tcl`, driver) giữ nguyên.

## 8. Rủi ro và việc cần làm rõ

| Vấn đề | Ảnh hưởng | Cách xử lý |
|--------|-----------|------------|
| Chưa có tài liệu E316 (chip, DDR, PHY, UART console) | sai cấu hình PS, không boot được | lấy schematic/tài liệu từ nhà sản xuất; cấu hình PS theo đó |
| Phiên bản Vivado khác 2023.2 | `device-tree-xlnx` không khớp, lỗi khi sinh DT | đổi `XILINX_VER` cho tất cả thành phần cùng lúc |
| Tên/label IP modem trong `pl.dtsi` phụ thuộc thiết kế Vivado | `system-user.dtsi` tham chiếu sai label thì `dtc` báo lỗi | giữ phần IP dưới dạng mẫu có chú thích, chỉ bật khi IP đã có trong thiết kế |
| QEMU không giả lập PL | không kiểm thử được IP modem trên QEMU | kiểm thử IP trên board thật |
| Front-end RF (vd. AD9361) cần driver và `libiio` riêng | chưa nằm trong phạm vi giai đoạn này | bổ sung ở giai đoạn phát triển chức năng modem |

## 9. Kế hoạch thực hiện

| Giai đoạn | Nội dung | Kết quả |
|-----------|----------|---------|
| 1 | Dựng `Makefile`, `br2-external`, `xsa-import.tcl` | build được kernel + rootfs, boot trên QEMU |
| 2 | Thiết kế Vivado cho E316 (chỉ PS) | `BOOT.BIN` boot được tới Linux trên board |
| 3 | Hoàn thiện Device Tree và driver ngoại vi PS | `zynq-selftest` đạt các mục về PS |
| 4 | Thêm IP thanh ghi modem vào PL, tích hợp `modem-regs` | đọc/ghi được thanh ghi IP từ Linux |
| 5 | Triển khai QSPI, đo hiệu năng, chạy thử ổn định | báo cáo kiểm thử |
