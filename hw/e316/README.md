# Thiết kế phần cứng E316 (Vivado 2021.1)

Thiết kế Vivado tự làm cho ANTSDR E316, xuất ra `system.xsa` để PetaLinux dùng. Làm theo lát: **T4** chỉ có PS (DDR, UART1, SD0); **T6** thêm Ethernet; **T10** thêm IP AXI-Lite của modem. Thông số đối chiếu với thiết kế tham khảo trong `_ZynQ_Study/` (chỉ đọc).

| File | Vai trò | Commit |
|------|---------|--------|
| `create_project.tcl` | Tái tạo project + block design (xuất bằng *File → Project → Write Tcl*) | Có |
| `system.xsa` | Phần cứng cho PetaLinux | Có |
| `vivado/` | Project Vivado sinh ra | Không (`.gitignore`) |

## 0. XSA mẫu của nhà sản xuất (tham khảo)

Tải từ release GitHub `MicroPhase/antsdr-fw-patch` ngày 23/10/2026, lưu ở `hw/e316/vendor/` (**không commit**: giấy phép chưa rõ, repo public).

| File | Nguồn | Vivado | Part | Dùng được với PetaLinux 2021.1? |
|------|-------|--------|------|-------------------------------|
| `vendor/e316_fw-v0.38_vivado2022.2.xsa` | release `v0.38`, `e316.zip` → `e316/build/system_top.xsa` (18/12/2024) | 2022.2 | `xc7z020clg400-2` | **Không** (XSA mới hơn PetaLinux) |
| `vendor/e310v2_fw-v0.39_vivado2023.2.xsa` | release `v0.39`, `e310v2.zip` (19/12/2024) | 2023.2 | `xc7z020clg400-2` | **Không** |
| `_ZynQ_Study/_ZynQ_Study/_HW/system_top.xsa` | build lại HDL `e310v2` của nhà sản xuất bằng Vivado 2021.1 (giai đoạn học) | 2021.1 | `xc7z020clg400-2` | Có (chỉ để chẩn đoán) |

Cả ba XSA có cùng thông số PS: DDR `MT41J256M16 RE-125`, DQS0 0.048 / Board delay0 0.241, thạch anh 33.333333 MHz, Bank1 LVCMOS 3.3V, UART1 MIO 12..13, SD0 MIO 40..45 — trùng bảng mục 1. Vai trò trong dự án: **đối chiếu thông số và chẩn đoán** (nếu XSA tự làm không boot được thì thử XSA 2021.1 để tách lỗi thiết kế với lỗi board), không dùng làm phần cứng của dự án.

## 1. Bảng thông số PS (T4)

Nguồn đối chiếu: `_ZynQ_Study/_ZynQ_Study/_HW/system_bd.tcl` (dòng ~39–86) và `system.hwh` trong `_HW/system_top.xsa`. Cột “Đặt trong T4” là giá trị dùng cho thiết kế này.

| Nhóm | Thông số (`CONFIG.PCW_…`) | Tham khảo | Đặt trong T4 | Ghi chú |
|------|---------------------------|-----------|--------------|---------|
| Chip | Part | `xc7z020clg400-2` | `xc7z020clg400-2` | Speed grade **-2** (theo mọi XSA của nhà sản xuất); project học V1 dùng -1 vẫn chạy nhưng sai thông số chip |
| I/O | `PRESET_BANK0_VOLTAGE` | LVCMOS 3.3V | LVCMOS 3.3V | |
| I/O | `PRESET_BANK1_VOLTAGE` | LVCMOS 3.3V | LVCMOS 3.3V | Mặc định Vivado là 1.8V, **phải đổi** |
| Clock | `CRYSTAL_PERIPHERAL_FREQMHZ` | 33.333333 | 33.333333 | Thạch anh PS trên board |
| Clock | `APU_PERIPHERAL_FREQMHZ` (CPU) | 666.666666 | 666.666666 | |
| Clock | `UIPARAM_DDR_FREQ_MHZ` | 533.333333 | 533.333333 | |
| Clock | `UART_PERIPHERAL_FREQMHZ` | 100 | 100 | |
| DDR | `UIPARAM_DDR_MEMORY_TYPE` | DDR 3 | DDR 3 | |
| DDR | `UIPARAM_DDR_PARTNO` | MT41J256M16 RE-125 | MT41J256M16 RE-125 | 4 Gbit/chip |
| DDR | `UIPARAM_DDR_BUS_WIDTH` | 32 Bit | 32 Bit | 2 chip → 1 GB (`DDR_RAM_HIGHADDR` = `0x3FFFFFFF`) |
| DDR | `UIPARAM_DDR_USE_INTERNAL_VREF` | 0 | 0 | |
| DDR | `UIPARAM_DDR_TRAIN_WRITE_LEVEL` / `READ_GATE` / `DATA_EYE` | 1 / 1 / 1 | 1 / 1 / 1 | |
| DDR | `UIPARAM_DDR_DQS_TO_CLK_DELAY_0..3` | 0.048 / 0.050 / 0.0 / 0.0 | 0.048 / 0.050 / 0.0 / 0.0 | ns |
| DDR | `UIPARAM_DDR_BOARD_DELAY0..3` | 0.241 / 0.240 / 0.25 / 0.25 | 0.241 / 0.240 / 0.25 / 0.25 | ns |
| UART | `UART1_PERIPHERAL_ENABLE`, `UART1_UART1_IO` | 1, MIO 12 .. 13 | 1, MIO 12 .. 13 | Console, USB Type-C |
| UART | `UART1_BAUD_RATE` | 115200 | 115200 | |
| SD | `SD0_PERIPHERAL_ENABLE`, `SD0_SD0_IO` | 1, MIO 40 .. 45 | 1, MIO 40 .. 45 | Thẻ boot |
| SD | `SD0_GRP_CD_ENABLE` / `WP_ENABLE` / `POW_ENABLE` | 0 / 0 / 0 | 0 / 0 / 0 | Không dùng chân card-detect |
| SD | `SDIO_PERIPHERAL_FREQMHZ` | 50 | 50 | |
| Tắt | `TTC0_PERIPHERAL_ENABLE` | 0 | 0 | |
| Tắt (để lát sau) | ENET0 (EMIO), USB0 (MIO 28..39), I2C0 (MIO 10..11), QSPI (MIO 1..6), SPI0/1 (EMIO), GPIO EMIO | bật | **tắt** | ENET0 → T6; QSPI → T12; còn lại chưa cần |
| PS–PL | `USE_M_AXI_GP0` | 1 | **0** | Chưa có IP trên PL; bật lại ở T10 |

## 2. Hướng dẫn làm bằng giao diện Vivado (T4)

> Cách nhanh (đã dùng cho T4): `cd hw/e316 && vivado -mode batch -source create_project.tcl` làm toàn bộ Bước 2–8. Các bước GUI dưới đây dùng để hiểu thiết kế và chụp hình (Bước 9) — mở `vivado/e316.xpr` sau khi chạy script.

**Trước khi bắt đầu:** không chạy `petalinux-build` song song (máy 15 GB RAM, swap 2 GB).

### Bước 1. Mở Vivado

```sh
source /tools/Xilinx/Vivado/2021.1/settings64.sh
cd ~/repos/NCKH_V7/hw/e316        # file .jou/.log của Vivado sinh ở đây, đã được .gitignore bỏ qua
vivado &
```

### Bước 2. Tạo project

1. **Create Project** → Next.
2. Project name: `e316`; Project location: `/home/ntdang/repos/NCKH_V7/hw/e316/vivado`; bỏ chọn *Create project subdirectory* → Next.
3. **RTL Project**, chọn *Do not specify sources at this time* → Next.
4. Tab **Parts**: ô Search gõ `xc7z020clg400-2`, chọn dòng đó → Next → Finish.

### Bước 3. Tạo block design và thêm PS

1. Bên trái, *IP INTEGRATOR* → **Create Block Design**, tên `system` → OK.
2. Trong khung Diagram bấm **+** (Add IP), gõ `zynq`, chọn **ZYNQ7 Processing System**.
3. Dải xanh phía trên hiện **Run Block Automation** → bấm, **bỏ chọn *Apply Board Preset*** (E316 không có board file) → OK. Vivado tạo 2 cổng ngoài `DDR` và `FIXED_IO`.

### Bước 4. Cấu hình PS (nhấp đúp vào khối `processing_system7_0`)

Đặt đúng theo cột “Đặt trong T4” của bảng mục 1.

**a) Page Navigator → MIO Configuration**
- *Bank 0 I/O Voltage*: **LVCMOS 3.3V**; *Bank 1 I/O Voltage*: **LVCMOS 3.3V**.
- Mở *I/O Peripherals*, **bỏ chọn tất cả** (ENET 0, USB 0, SD 1, UART 0, I2C, SPI, CAN, Quad SPI Flash, GPIO…), rồi chỉ bật:
  - **SD 0**: IO = `MIO 40 .. 45`; *CD*, *WP*, *POW* để trống (không chọn).
  - **UART 1**: IO = `MIO 12 .. 13`.
- Mở *Application Processor Unit*: bỏ chọn **Timer 0** (TTC0).

**b) Clock Configuration**
- *Input Frequency (MHz)*: **33.333333**.
- *Processor/Memory Clocks*: CPU = **666.666666**, DDR = **533.333333**.
- *IO Peripheral Clocks*: UART = **100**, SDIO = **50**.

**c) DDR Configuration**
- *Memory Type*: **DDR 3**; *Memory Part*: **MT41J256M16 RE-125**; *Effective DRAM Bus Width*: **32 Bit**.
- Bỏ chọn *Internal Vref* (nếu có ô này).
- *Training/Board Details*: chọn **Write leveling**, **Read gate**, **Read data eye**; chọn nhập tay (*User input / Enter board delays*) rồi điền:

  | | DQS0 | DQS1 | DQS2 | DQS3 |
  |---|---|---|---|---|
  | DQS to Clock Delay (ns) | 0.048 | 0.050 | 0.0 | 0.0 |
  | Board Delay (ns) | 0.241 | 0.240 | 0.25 | 0.25 |

**d) PS-PL Configuration**
- *General → UART1 Baud Rate*: **115200**.
- *AXI Non Secure Enablement → GP Master AXI Interface*: **bỏ chọn M AXI GP0 interface** (chưa có IP trên PL; nếu để thì Validate báo cổng clock bị treo).

→ **OK**.

### Bước 5. Kiểm tra và tạo wrapper

1. **Validate Design** (F6) → phải báo *Validation successful*. Cảnh báo về cổng `FCLK_CLK0` không nối thì chấp nhận được.
2. Tab **Sources** → chuột phải `system.bd` → **Create HDL Wrapper** → *Let Vivado manage wrapper* → OK.

### Bước 6. Tạo bitstream

1. **Generate Bitstream** (thanh bên trái) → Yes để chạy synthesis + implementation → Jobs: 6 → OK. Mất khoảng 5–15 phút.
2. Xong → hộp thoại hỏi bước tiếp → chọn *View Reports* hoặc Cancel.
3. **Ghi lại cho báo cáo:** thời gian synthesis và implementation (tab *Design Runs*), bảng *Utilization*.

### Bước 7. Xuất XSA

1. **File → Export → Export Hardware** → Next.
2. Chọn **Include bitstream** → Next.
3. XSA file name: `system`; Export to: `/home/ntdang/repos/NCKH_V7/hw/e316` → Next → Finish.

### Bước 8. Lưu script tái tạo project

**File → Project → Write Tcl…** → File: `/home/ntdang/repos/NCKH_V7/hw/e316/create_project.tcl`; chọn *Copy sources to new project* = **không**; *Recreate Block Designs using Tcl* = **có** → OK.

### Bước 9. Chụp hình cho báo cáo

Lưu vào `docs/hinh/`: block design (`t4-block-design.png`), trang MIO Configuration (`t4-mio.png`), trang DDR Configuration (`t4-ddr.png`).

## 3. Kiểm chứng (Claude chạy sau khi bạn báo xong)

```sh
unzip -l hw/e316/system.xsa | grep -E '\.bit|ps7_init|\.hwh'
unzip -p hw/e316/system.xsa system.hwh | grep -oE 'NAME="PCW_(UIPARAM_DDR_[A-Z_0-9]+|UART1_[A-Z_0-9]+|SD0_[A-Z_0-9]+|PRESET_BANK[01]_VOLTAGE|CRYSTAL_PERIPHERAL_FREQMHZ|APU_PERIPHERAL_FREQMHZ)" VALUE="[^"]*"'
```

So từng giá trị với bảng mục 1; khớp hết thì T4 đạt và chuyển sang T5.

## 4. Kết quả thực tế

- **Kết quả:** Vivado 2021.1 batch `create_project.tcl` (24/10/2026, thư mục project trống, `-jobs 6`): tổng **1 phút 55 giây** từ tạo project tới `write_hw_platform` (synth ~14 s, impl + bitstream ~1,5 phút), RAM đỉnh 2,2 GB. Tài nguyên PL: 0 LUT, 0 FF, 0 BRAM, 0 DSP (chỉ PS). `system.xsa` 258 KB gồm `system.bit`, `ps7_init.c/.h/.tcl`, `ps7_init_gpl.*`, `system.hwh`. Đối chiếu `.hwh`: **toàn bộ 38 tham số khớp bảng mục 1** (part `7z020` `-2`, DDR, clock, UART1, SD0, ngoại vi tắt, `DDR_RAM_HIGHADDR` = `0x3FFFFFFF`). Log: `tasks/logs/t4-vivado.log`, `tasks/logs/t4-utilization.rpt`.
- **Tái lập** (chạy lần 2, xoá `vivado/`, 2 phút 2 giây): 7 file `ps7_init*`/`sysdef.xml` giống từng byte, 876 tham số `PCW_*` trong `.hwh` giống hệt, `system.bit` chỉ khác 4 byte (giờ tạo trong header); phần khác chỉ là dấu thời gian (`.hwh`, `xsa.json`, `xsa.xml`) và metadata hiển thị block design (`system.bda`). Log lần 2: `tasks/logs/t4-vivado-run2.log`.
- **Vấn đề gặp & xử lý:** Không có lỗi hay critical warning. 3 warning vô hại: `Boardtcl 53-1` (không đặt board part — E316 không có board file), `BD 5-700`/`BD 5-699` (chưa có address space vì `M_AXI_GP0` tắt). Script viết tay thay cho thao tác GUI + *Write Tcl* để tái lập ngay từ đầu; bitstream có dấu thời gian nên so sánh hai lần build theo nội dung từng file trong `.xsa`, không bằng checksum `.xsa`.
