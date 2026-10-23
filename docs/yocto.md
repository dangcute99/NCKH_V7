# Tìm hiểu Yocto (bên dưới PetaLinux)

Tài liệu giải thích Yocto cho người mới, đủ để đọc và sửa layer [`meta-modem/`](../meta-modem/) của dự án. Phiên bản: **Yocto 3.2 “gatesgarth”**, là bản PetaLinux 2021.1 dùng bên dưới. Phần PetaLinux xem [petalinux.md](petalinux.md).

## 1. Yocto là gì (và không phải là gì)

**Yocto không phải một bản Linux** như Ubuntu. Nó là **bộ công cụ để bạn tự build một bản Linux riêng** cho board nhúng: biên dịch từ mã nguồn mọi thứ, từ trình biên dịch chéo, kernel, thư viện đến ứng dụng, rồi gói thành ảnh để nạp lên board.

| Ubuntu trên PC | Yocto cho board |
|----------------|-----------------|
| Tải gói đã build sẵn (`apt install`) | Build mọi gói từ mã nguồn trên máy build |
| Một bản dùng cho mọi máy | Mỗi board một bản, chỉ có đúng thứ cần |
| Rootfs vài GB | Rootfs có thể chỉ vài chục MB |
| Cài thêm gói lúc chạy | Muốn thêm gói phải build lại ảnh (board không có `apt`) |

**PetaLinux = Yocto + lớp lệnh `petalinux-*` + công cụ đọc `.xsa` của Xilinx.** Khi bạn gõ `petalinux-build`, thực chất PetaLinux gọi `bitbake` của Yocto.

## 2. Các khái niệm cốt lõi

```
                 ┌──────────────── Metadata (văn bản bạn đọc/viết) ────────────────┐
                 │  layer (meta-*)                                                  │
                 │   ├── conf/layer.conf          khai báo layer                    │
                 │   ├── recipes-*/<gói>/<gói>.bb      công thức build một gói      │
                 │   ├── recipes-*/<gói>/<gói>.bbappend  sửa thêm recipe của layer khác │
                 │   └── classes/*.bbclass        đoạn logic dùng chung (vd. module) │
                 └───────────────────────────────┬──────────────────────────────────┘
                                                 │ đọc
                                          ┌──────▼──────┐
                                          │   BitBake   │  công cụ thực thi
                                          └──────┬──────┘
                                                 │ chạy các task của từng recipe
                     fetch → unpack → patch → configure → compile → install → package
                                                 │
                                          ┌──────▼──────┐
                                          │ Ảnh (image) │  rootfs + kernel + bootloader
                                          └─────────────┘
```

| Khái niệm | Giải thích | Ví dụ trong dự án |
|-----------|------------|-------------------|
| **Poky** | Bản tham chiếu của Yocto: BitBake + metadata lõi (`meta`, `meta-poky`) | PetaLinux đã kèm sẵn |
| **BitBake** | Chương trình đọc recipe, giải quyết phụ thuộc, chạy task song song | Gọi gián tiếp qua `petalinux-build` |
| **Layer** | Thư mục gom recipe theo chủ đề, tên bắt đầu `meta-` | `meta-modem`, `meta-xilinx`, `meta-petalinux`, `meta-user` |
| **Recipe (`.bb`)** | Công thức build một gói: lấy nguồn ở đâu, build thế nào, cài file gì | `modem-regs.bb`, `zynq-selftest.bb` |
| **bbappend** | Sửa thêm vào recipe có sẵn ở layer khác, không đụng file gốc | `linux-xlnx_%.bbappend` thêm config cho kernel |
| **Class (`.bbclass`)** | Logic dùng chung, gọi bằng `inherit` | `inherit module` để build module kernel |
| **Configuration (`.conf`)** | Biến toàn cục: máy đích, distro, layer nào được dùng | `layer.conf`, `petalinuxbsp.conf` |
| **MACHINE** | Board đích, quyết định CPU, kernel, bootloader | PetaLinux đặt theo `.xsa` (Zynq-7000) |
| **DISTRO** | Chính sách chung: init system, tính năng | `petalinux` |
| **Image** | Recipe đặc biệt gom nhiều gói thành rootfs | `petalinux-image-minimal` |
| **Task** | Một bước build của recipe (`do_fetch`, `do_compile`, …) | `petalinux-build -c modem-regs -x compile` |

## 3. Một recipe đi qua những bước nào

| Task | Làm gì | Hay lỗi vì |
|------|--------|-----------|
| `do_fetch` | Tải nguồn (git, tarball, hoặc `file://` trong layer) | Mất mạng, URL cũ (2021.1 đã cũ, mirror có thể chết) |
| `do_unpack` | Giải nén vào thư mục làm việc `WORKDIR` | — |
| `do_patch` | Áp các file `.patch` khai báo trong `SRC_URI` | Patch không khớp phiên bản nguồn |
| `do_configure` | Chạy `./configure`, `cmake`, hoặc ghép config kernel | Thiếu phụ thuộc (`DEPENDS`) |
| `do_compile` | Biên dịch bằng trình biên dịch chéo ARM | Lỗi code, thiếu header |
| `do_install` | Chép file kết quả vào `${D}` (thư mục giả lập `/` của board) | Quên cài file → gói rỗng |
| `do_package` | Chia file thành gói, kiểm tra license | `LIC_FILES_CHKSUM` sai |
| `do_rootfs` (của image) | Gom các gói vào rootfs | Gói không được thêm vào image |

Nơi tìm log khi lỗi: `build/tmp/work/<kiến trúc>/<recipe>/<phiên bản>/temp/log.do_<task>`. Lệnh in ra cũng ghi đường dẫn này.

## 4. Biến và cú pháp (bản gatesgarth)

Recipe và file `.conf` là danh sách gán biến. Các phép gán hay gặp:

| Cú pháp | Nghĩa |
|---------|-------|
| `A = "x"` | Gán (giá trị được mở rộng lúc dùng) |
| `A := "x"` | Gán và mở rộng ngay lúc đọc |
| `A ?= "x"` | Gán nếu chưa có giá trị |
| `A += "x"` | Nối thêm, **có** dấu cách |
| `A_append = " x"` | Nối thêm vào cuối, **không** tự thêm dấu cách (nên tự viết `" x"`), áp dụng sau cùng |
| `A_prepend = "x "` | Chèn vào đầu, áp dụng sau cùng |
| `A_remove = "x"` | Bỏ từ `x` khỏi giá trị |
| `${VAR}` | Lấy giá trị biến khác |

**Cú pháp override đổi theo phiên bản Yocto. Đây là chỗ dễ sai nhất khi đọc tài liệu trên mạng:**

| Yocto | Tên | Cú pháp override | Ai dùng |
|-------|-----|------------------|---------|
| 3.1 | dunfell | `SRC_URI_append` | — |
| **3.2** | **gatesgarth** | **`SRC_URI_append`** (dấu `_`) | **PetaLinux 2021.1: dự án này** |
| 3.3 | hardknott | `_append` | PetaLinux 2021.2 |
| 3.4 | honister | `SRC_URI:append` (dấu `:`) | — |
| 4.0 | kirkstone | `:append` | PetaLinux 2022.x |
| 4.1 | langdale | `:append` | PetaLinux 2023.x |

Ví dụ trên mạng hiện nay phần lớn viết `:append`. Trên 2021.1 phải đổi thành `_append`, nếu không bitbake báo lỗi cú pháp.

Các biến hay gặp trong recipe:

| Biến | Nghĩa |
|------|-------|
| `SUMMARY`, `DESCRIPTION` | Mô tả gói |
| `LICENSE`, `LIC_FILES_CHKSUM` | Giấy phép + md5 của file giấy phép (bắt buộc; sai md5 là build dừng) |
| `SRC_URI` | Nguồn: `git://…`, `https://…`, hoặc `file://tên` (file nằm cạnh recipe) |
| `S` | Thư mục chứa mã nguồn sau khi giải nén |
| `WORKDIR` | Thư mục làm việc của recipe |
| `D` | Thư mục đích giả lập `/` của board, dùng trong `do_install` |
| `bindir`, `sysconfdir` | `/usr/bin`, `/etc` |
| `DEPENDS` | Gói cần lúc **build** |
| `RDEPENDS_${PN}` | Gói cần lúc **chạy** trên board (gatesgarth dùng `_`) |
| `PN`, `PV` | Tên gói, phiên bản (lấy từ tên file `.bb`) |
| `FILESEXTRAPATHS` | Thêm thư mục để tìm file `file://…` |

## 5. Đọc layer `meta-modem` từng file

### `conf/layer.conf` — khai báo layer

```bitbake
BBPATH .= ":${LAYERDIR}"                       # cho BitBake biết thư mục layer
BBFILES += "${LAYERDIR}/recipes-*/*/*.bb ${LAYERDIR}/recipes-*/*/*.bbappend"   # tìm recipe ở đâu
BBFILE_COLLECTIONS += "modem"                  # tên collection của layer
BBFILE_PATTERN_modem = "^${LAYERDIR}/"
BBFILE_PRIORITY_modem = "7"                    # ưu tiên khi 2 layer có cùng recipe
LAYERDEPENDS_modem = "core"                    # cần layer lõi của Yocto
LAYERSERIES_COMPAT_modem = "gatesgarth"        # chạy với Yocto 3.2; sai tên là bị cảnh báo/từ chối
```

### `recipes-kernel/linux/linux-xlnx_%.bbappend` — thêm cấu hình kernel

```bitbake
FILESEXTRAPATHS_prepend := "${THISDIR}/${PN}:"   # tìm file trong thư mục linux-xlnx/ cạnh bbappend
SRC_URI_append = " file://modem.cfg"            # thêm fragment cấu hình vào nguồn kernel
KERNEL_FEATURES_append = " modem.cfg"           # bảo kernel ghép fragment này vào .config
```

- `linux-xlnx_%`: dấu `%` nghĩa là áp cho **mọi phiên bản** của recipe `linux-xlnx` (kernel của Xilinx).
- `modem.cfg` là **config fragment**: vài dòng `CONFIG_…=y`, được trộn vào cấu hình mặc định `xilinx_zynq_defconfig` thay vì chép cả file `.config` khổng lồ.

### `recipes-modules/modem-regs/modem-regs.bb` — build module kernel

```bitbake
LICENSE = "GPL-2.0"                             # gatesgarth chưa có tên "GPL-2.0-only"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/GPL-2.0;md5=801f80980d171dd6425610833a22dbe6"
inherit module                                  # dùng class build module ngoài cây kernel
SRC_URI = "file://Makefile file://modem-regs.c"
S = "${WORKDIR}"                                # nguồn nằm thẳng trong WORKDIR
```

`inherit module` lo hết: tìm kernel đã build, gọi `make` với đúng trình biên dịch chéo, cài `.ko` vào `/lib/modules/…`.

### `recipes-apps/zynq-selftest/zynq-selftest.bb` — cài một script

```bitbake
SRC_URI = "file://zynq-selftest"
S = "${WORKDIR}"
do_install() {
    install -Dm 0755 ${WORKDIR}/zynq-selftest ${D}${bindir}/zynq-selftest
}
```

Không cần biên dịch; chỉ chép script vào `/usr/bin/` của board.

## 6. PetaLinux ánh xạ lên Yocto thế nào

| Bạn làm trong PetaLinux | Tương ứng trong Yocto |
|-------------------------|-----------------------|
| `petalinux-build` | `bitbake petalinux-image-minimal` (và FSBL, U-Boot, DT) |
| `petalinux-build -c <recipe>` | `bitbake <recipe>` |
| `petalinux-build -c <recipe> -x <task>` | `bitbake -c <task> <recipe>` (vd. `-x cleansstate` xoá cache của recipe) |
| `petalinux-config` → User Layers | Thêm layer vào `BBLAYERS` |
| `petalinux-config -c rootfs` | Chọn gói đưa vào `IMAGE_INSTALL` |
| `project-spec/meta-user/` | Một layer riêng của project, PetaLinux tạo sẵn |
| `meta-user/conf/petalinuxbsp.conf` | Nơi đặt biến Yocto toàn cục (vd. `INHERIT += "rm_work"`) |
| `build/tmp/` | `TMPDIR` của Yocto: kết quả trung gian |
| `build/downloads/`, `build/sstate-cache/` | `DL_DIR`, `SSTATE_DIR`: bộ nhớ đệm, giữ lại để build lần sau nhanh |

Các layer có sẵn trong project PetaLinux 2021.1: `meta` + `meta-poky` (lõi Yocto), `meta-openembedded` (gói mở rộng), `meta-xilinx` (hỗ trợ chip Xilinx), `meta-petalinux` (cấu hình PetaLinux), `meta-user` (của project), cộng thêm `meta-modem` khi ta khai báo.

## 7. Bộ nhớ đệm: vì sao lần đầu lâu, lần sau nhanh

- **`downloads/`**: mọi mã nguồn đã tải. Lần sau không tải lại.
- **`sstate-cache/`** (shared state): kết quả từng task, đánh dấu bằng hash của đầu vào. Nếu đầu vào không đổi, BitBake lấy lại kết quả thay vì build lại.

Lần build đầu: 1–2 giờ và cần mạng. Sửa một recipe nhỏ: vài phút, vì chỉ recipe đó và những thứ phụ thuộc nó bị build lại. **Đừng xoá hai thư mục này** trừ khi cần build sạch hoàn toàn.

`INHERIT += "rm_work"` xoá thư mục làm việc của từng gói sau khi build xong để tiết kiệm ổ đĩa (`build/` giảm từ ~40–50 GB còn ~15–25 GB). Đổi lại, khó xem file trung gian khi gỡ lỗi.

## 8. Thêm một gói mới vào dự án (công thức chung)

1. Tạo thư mục `meta-modem/recipes-apps/<tên-gói>/` (tên dùng chữ thường và `-`, không dùng `_`).
2. Đặt nguồn vào `files/`, viết `<tên-gói>.bb` (mẫu: `zynq-selftest.bb` nếu chỉ cài file; `inherit module` nếu là driver; `inherit cmake` hoặc Makefile nếu là chương trình C).
3. Thêm vào rootfs: dòng `CONFIG_<tên-gói>` trong `project-spec/meta-user/conf/user-rootfsconfig`, rồi bật trong `petalinux-config -c rootfs`.
4. Build thử riêng: `petalinux-build -c <tên-gói>`; lỗi thì đọc `log.do_<task>`.
5. Build ảnh: `petalinux-build`.

Có thể dùng `petalinux-create -t apps --name <tên> --template c` hoặc `-t modules` để PetaLinux sinh recipe mẫu trong `meta-user`, sau đó chuyển sang `meta-modem/`.

## 9. Lỗi hay gặp

| Thông báo / hiện tượng | Nguyên nhân thường gặp | Cách xử lý |
|------------------------|------------------------|------------|
| `ParseError … :append` | Dùng cú pháp `:` trên gatesgarth | Đổi thành `_append` |
| `Layer modem is not compatible with the core layer` | `LAYERSERIES_COMPAT` sai tên | Đặt `gatesgarth` |
| `LIC_FILES_CHKSUM … md5 mismatch` hoặc không tìm thấy file license | Tên license / md5 sai | Dùng tên có trong `${COMMON_LICENSE_DIR}` (vd. `GPL-2.0`, `MIT`) |
| `Fetcher failure` | Mất mạng, mirror cũ không còn | Kiểm tra mạng; giữ `downloads/`; thử lại |
| Gói build xong nhưng không có trên board | Chưa thêm vào image | `user-rootfsconfig` + bật trong menu rootfs |
| Sửa file nhưng build không đổi | Cache cũ | `petalinux-build -c <recipe> -x cleansstate` rồi build lại |
| Tên recipe có `_` lỗi lạ | `_` trùng với cú pháp override | Đổi sang `-` |

## 10. Học theo thứ tự nào

1. Đọc mục 2–4 tài liệu này, rồi đọc lại 4 file của `meta-modem` (mục 5) cho tới khi hiểu từng dòng.
2. Khi chạy T8 (thêm `meta-modem` vào project): quan sát log, tìm thư mục `build/tmp/work/…/linux-xlnx/…` và file `.config` sinh ra.
3. Thử thay đổi nhỏ: thêm một option vào `modem.cfg`, build lại kernel, kiểm tra trên board.
4. Viết một recipe ứng dụng C nhỏ (mục 8) và đưa lên board.

## 11. Đọc thêm

- **Yocto Project 3.2 Overview and Concepts Manual**: khái niệm, luồng build.
- **Yocto Project 3.2 Development Tasks Manual**: “Understanding and Creating Layers”, “Writing a New Recipe”.
- **Yocto Project 3.2 Kernel Development Manual**: “Creating Configuration Fragments”.
- **BitBake User Manual** (bản đi cùng 3.2): cú pháp biến và override.
- Tài liệu trên docs.yoctoproject.org có chọn phiên bản ở góc trang: chọn **3.2 (gatesgarth)** để cú pháp khớp.
