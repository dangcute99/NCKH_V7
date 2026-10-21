# Implementation Plan: Linux nhúng trên Zynq-7000 cho modem vệ tinh (PetaLinux 2023.2)

Spec: [README.md](../README.md). Quy trình chi tiết: [docs/quy-trinh-xay-dung.md](../docs/quy-trinh-xay-dung.md). Task list: [todo.md](todo.md).

## Overview

Dựng hệ thống Linux nhúng cho Zynq-7000 (ví dụ board E316) bằng **PetaLinux 2023.2**. Phần tùy biến dùng chung (kernel fragment, driver `modem-regs`, ứng dụng `zynq-selftest`) nằm trong layer Yocto `meta-nckh`. Phần riêng board (`.xsa`, `system-user.dtsi`, cấu hình hệ thống) nằm trong project `plnx/<board>`. Làm trước những phần **không cần board E316 và Vivado** bằng BSP ZC702 + QEMU, sau đó mới áp lên E316.

## Architecture Decisions

- **PetaLinux 2023.2 thay cho Buildroot** (người dùng chọn 21/10). Đây là công cụ chính thức của AMD, tự sinh FSBL, DT, cấu hình U-Boot/kernel từ `.xsa`. Đánh đổi: build nặng, nhiều bước bị ẩn → tài liệu quy trình ghi rõ PetaLinux làm gì bên dưới ở mỗi bước.
- **Tùy biến dùng chung đặt trong layer `meta-nckh`**, thêm vào từng project qua *User Layers*. Phát triển một lần trên project ZC702, dùng lại nguyên vẹn cho E316. Layer chuẩn Yocto nên chuyển được sang EDF/Yocto sau này.
- **Thử trước bằng BSP ZC702 + QEMU.** BSP có sẵn `.xsa` và ảnh mẫu; QEMU của Xilinx mô phỏng ZC702. Nhờ vậy kiểm tra được máy build, kernel, rootfs, driver mà chưa cần Vivado hay `.xsa` của E316.
- **Rootfs EXT4 trên thẻ SD** (thay cho INITRAMFS mặc định), để dữ liệu và cấu hình còn sau khi tắt nguồn.
- **Mọi lệnh chạy code dự án** (`petalinux-*`, script, qemu, ghi thẻ SD) đều phải được người dùng đồng ý trước khi chạy.

## Dependency Graph

```
Cài PetaLinux (T1)
  └── Project BSP ZC702 → build → QEMU (T2)
        ├── meta-nckh: kernel fragment (T3)
        ├── meta-nckh: gói rootfs + zynq-selftest (T4)
        └── meta-nckh: driver modem-regs (T5)
              │
system.xsa E316 (T6, Vivado) ─┐
                              └── Project E316 + system-user.dtsi (T7)
                                    └── build + BOOT.BIN + wic → boot E316 (T8)
                                          ├── IP modem trên PL (T9)
                                          ├── QSPI (T10)
                                          └── đo đạc + ổn định (T11)
```

## Task List

### Phase 1: Môi trường + QEMU (không cần board, không cần Vivado)
- [ ] T1: Cài PetaLinux 2023.2 + gói host
- [ ] T2: Project từ BSP ZC702 → `petalinux-build` → QEMU tới login

### Checkpoint 1
- [ ] Build ZC702 sạch; QEMU boot tới login; người dùng duyệt

### Phase 2: Layer meta-nckh (kiểm thử trên QEMU ZC702)
- [ ] T3: Layer `meta-nckh` + kernel fragment `nckh.cfg`
- [ ] T4: Gói rootfs + ứng dụng `zynq-selftest`
- [ ] T5: Driver mẫu `modem-regs`

### Checkpoint 2
- [ ] `zynq-selftest` chạy trên QEMU, không FAIL ở các mục PS mà QEMU mô phỏng; `modem-regs` nạp/gỡ sạch; người dùng duyệt

### Phase 3: Board E316 (cần Vivado 2023.2 + board)
- [ ] T6: Thiết kế Vivado chỉ có PS cho E316, xuất `system.xsa` *(người dùng làm, Claude hỗ trợ)*
- [ ] T7: Project PetaLinux E316 + cấu hình hệ thống + `system-user.dtsi`
- [ ] T8: Build, `BOOT.BIN`, ảnh `.wic` → boot E316 từ thẻ SD

### Checkpoint 3
- [ ] E316 boot tới login; `zynq-selftest` PASS các mục PS; người dùng duyệt

### Phase 4: Modem IP, QSPI, đo đạc
- [ ] T9: IP thanh ghi trên PL + `modem-regs` trên board thật
- [ ] T10: Boot từ QSPI *(ngoài phạm vi lịch 1 tuần, chỉ làm nếu còn thời gian)*
- [ ] T11: Đo boot time, iperf3, chạy ổn định → báo cáo kiểm thử (rút gọn: 12 giờ, reboot 20 lần)

### Checkpoint 4
- [ ] Đạt các tiêu chí ở README §6 (bản rút gọn); người dùng duyệt

## Lịch thực hiện (1 tuần: Thứ Tư 21/10 – Thứ Tư 28/10/2026)

| Ngày | Task | Checkpoint |
|------|------|------------|
| Thứ Tư 21/10 | T1 (tải + cài ~1–2 giờ); tối: bắt đầu build T2 chạy qua đêm | |
| Thứ Năm 22/10 | T2 (QEMU), T3 | **CP1** |
| Thứ Sáu 23/10 | T4, T5 | **CP2** |
| Thứ Bảy 24/10 – Chủ Nhật 25/10 | dự phòng | |
| Thứ Hai 26/10 | T7, T8 | **CP3** |
| Thứ Ba 27/10 | T9; buổi tối bắt đầu chạy thử ổn định qua đêm | |
| Thứ Tư 28/10 | T11 rút gọn, viết báo cáo | **CP4** |

- **Song song:** người dùng làm T6 (Vivado → `system.xsa`) với **hạn chót Thứ Sáu 23/10**.
- **Đường găng:** T6 → T7 → T8. Nếu `.xsa` hoặc tài liệu E316 trễ bao nhiêu ngày thì Phase 3–4 trễ bấy nhiêu ngày.
- **Cắt phạm vi để vừa 1 tuần:** T10 (QSPI) bỏ; T11 rút gọn (12 giờ thay cho 24 giờ, 20 lần reboot thay cho 100). Nếu vẫn trễ thì bỏ tiếp T9, giữ `modem-regs` ở mức chạy được trên QEMU.

## Risks and Mitigations

| Risk | Impact | Mitigation |
|------|--------|------------|
| Ổ đĩa: còn ~80 GB, trong khi PetaLinux ~10 GB + project ZC702 ~50 GB + project E316 ~50 GB + Vivado ~70 GB | High | Cài Vivado ở máy khác hoặc mở rộng ổ; dùng chung thư mục downloads/sstate giữa 2 project; xóa `plnx/zc702/build` sau CP2 |
| Chưa có bộ cài PetaLinux, BSP ZC702, Vivado (phải tải bằng tài khoản AMD) | High | Người dùng tải ngay ngày 21/10; T1 bị chặn tới khi có bộ cài |
| Chưa có tài liệu E316 (DDR, UART, PHY, boot mode) | High | Phase 1–2 không phụ thuộc board; cần schematic trước T6 |
| Ubuntu 22.04.5 có thể nằm ngoài danh sách hỗ trợ chính thức của 2023.2 | Med | Thường vẫn chạy; nếu installer từ chối thì xử lý theo thông báo, ghi lại cách xử lý |
| Build lần đầu 1–2 giờ, cần mạng ổn định | Med | Chạy nền qua đêm; giữ `build/` |
| QEMU khác phần cứng thật (không có PL, khác PHY) | Med | QEMU chỉ dùng kiểm thử kernel/rootfs/driver; T8–T11 kiểm thử trên board |
| PetaLinux đã bị thay bằng EDF từ 2025.1 | Low | 2023.2 khớp Vivado 2023.2, đủ cho đề tài; tùy biến nằm trong layer Yocto nên chuyển được |

## Open Questions

- Board E316: nhà sản xuất, chip chính xác (7z010/7z020?), DDR, UART nào ra console, PHY Ethernet?
- Máy có Vivado 2023.2 không, cài ở máy này hay máy khác (ổ đĩa không đủ cho tất cả)?
- IP modem trên PL đã có thiết kế chưa, hay T9 dùng một IP thanh ghi giả để thử?
