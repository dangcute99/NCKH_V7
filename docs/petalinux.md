# PetaLinux là gì và hoạt động thế nào

Tài liệu giải thích PetaLinux cho người mới: nó làm gì, đứng ở đâu giữa Vivado và Linux, sinh ra file gì, và khi sửa một thứ thì phải build lại cái gì. Viết cho **PetaLinux 2021.1** trên Zynq-7000 (board E316). Các bước build cụ thể xem [quy-trinh-xay-dung.md](quy-trinh-xay-dung.md); lộ trình xem [lo-trinh.md](lo-trinh.md).

## 1. Một câu

**PetaLinux là bộ công cụ của AMD/Xilinx để biến một file mô tả phần cứng (`.xsa`) thành một hệ Linux chạy được trên chip Zynq**, gồm bootloader, kernel, Device Tree và hệ thống file.

Bên dưới, PetaLinux là **Yocto** (bộ công cụ build Linux nhúng chuẩn công nghiệp) được bọc thêm các lệnh `petalinux-*` và các script đọc `.xsa`. Bạn có thể dùng PetaLinux mà không hiểu sâu Yocto, nhưng khi cần tùy biến (thêm driver, ứng dụng, cấu hình kernel) thì sẽ chạm vào khái niệm của Yocto: *layer*, *recipe*, *bbappend*.

## 2. PetaLinux đứng ở đâu

```
 ┌──────────────┐   .xsa    ┌─────────────────────────────┐   BOOT.BIN, image.ub,   ┌──────────────┐
 │   Vivado     │ ────────► │         PetaLinux           │   boot.scr, rootfs ───► │  Thẻ SD /    │
 │ (thiết kế    │           │  lệnh petalinux-*           │                         │  board E316  │
 │  phần cứng)  │           │   └─ Yocto / bitbake        │                         └──────────────┘
 └──────────────┘           │       └─ gcc, kernel, ...   │
                            └─────────────────────────────┘
```

| Lớp | Công cụ | Bạn làm gì ở lớp này | Kết quả |
|-----|---------|----------------------|---------|
| Phần cứng | Vivado | Cấu hình PS (DDR, UART, SD, Ethernet), thêm IP trên PL, tạo bitstream | `system.xsa` |
| Linux | PetaLinux | Tạo project từ `.xsa`, chọn cấu hình, thêm driver/app, build | file boot + rootfs |
| Ứng dụng | C, shell trên board | Viết chương trình chạy trên Linux | chương trình modem |

**Điểm mấu chốt:** `.xsa` là cầu nối duy nhất từ Vivado sang PetaLinux. Mọi thông số phần cứng (DDR, chân UART, địa chỉ IP) đi vào Linux qua file này. Vivado và PetaLinux **phải cùng phiên bản** (ở đây cả hai là 2021.1).

## 3. PetaLinux tạo ra những gì

Một hệ Linux trên Zynq cần 5 thành phần. PetaLinux sinh ra cả 5 từ `.xsa`:

| Thành phần | Nó là gì | PetaLinux sinh ra thế nào | File |
|------------|----------|---------------------------|------|
| **FSBL** (First Stage Boot Loader) | Chương trình chạy đầu tiên: bật clock, khởi tạo DDR, nạp bitstream vào PL | Biên dịch mã `ps7_init` lấy từ `.xsa` | `zynq_fsbl.elf` |
| **Bitstream** | Cấu hình cho phần FPGA (PL) | Tách thẳng từ `.xsa` | `system.bit` |
| **U-Boot** | Bootloader thứ hai: đọc thẻ SD, nạp kernel | Build `u-boot-xlnx` 2021.01 | `u-boot.elf`, `boot.scr` |
| **Linux kernel + Device Tree** | Nhân hệ điều hành + bản mô tả phần cứng cho kernel | Build `linux-xlnx` 5.10; sinh DT từ `.xsa` | `image.ub` (chứa kernel + `system.dtb`) |
| **Root filesystem** | Toàn bộ thư mục `/`: shell, thư viện, ứng dụng | Yocto build từng gói (BusyBox, glibc, ...) | `rootfs.ext4` hoặc nằm trong `image.ub` |

Sau đó PetaLinux **đóng gói**:

- `BOOT.BIN` = FSBL + bitstream + U-Boot (+ DTB), gói bằng công cụ `bootgen`. BootROM trong chip đọc file này đầu tiên.
- `image.ub` = ảnh FIT chứa kernel + DTB (và rootfs nếu dùng INITRAMFS).
- `petalinux-sdimage.wic` = ảnh nguyên thẻ SD (phân vùng FAT + phân vùng ext4), ghi bằng `dd`.

Tất cả nằm trong `images/linux/` của project.

## 4. Chuỗi khởi động trên board

```
Bật nguồn → BootROM (trong chip) → đọc BOOT.BIN trên thẻ SD
          → FSBL: khởi tạo DDR, nạp system.bit vào PL
          → U-Boot: chạy boot.scr, nạp image.ub vào RAM
          → Kernel: đọc Device Tree, nạp driver, mount rootfs
          → init → dấu nhắc login trên UART (ttyPS0, 115200)
```

Hiểu chuỗi này giúp khoanh vùng lỗi: log UART dừng ở đâu thì lỗi ở bước đó. Bảng tra lỗi đầy đủ ở [quy-trinh-xay-dung.md](quy-trinh-xay-dung.md) (mục R1–R7 và Phụ lục).

## 5. Cấu trúc một project PetaLinux

Sau `petalinux-create -t project --template zynq -n e316`:

```
plnx/e316/
├── project-spec/              ← NƠI BẠN SỬA (commit vào git)
│   ├── hw-description/        bản sao .xsa đã import
│   ├── configs/
│   │   ├── config             cấu hình hệ thống (console, rootfs type, bootargs, layer)
│   │   └── rootfs_config      danh sách gói trong rootfs
│   └── meta-user/             layer riêng của project
│       ├── conf/petalinuxbsp.conf     biến Yocto thêm vào (vd. INHERIT += "rm_work")
│       ├── conf/user-rootfsconfig     khai báo thêm gói cho menu rootfs
│       └── recipes-bsp/device-tree/files/system-user.dtsi   Device Tree viết tay
├── components/                ← SINH TỰ ĐỘNG, không sửa (DT sinh từ .xsa nằm ở đây)
├── build/                     ← Yocto làm việc ở đây (40–50 GB), không commit
└── images/linux/              ← KẾT QUẢ: BOOT.BIN, image.ub, boot.scr, rootfs, .wic
```

**Quy tắc vàng:** chỉ sửa trong `project-spec/` (và layer dùng chung `meta-modem/`). File trong `components/` và `build/` bị sinh lại mỗi lần build, sửa ở đó sẽ mất.

## 6. Các lệnh chính

Trước khi dùng, mỗi terminal phải nạp môi trường:

```sh
source /opt/pkg/petalinux/2021.1/settings.sh /opt/pkg/petalinux/2021.1
stty rows 40 cols 120        # tránh menu cấu hình bị vỡ khung
```

| Lệnh | Làm gì | Bên dưới thực chất là |
|------|--------|------------------------|
| `petalinux-create -t project --template zynq -n e316` | Tạo khung project rỗng | Chép mẫu project |
| `petalinux-config --get-hw-description=<đường dẫn .xsa>` | Nạp phần cứng, mở menu cấu hình hệ thống | Chạy XSCT đọc `.xsa`, ghi `configs/config` |
| `petalinux-config` | Menu hệ thống: console, rootfs type, bootargs, user layers | Sửa `configs/config` |
| `petalinux-config -c kernel` | Menu cấu hình kernel (giống `make menuconfig`) | Lưu thành fragment trong `meta-user` |
| `petalinux-config -c rootfs` | Chọn gói đưa vào rootfs | Sửa `configs/rootfs_config` |
| `petalinux-build` | Build toàn bộ (lần đầu 1–2 giờ, cần mạng) | `bitbake` build FSBL, U-Boot, DT, kernel, rootfs |
| `petalinux-build -c <tên>` | Build lại một thành phần (`kernel`, `device-tree`, `modem-regs`, ...) | `bitbake <recipe>` |
| `petalinux-package --boot --fsbl … --fpga … --u-boot --force` | Gói `BOOT.BIN` | Sinh file `.bif` rồi gọi `bootgen` |
| `petalinux-package --wic` | Tạo ảnh thẻ SD `.wic` | Công cụ `wic` của Yocto |
| `petalinux-boot --jtag` / `--qemu` | Nạp qua cáp JTAG / chạy giả lập | `xsdb` / QEMU của Xilinx |

## 7. Sửa gì thì build lại gì

Đây là bảng dùng nhiều nhất khi làm việc hằng ngày (rút từ kinh nghiệm Lab 8):

| Bạn vừa sửa | Build lại | Chép lên thẻ |
|-------------|-----------|--------------|
| Thiết kế Vivado (`.xsa` mới, đổi bitstream) | `petalinux-config --get-hw-description=…` → `petalinux-build` → `petalinux-package --boot … --force` | `BOOT.BIN` (+ `image.ub` vì DT đổi theo) |
| Device Tree (`system-user.dtsi`) | `petalinux-build -c device-tree` rồi `petalinux-build` | `image.ub` |
| Cấu hình kernel / kernel fragment | `petalinux-build -c kernel` rồi `petalinux-build` | `image.ub` |
| Gói rootfs, ứng dụng, driver module | `petalinux-build` | INITRAMFS: `image.ub`; EXT4: ghi lại `.wic` hoặc chép file qua SSH |
| Chỉ thử nhanh một chương trình | Không cần build image: cross-compile rồi `scp` lên board | Không |

Khi dùng **INITRAMFS** (rootfs nằm trong RAM), mọi thứ bạn sửa trong `/etc` trên board **mất khi reboot**. Muốn giữ thì phải đưa vào image, hoặc dùng rootfs **EXT4** trên thẻ SD (task T7 của plan).

## 8. Tùy biến bằng layer: `meta-modem`

Yocto tổ chức mọi thứ thành **layer** (thư mục bắt đầu bằng `meta-`); giải thích kỹ ở [yocto.md](yocto.md). Mỗi layer chứa **recipe** (`.bb`: công thức build một gói) và **bbappend** (`.bbappend`: sửa thêm vào recipe của layer khác).

Project này có layer dùng chung [`meta-modem/`](../meta-modem/):

| File | Vai trò |
|------|---------|
| `conf/layer.conf` | Khai báo layer: tên collection `modem`, tương thích `gatesgarth` (bản Yocto của PetaLinux 2021.1) |
| `recipes-kernel/linux/linux-xlnx_%.bbappend` + `linux-xlnx/modem.cfg` | Bật thêm option kernel cho modem (UIO, AXI DMA, FPGA manager, SPI, I2C, IIO, PREEMPT) |
| `recipes-modules/modem-regs/` | Driver kernel mẫu: đọc/ghi thanh ghi IP modem trên PL qua `/dev/modem_<địa chỉ>` |
| `recipes-apps/zynq-selftest/` | Script kiểm thử nhanh board sau khi boot |

Thêm layer vào project: `petalinux-config` → **Yocto Settings → User Layers** → `${PROOT}/../../meta-modem`.

Lý do để tùy biến ở layer riêng thay vì sửa thẳng trong project: dùng lại được cho board Zynq khác, và khi AMD thay PetaLinux bằng Yocto thuần (EDF), layer vẫn dùng được.

**Lưu ý cú pháp 2021.1 (Yocto gatesgarth):** dùng `SRC_URI_append`, `FILESEXTRAPATHS_prepend` (dấu gạch dưới). Cú pháp `SRC_URI:append` (dấu hai chấm) chỉ có từ Yocto 3.4, **không** chạy trên 2021.1.

## 9. Device Tree: tại sao cần, sửa ở đâu

Kernel Linux trên ARM không tự dò được phần cứng. **Device Tree** là file mô tả “board này có UART ở địa chỉ nào, PHY Ethernet nối ở đâu, IP modem dùng ngắt số mấy”.

PetaLinux sinh DT tự động từ `.xsa` thành ba file trong `components/`:
- `zynq-7000.dtsi`: phần chung của chip
- `pcw.dtsi`: ngoại vi PS đã bật trong Vivado
- `pl.dtsi`: IP trên PL

Những gì `.xsa` không biết (PHY Ethernet là chip gì, `compatible` của driver tự viết) thì bạn ghi vào **`system-user.dtsi`**, ghi đè node bằng cú pháp `&nhãn { ... };`. Ví dụ gán driver `modem-regs` cho một IP:

```dts
&ten_ip_trong_pl_dtsi {
    compatible = "modem,modem-regs";
};
```

## 10. Bẫy thường gặp (đã gặp ở Lab 8)

| Bẫy | Cách tránh |
|-----|------------|
| `source settings.sh` không đủ đối số | Luôn truyền đường dẫn cài: `settings.sh /opt/pkg/petalinux/2021.1` |
| Menu `petalinux-config` vỡ khung | `stty rows 40 cols 120` trước khi mở |
| `/bin/sh` là dash → build lỗi | Đổi sang bash bằng `sudo dpkg-reconfigure dash` (đã làm ở T1) |
| Đặt tên app/module có dấu `_` → build lỗi | Dùng chữ thường và dấu `-` (vd. `fir_cpu` lỗi, `fircpu` chạy) |
| Đổi `.xsa` nhưng quên `petalinux-package --boot` | Bitstream cũ vẫn nằm trong `BOOT.BIN` cũ trên thẻ |
| Sửa file trong `components/` | Bị ghi đè khi build lại; sửa trong `project-spec/` |
| Chạy `apt` trên board, hoặc dùng gcc của Ubuntu build chương trình cho board | Board không có `apt`; chương trình phải build bằng toolchain ARM của PetaLinux |
| Mật khẩu `root` | 2021.1 mặc định bật `debug-tweaks`: `root`/`root`. Tắt trước khi dùng thật |

## 11. Từ điển nhanh

| Thuật ngữ | Nghĩa |
|-----------|-------|
| **PS** / **PL** | Processing System (2 nhân ARM Cortex-A9 + ngoại vi cứng) / Programmable Logic (phần FPGA) |
| **XSA** | Xilinx Support Archive: file zip mô tả phần cứng, xuất từ Vivado |
| **Yocto / bitbake** | Bộ công cụ build Linux nhúng / chương trình thực thi các recipe |
| **Layer / recipe / bbappend** | Thư mục tùy biến / công thức build một gói / phần sửa thêm vào recipe khác |
| **FIT image (`image.ub`)** | Một file chứa kernel + Device Tree (+ rootfs) mà U-Boot nạp được |
| **INITRAMFS / EXT4** | Rootfs nằm trong RAM (mất khi tắt) / rootfs trên phân vùng thẻ SD (giữ được) |
| **XSCT** | Công cụ dòng lệnh của Xilinx; PetaLinux dùng nó để đọc `.xsa` |
| **bootgen** | Công cụ gói FSBL + bitstream + U-Boot thành `BOOT.BIN` |
| **sstate-cache / downloads** | Bộ nhớ đệm của Yocto; giữ lại giúp build lần sau nhanh hơn nhiều |

## 12. Đọc thêm

- **UG1144 v2021.1** — PetaLinux Tools Reference Guide (tài liệu chính thức; chương 2 cài đặt, chương 3–5 tạo/cấu hình/build/đóng gói).
- **UG585** chương 6 — Boot and Configuration của Zynq-7000.
- **Yocto 3.2 Dev Manual** — mục “Understanding and Creating Layers”.
- Ghi chú thực hành Lab 8: `_ZynQ_Study/_ZynQ_Study/8.E316_B_LINUX/1.Petalinux_HUONG_DAN.txt` (chỉ tham khảo).
