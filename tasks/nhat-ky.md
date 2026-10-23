# Nhật ký thực hiện

Ghi theo ngày: đã làm gì, quyết định gì, vướng gì. Log thô (build, UART) lưu trong `tasks/logs/`.
Kế hoạch: [plan.md](plan.md) · Task: [todo.md](todo.md)

---

## Thứ Tư 21/10/2026

**Đã làm**
- Viết README đầu tiên (quy trình Buildroot) và đẩy lên GitHub `dangcute99/NCKH_V7` (commit `2ada879`).
- Lập plan + task list; rút lịch từ 2 tuần xuống **1 tuần (21/10 – 28/10)**.
- Dựng Makefile + `br2-external` cho Buildroot, chạy thử `e316_defconfig`: cấu hình hợp lệ, khớp từng dòng.
- Viết [docs/quy-trinh-xay-dung.md](../docs/quy-trinh-xay-dung.md): build B1–B9, boot R1–R7, bảng tra lỗi UART.
- **Đổi sang PetaLinux 2023.2**: xóa phần Buildroot (chưa từng commit), viết lại README, quy trình, plan (commit `341ab1b`).
- Review tài liệu, sửa 3 lỗi: `.gitignore` chặn nhầm file của PetaLinux, bootargs ghi lệch giữa README và docs, `dd` thiếu bước xác định thẻ SD (commit `5f78ca6`).
- Dựng graph: graph chung, graph quy trình build + boot (`docs/graphify-out/`), graph chỉ quy trình build (`graphs/xay-dung/`).

**Quyết định**
- Dùng PetaLinux 2023.2 thay Buildroot (người dùng chọn, đã so sánh hai cách).
- Tùy biến dùng chung đặt trong layer Yocto `meta-nckh`; thử trước bằng BSP ZC702 + QEMU.
- Chạy code dự án phải hỏi người dùng trước.

**Phát hiện**
- Không tìm được tài liệu board E316 → thông số trong README là giả định.
- AMD đã thay PetaLinux bằng EDF từ 2025.1; bản PetaLinux cuối dự kiến cuối 2026.

---

## Thứ Năm 22/10/2026

**Đã làm**
- Ghi quyết định **không cài Vivado trên máy build** vào README, docs, plan; thêm `rm_work` và chỉ giữ một project để vừa ổ đĩa.
- Kiểm tra máy build: Ubuntu 22.04.5, 12 luồng, 15 GB RAM, 80 GB trống, mạng tới AMD/Yocto OK; **thiếu 26 gói**, `/bin/sh` là dash, không có swap.
- Viết layer `meta-nckh` (T3–T5): `layer.conf`, kernel fragment `nckh.cfg`, `zynq-selftest`, driver `modem-regs` (commit `cc95903`).
- Chạy thử `zynq-selftest` trên máy host: cú pháp OK, ép FAIL mục CPU → mã thoát 1. Sửa 2 lỗi: báo PASS sai khi không đọc được `dmesg`; lỗi đếm phân vùng `/proc/mtd`.

**Vướng**
- Biên dịch thử `modem-regs` trên host thất bại: kernel host build bằng gcc-12, máy chưa có gcc-12. Để dành build bằng toolchain PetaLinux (T5) hoặc sau khi cài gcc-12.
- **T1, T2 bị chặn**: chưa cài gói hệ thống (cần sudo), chưa có bộ cài PetaLinux + BSP ZC702. Phase 1 trễ ~1 ngày so với lịch (CP1 dự kiến hôm nay).

**Việc tiếp theo**
- Người dùng: cài gói + `gcc-12`, đổi dash, tải bộ cài PetaLinux 2023.2 + BSP ZC702, chốt nguồn `system.xsa` (hạn 23/10).
- Claude: biên dịch thử driver, cài PetaLinux (T1), tạo project ZC702 và build qua đêm (T2).

---

## Thứ Sáu 23/10/2026

**Đã làm**
- Đọc lộ trình học E316 (`_ZynQ_Study/`, chưa đưa vào git): Lab 1–8 xong, Lab 9 (AD9361/IIO) đạt M5 ngày 06/10, tất cả dùng Vivado/PetaLinux **2021.1** + Ubuntu 20.04.
- **Đổi phiên bản 2023.2 → 2021.1** theo lộ trình E316: sửa README, docs, plan, todo; sửa layer `meta-nckh` cho Yocto gatesgarth (`LAYERSERIES_COMPAT`, cú pháp `_append`, license `GPL-2.0`).
- Cập nhật README §2.2 bằng thông số E316 đã kiểm trên board (chip, DDR, UART1 → `ttyPS0`, PHY RTL8211F `rgmii-id`); docs B6 thêm công thức DT Ethernet của Lab 8.

- **T2 xong:** PetaLinux 2021.1 cài tại `/opt/pkg/petalinux/2021.1` (13 GB), `petalinux-*` chạy được. Gói host đã cài; `/bin/sh` đã đổi sang bash (`readlink -f /bin/sh` → `/usr/bin/bash`); cảnh báo thiếu tftp server bỏ qua (boot SD).
- **T1 xong:** 33 gói, bash, `/tools/Xilinx`, nhóm `dialout`, token AMD (hạn 30/10). **T3 bắt đầu:** cài Vivado ML Standard 2021.1 chỉ Zynq-7000 bằng `xsetup -b Install` chạy nền.

**Đính chính mục 22/10**
- Máy build thực tế: **Ubuntu 20.04.6**, kernel 5.15, 12 luồng, 15 GB RAM, **~419 GB trống**. Ghi "22.04.5, 80 GB" hôm 22/10 là sai. `/bin/sh` vẫn là dash; PetaLinux chưa cài.

**Quyết định**
- PetaLinux 2021.1 (người dùng chọn), để dùng lại XSA E316 và `meta-adi` 2021_R1 đã chạy được.
- **Lập lại plan từ đầu** (người dùng chọn): bỏ ZC702 + QEMU, mỗi task là một lát dọc kiểm trên E316 (boot INITRAMFS → eth/SSH → EXT4 `.wic` → `meta-nckh` → selftest → `modem-regs` trên BRAM `0x40000000` → IP AXI-Lite). Bản cũ sao lưu ngoài repo.
- **Làm từ đầu, `_ZynQ_Study/` chỉ để tham khảo** (người dùng chốt): không chép XSA/project/image; tự làm XSA bằng script Tcl trong Vivado 2021.1 **cài trên máy build** (thay quyết định 21/10). Plan sửa lại thành T1–T12: thêm T3 cài Vivado, T4 XSA tối thiểu (PS), Ethernet thành lát Vivado + DT; bỏ bước thử driver trên BRAM.
- **Đổi tên layer `meta-nckh/` → `Modem_P2P/`** (người dùng đổi, xác nhận có chủ ý). Đã cập nhật đường dẫn trong README, docs, plan, todo, CLAUDE.md, Stop hook; giữ nguyên định danh bên trong (collection `nckh`, `nckh.cfg`, `compatible = "nckh,modem-regs"`).
- **Đổi tiếp toàn bộ thành `modem_P2P`** (người dùng yêu cầu): thư mục `modem_P2P/`, collection `modem_P2P`, fragment `modem_P2P.cfg`; DT `compatible = "modem-p2p,modem-regs"` (quy ước DT: chữ thường, không `_` ở vendor prefix). Driver và tài liệu đổi cùng lúc.
- **Chốt tên layer theo quy ước Yocto** (người dùng yêu cầu): thư mục `meta-modem/`, collection `modem`, fragment `modem.cfg`, DT `compatible = "modem,modem-regs"`. Thay cho tên `modem_P2P` ở trên.
- T6 dùng XSA bringup 2021.1 có sẵn trong lộ trình học, không cần làm `.xsa` mới.

**Việc tiếp theo**
- Người dùng: đổi dash → bash, tải bộ cài PetaLinux 2021.1 + BSP ZC702 2021.1.
- Claude: T1 cài PetaLinux, T2 build ZC702 + QEMU; kiểm layer `meta-nckh` build được trên 2021.1.
