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
