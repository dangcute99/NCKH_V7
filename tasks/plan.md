# Implementation Plan: Linux nhúng trên Zynq-7000 cho modem vệ tinh (PetaLinux 2021.1, E316)

Spec: [README.md](../README.md). Quy trình chi tiết: [docs/quy-trinh-xay-dung.md](../docs/quy-trinh-xay-dung.md). Task list: [todo.md](todo.md). Nhật ký: [nhat-ky.md](nhat-ky.md). Lập lại 23/10/2026; sửa cùng ngày khi người dùng chốt “làm từ đầu, `_ZynQ_Study` chỉ tham khảo”.

## Overview

Dựng Linux nhúng cho ANTSDR E316 **từ đầu** bằng Vivado 2021.1 + PetaLinux 2021.1 trên cùng máy build Ubuntu 20.04.6. Mỗi task là một **lát dọc chạy trên board**: thêm đúng một khả năng (login, mạng, rootfs bền, kernel fragment, selftest, IP modem + driver) và kiểm ngay trên E316. Phần dùng chung nằm trong layer `meta-modem`; phần riêng board nằm trong `hw/e316/` (project Vivado + XSA) và `plnx/e316/project-spec/`.

## Architecture Decisions

- **Làm từ đầu; `_ZynQ_Study/` chỉ để tham khảo** (người dùng chốt 23/10). Không chép XSA, project Vivado hay image từ đó. Được đọc để đối chiếu thông số đã kiểm trên board: DDR `MT41J256M16 RE-125` 32-bit, UART1 MIO 12–13, chân + clock Ethernet (ENET0 EMIO, `gmii_to_rgmii`, FCLK_CLK2 200 MHz), công thức DT PHY RTL8211F.
- **Vivado 2021.1 cài trên máy build** (thay quyết định 21/10). Lý do: làm XSA từ đầu cần sửa thiết kế nhiều lần; máy còn ~419 GB. Chỉ cài Vivado ML Standard + họ Zynq-7000 (~30 GB), không cài Vitis. Không chạy synthesis song song với `petalinux-build` (15 GB RAM, chưa có swap).
- **XSA làm theo lát:** XSA đầu chỉ có PS (DDR, UART1, SD0) để boot; thêm Ethernet ở lát mạng; thêm IP AXI-Lite ở lát driver. Mỗi lần đổi XSA: `--get-hw-description` → `petalinux-package --boot` → chép lại `BOOT.BIN`.
- **Bỏ ZC702 + QEMU, kiểm thẳng trên E316.** Đánh đổi: không có môi trường thử khi chưa có board; bù bằng snapshot `deploy/` trước mỗi thay đổi lớn và khoanh vùng 3 lớp PCB / FPGA / Linux.
- **Driver `modem-regs` kiểm trên IP AXI-Lite tự làm** (thanh ghi version cố định + scratch), không qua BRAM trung gian.
- **Yocto gatesgarth:** recipe dùng `_append`, license `GPL-2.0`; lần đầu kiểm ở T8.
- **Mọi lệnh chạy code dự án** (`petalinux-*`, Vivado batch, ghi thẻ SD, chạy trên board) phải được người dùng đồng ý trước.

## Dependency Graph

```
T1 Máy build sẵn sàng (người dùng: sudo, tải bộ cài, token AMD)
 ├── T2 Cài PetaLinux 2021.1 ─────────────┐
 └── T3 Cài Vivado 2021.1                 │
      └── T4 XSA tối thiểu (PS: DDR, UART1, SD0)
           └── T5 E316 boot tới login (INITRAMFS) ← đường găng, rủi ro cao nhất
                └── T6 Ethernet (Vivado EMIO + gmii_to_rgmii + DT) + SSH
                     └── T7 Rootfs EXT4 trên SD (.wic)
                          └── T8 meta-modem + kernel fragment
                               ├── T9 zynq-selftest trên board
                               └── T10 IP AXI-Lite (Vivado) + modem-regs
T11 Đo đạc + báo cáo (cần T7, T9; T10 nếu kịp)
T12 QSPI (tuỳ chọn, sau T7)
```

## Task List

### Phase 1: Công cụ + đường boot tối thiểu
- [ ] T1: Máy build sẵn sàng *(người dùng)*
- [ ] T2: Cài PetaLinux 2021.1
- [ ] T3: Cài Vivado 2021.1 (ML Standard, Zynq-7000)
- [ ] T4: XSA tối thiểu cho E316 (PS: DDR, UART1, SD0)
- [ ] T5: E316 boot tới login qua UART (INITRAMFS)

### Checkpoint 1
- [ ] E316 login UART bằng XSA + image tự làm; người dùng duyệt

### Phase 2: Hệ thống dùng được
- [ ] T6: Ethernet + SSH (Vivado + DT)
- [ ] T7: Rootfs EXT4 trên thẻ SD (`.wic`)
- [ ] T8: Layer `meta-modem` + kernel fragment

### Checkpoint 2
- [ ] Boot từ `.wic`, SSH được, dữ liệu còn sau reboot, kernel có option modem; commit; người dùng duyệt

### Phase 3: Kiểm thử và driver modem
- [ ] T9: `zynq-selftest` chạy trên board
- [ ] T10: IP AXI-Lite (version + scratch) + `modem-regs`

### Checkpoint 3
- [ ] selftest PASS PS + PL + modem; người dùng duyệt

### Phase 4: Đo đạc
- [ ] T11: Đo boot, mạng, rootfs, ổn định → `docs/test-report.md`
- [ ] T12: Boot QSPI *(tuỳ chọn)*

### Checkpoint 4
- [ ] Đạt tiêu chí README §6 (bản rút gọn); người dùng duyệt

## Lịch (Thứ Sáu 23/10 – Thứ Tư 28/10/2026)

| Ngày | Task | Checkpoint |
|------|------|------------|
| Thứ Sáu 23/10 | T1 (người dùng); T2; T3 chạy nền (tải + cài 1–2 giờ) | |
| Thứ Bảy 24/10 | T4 (thiết kế + bitstream); tối: build đầu PetaLinux | |
| Chủ Nhật 25/10 | T5 | **CP1** |
| Thứ Hai 26/10 | T6, T7 | |
| Thứ Ba 27/10 | T8, T9 | **CP2** |
| Thứ Tư 28/10 | T10, T11 rút gọn, báo cáo | **CP3/CP4** |

- **Đường găng:** T1 → T3 → T4 → T5 → T6 → T7 → T8. Làm từ đầu thêm T3, T4 và phần Vivado của T6 vào đường găng; lịch 1 tuần **rất sát**.
- **Cắt phạm vi nếu trễ (theo thứ tự):** bỏ T12; T11 rút gọn (12 giờ, 20 lần reboot); T10 lùi sang tuần sau; giữ CP2 làm mốc tối thiểu cho 28/10.
- **Song song được:** T2 với T3 (khác thư mục, nhưng không build cùng lúc); viết `system-user.dtsi` của T6 trong lúc chờ bitstream.

## Risks and Mitigations

| Risk | Impact | Mitigation |
|------|--------|------------|
| Cấu hình DDR sai trong XSA tự làm → treo sau banner FSBL | High | Đối chiếu từng thông số DDR với `system_bd.tcl` vàng (chỉ đọc); kiểm ở T5 với XSA chỉ có PS trước khi thêm PL |
| Chưa có bộ cài PetaLinux / Vivado 2021.1 (cần tài khoản AMD) | High | Người dùng tải và tạo token trong T1; T2, T3 bị chặn tới khi có |
| Mirror Yocto của 2021.1 đã cũ, build đầu lỗi tải gói | High | Build đầu ngay khi có XSA (T5); giữ `downloads/`, `sstate-cache/` |
| Ethernet qua EMIO + `gmii_to_rgmii` sai clock/XDC → không link | Med | Tách thành lát riêng T6 sau khi boot ổn; đối chiếu XDC và FCLK_CLK2 200 MHz với tài liệu tham khảo |
| 15 GB RAM, không swap: Vivado + PetaLinux cùng lúc hết RAM | Med | Chạy lần lượt; cân nhắc tạo swapfile 16 GB (người dùng quyết) |
| Sửa đổi gatesgarth của `meta-modem` sai | Med | T8 tách riêng sau khi board chạy ổn |
| Ghi nhầm thiết bị khi `dd` thẻ SD | High | `lsblk` trước/sau; người dùng xác nhận `/dev/sdX` |
| Lịch 1 tuần không đủ khi làm từ đầu | High | Thứ tự cắt phạm vi ở trên; CP2 là mốc tối thiểu |

## Open Questions

- T7 đổi rootfs sang EXT4 (thay INITRAMFS mặc định): giữ?
- T10: IP modem thật hay IP thanh ghi giả (version cố định + scratch)?
- Tạo swapfile 16 GB trên máy build?
- `docs/lo-trinh.md` và graph lộ trình còn theo plan cũ: cập nhật sau khi người dùng duyệt plan này.
