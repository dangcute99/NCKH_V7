# Dàn ý báo cáo nghiên cứu

**Đề tài:** Nghiên cứu phương pháp và quy trình xây dựng, tùy biến hệ điều hành Linux nhúng trên SoC Zynq-7000 ứng dụng cho modem vệ tinh.

Tài liệu này là **bản đồ để viết báo cáo**: mỗi chương lấy nội dung từ file nào trong repo, đã có gì, còn thiếu gì, và cần thu thập bằng chứng gì trong lúc làm. Cập nhật cột “Trạng thái” khi tiến độ thay đổi.

## 1. Cấu trúc báo cáo và nguồn nội dung

| Chương | Nội dung chính | Nguồn trong repo | Trạng thái |
|--------|----------------|------------------|------------|
| **Mở đầu** | Lý do chọn đề tài; mục tiêu; đối tượng và phạm vi (Zynq-7000, board ANTSDR E316, PetaLinux 2021.1); phương pháp nghiên cứu; đóng góp | [README.md](../README.md) §1–2; mục 2 bên dưới | Có ý, cần viết thành văn |
| **Ch. 1 – Tổng quan** | 1.1 SoC Zynq-7000: PS, PL, AXI, chuỗi boot. 1.2 Thành phần hệ Linux nhúng: bootloader, kernel, Device Tree, rootfs, driver. 1.3 Công cụ xây dựng: PetaLinux, Yocto, Buildroot; so sánh. 1.4 Yêu cầu phần mềm của modem vệ tinh đối với hệ điều hành | [petalinux.md](petalinux.md), [yocto.md](yocto.md), [quy-trinh-xay-dung.md](quy-trinh-xay-dung.md) Phần B | 1.1–1.2 có; **thiếu 1.3 (bảng so sánh) và 1.4** |
| **Ch. 2 – Phương pháp và quy trình** | 2.1 Lựa chọn công cụ và phiên bản (lý do PetaLinux, 2021.1). 2.2 Phương pháp phát triển theo lát dọc, kiểm chứng trên phần cứng thật. 2.3 Quy trình build B0–B9 và chuỗi khởi động R1–R7. 2.4 Tổ chức phần tùy biến: layer `meta-modem`, `project-spec/`, script Vivado. 2.5 Phương pháp gỡ lỗi theo 3 lớp PCB / FPGA / Linux | [quy-trinh-xay-dung.md](quy-trinh-xay-dung.md) (nguồn chính); [../tasks/plan.md](../tasks/plan.md) mục Architecture Decisions; [lo-trinh.md](lo-trinh.md) | 2.3 có; 2.1, 2.2, 2.4 có ý trong plan, cần viết thành văn |
| **Ch. 3 – Thiết kế và tùy biến cho E316** | 3.1 Thiết kế phần cứng (block design, bảng thông số PS/DDR/MIO, Ethernet EMIO). 3.2 Cấu hình hệ thống. 3.3 Tùy biến kernel (fragment `modem.cfg`, lý do từng nhóm option). 3.4 Device Tree. 3.5 Rootfs. 3.6 Driver `modem-regs` (thiết kế, giao diện `/dev/modem_*`). 3.7 Ứng dụng kiểm thử `zynq-selftest` | `hw/e316/` (T4, T6, T10); [README.md](../README.md) §5.1–5.3; [../meta-modem/](../meta-modem/); [quy-trinh-xay-dung.md](quy-trinh-xay-dung.md) B1–B7 | Thiết kế có; 3.1 có kết quả T4 (XSA chỉ PS); **còn thiếu** T5–T10 |
| **Ch. 4 – Thực nghiệm và đánh giá** | 4.1 Môi trường thực nghiệm (máy build, công cụ, board). 4.2 Kịch bản kiểm thử. 4.3 Kết quả: build (thời gian, dung lượng), boot (thời gian từng mốc), chức năng (selftest), hiệu năng (iperf3, RAM, rootfs), ổn định. 4.4 Vấn đề gặp và cách xử lý. 4.5 Đánh giá theo chỉ tiêu | [README.md](../README.md) §6; [quy-trinh-xay-dung.md](quy-trinh-xay-dung.md) dòng *Kết quả thực tế* + *Vấn đề gặp*; `docs/test-report.md` (T11); `tasks/logs/`; [../tasks/nhat-ky.md](../tasks/nhat-ky.md) | 4.1 có (B0); **còn lại chờ T5–T11** |
| **Kết luận và hướng phát triển** | Kết quả đạt được so với mục tiêu; hạn chế (PetaLinux 2021.1 đã cũ, chưa QSPI, chưa AD9361); hướng phát triển (chuyển Yocto/EDF, tích hợp AD9361/IIO, IP modem thật) | [README.md](../README.md) §8 | Chưa viết |
| **Tài liệu tham khảo** | UG585, UG1144, UG1165, UG1118, UG973, PG160, Yocto 3.2 manuals, LDD3, devicetree.org spec, … | Mục “Đọc thêm” trong [petalinux.md](petalinux.md), [yocto.md](yocto.md), [lo-trinh.md](lo-trinh.md) | Cần gom thành danh sách chuẩn (tên, phiên bản, năm) |
| **Phụ lục** | Lệnh đầy đủ; `create_project.tcl`; `system-user.dtsi`; `modem.cfg`; mã `modem-regs.c`; log boot | `hw/e316/`, `meta-modem/`, `plnx/e316/project-spec/`, `tasks/logs/` | Theo tiến độ |

## 2. Những gì cần bổ sung ngoài file quy trình

1. **Bảng so sánh công cụ xây dựng** (Ch. 1.3, 2.1): PetaLinux / Yocto thuần / Buildroot theo các tiêu chí: hỗ trợ Zynq, sinh FSBL/DT từ `.xsa`, thời gian build, dung lượng, độ linh hoạt, tài liệu, tương lai (AMD chuyển sang EDF từ 2025.1). Lý do chọn PetaLinux 2021.1 đã có trong [../tasks/plan.md](../tasks/plan.md) và README §3.4.
2. **Yêu cầu của modem vệ tinh đối với hệ điều hành** (Ch. 1.4): truy cập IP trên PL (UIO / driver), DMA mẫu tín hiệu, độ trễ (PREEMPT), cấu hình front-end RF qua SPI/I2C, cập nhật bitstream khi chạy. Ánh xạ mỗi yêu cầu sang tùy biến tương ứng (bảng README §5.1 là điểm xuất phát).
3. **Lý do của từng quyết định tùy biến**: vì sao EXT4 thay INITRAMFS, vì sao driver kernel thay vì chỉ `generic-uio`, vì sao từng option kernel. Ghi ngay khi làm task, trong dòng *Vấn đề gặp & xử lý* hoặc nhật ký.
4. **Số liệu thực nghiệm** (Ch. 4): đo theo mục 3 bên dưới.
5. **Hình minh họa**: sơ đồ kiến trúc, chuỗi boot, block design, ảnh chụp log UART, ảnh board.
6. **Danh mục tài liệu tham khảo** chuẩn hoá.

## 3. Bằng chứng cần thu thập trong lúc làm

Thu ngay khi làm từng task; làm lại để lấy số liệu sau này tốn thời gian hơn nhiều.

| Bằng chứng | Task | Lưu ở đâu | Dùng cho |
|------------|------|-----------|----------|
| Thông số máy build, phiên bản công cụ, thời gian và dung lượng cài | T1–T3 | ✅ đã ghi trong [quy-trinh-xay-dung.md](quy-trinh-xay-dung.md) B0 | Ch. 4.1 |
| Ảnh chụp block design, Address Editor, cấu hình DDR/MIO trong Vivado | T4, T6, T10 | `docs/hinh/` | Ch. 3.1 |
| Báo cáo tài nguyên PL (utilization) và timing | T4, T6, T10 | `tasks/logs/` | Ch. 3.1, 4.3 |
| Thời gian build lần đầu / build lại; dung lượng `build/`, `downloads/`, `sstate-cache/` | T5 | `tasks/logs/`, B8 *Kết quả thực tế* | Ch. 4.3 |
| Log UART đầy đủ từ lúc cấp nguồn tới login, có dấu thời gian | T5, T7 | `tasks/logs/t5-boot.log`, `t7-ext4.log` | Ch. 4.3, Phụ lục |
| Kích thước `BOOT.BIN`, `image.ub`, rootfs, `.wic` | T5, T7 | B8 *Kết quả thực tế* | Ch. 4.3 |
| Kết quả `zynq-selftest` | T9, T10 | `tasks/logs/t9-selftest.log` | Ch. 4.3 |
| Kết quả đọc/ghi thanh ghi qua `/dev/modem_*` | T10 | `tasks/logs/t10-ip.log` | Ch. 3.6, 4.3 |
| `iperf3`, RAM trống, 20 lần reboot, chạy 12 giờ | T11 | `docs/test-report.md`, `tasks/logs/t11-*` | Ch. 4.3, 4.5 |
| Mỗi lỗi gặp phải: hiện tượng, nguyên nhân, cách xử lý | mọi task | dòng *Vấn đề gặp & xử lý* + nhật ký | Ch. 4.4 |

## 4. Quy ước viết để dùng lại cho báo cáo

- [quy-trinh-xay-dung.md](quy-trinh-xay-dung.md) viết theo văn phong tài liệu kỹ thuật (không dùng ngôi thứ nhất), mỗi bước có: Mục đích / Đầu vào / Thực hiện / Cơ chế bên dưới / Đầu ra / Kiểm chứng / Tiến độ / Kết quả thực tế / Vấn đề gặp & xử lý.
- Phần **phương pháp tổng quát** (áp dụng mọi board Zynq-7000) tách khỏi phần **áp dụng trên E316** (số liệu, cấu hình cụ thể) để chuyển thẳng vào Ch. 2 và Ch. 3–4.
- Số liệu luôn kèm điều kiện đo (ngày, phiên bản, cấu hình máy) và đường dẫn tới log thô.
- Trích dẫn tài liệu theo mã AMD (UG/PG) kèm phiên bản 2021.1.
