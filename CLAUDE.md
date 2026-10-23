# NCKH_V7 — Linux nhúng trên Zynq-7000 cho modem vệ tinh

Repo tài liệu + layer Yocto để dựng Linux bằng PetaLinux cho Zynq-7000; board ví dụ: ANTSDR E316. Spec: [README.md](README.md). Quy trình chi tiết: [docs/quy-trinh-xay-dung.md](docs/quy-trinh-xay-dung.md). Lộ trình (sơ đồ, cần cài, cần học): [docs/lo-trinh.md](docs/lo-trinh.md). Giải thích PetaLinux cho người mới: [docs/petalinux.md](docs/petalinux.md). Giải thích Yocto (gatesgarth) và đọc `meta-modem`: [docs/yocto.md](docs/yocto.md). Kế hoạch / task / nhật ký: [tasks/](tasks/).

## Phiên bản (chốt 23/10/2026, theo lộ trình học E316)

- Vivado / Vitis / **PetaLinux 2021.1** — không đổi phiên bản giữa chừng; Vivado và PetaLinux phải cùng bản.
- Yocto **gatesgarth**: recipe dùng cú pháp `_append` / `_prepend` (chưa có `:append`), license `GPL-2.0` (chưa có `GPL-2.0-only`), `LAYERSERIES_COMPAT_modem = "gatesgarth"`.
- Kernel `linux-xlnx` 5.10, U-Boot 2021.01. AD9361 cần `meta-adi` nhánh **2021_R1**.
- Máy build: Ubuntu **20.04.6**, 12 luồng, 15 GB RAM, ~419 GB trống, chưa có swap. Vivado 2021.1 ML Standard (chỉ Zynq-7000) cài trên máy build tại `/tools/Xilinx` (xong 23/10, 27 GB; `source /tools/Xilinx/Vivado/2021.1/settings64.sh`); không chạy Vivado và `petalinux-build` cùng lúc.
- Cài PetaLinux tại `/opt/pkg/petalinux/2021.1`; mỗi terminal: `source /opt/pkg/petalinux/2021.1/settings.sh /opt/pkg/petalinux/2021.1`.

## Cấu trúc

- `meta-modem/` — layer Yocto dùng chung: kernel fragment `modem.cfg`, driver `modem-regs`, app `zynq-selftest`.
- `plnx/<board>/` — project PetaLinux (chỉ commit `project-spec/`); snapshot ảnh boot trong `plnx/<board>/deploy/`. Plan 23/10 đi thẳng E316, không dùng ZC702/QEMU.
- `hw/<board>/system.xsa` — phần cứng từ Vivado.
- `_ZynQ_Study/` — lộ trình học E316 (Lab 1–12), **chỉ để tham khảo** (người dùng chốt 23/10): đọc để đối chiếu thông số, **không chép** XSA, project hay image; project này làm từ đầu. Chưa đưa vào git (262 MB, có XSA/BSP nhà cung cấp; repo đang public), đừng `git add`.
- `hw/e316/` — thiết kế Vivado tự làm: `create_project.tcl` (tái lập), `*.xdc`, `system.xsa`; thư mục project Vivado sinh ra không commit.

## Sự thật về E316 (đã kiểm trên board trong lộ trình học)

- `xc7z020clg400-1`, DDR3 `MT41J256M16 RE-125` 32-bit.
- Console UART1 MIO 12–13 (`0xE0001000`) → Linux `ttyPS0`, 115200; petalinux-config chọn `ps7_uart_1`.
- Ethernet: GEM0 EMIO + `gmii_to_rgmii` (FCLK_CLK2 200 MHz), PHY RTL8211F @ MDIO 1, `phy-mode = "rgmii-id"`, tắt `gmii_to_rgmii_0`, converter `gmiitorgmii@8`.
- Thiết kế tham khảo trong lộ trình học có hai XSA (bringup không AD9361; vàng `system_top.xsa` có `axi_ad9361`) — chỉ đọc để đối chiếu, project này tự làm XSA.

## Quy ước làm việc

- Mọi lệnh chạy code dự án (`petalinux-*`, script, qemu, ghi thẻ SD) phải hỏi người dùng trước.
- Trả lời và viết tài liệu bằng tiếng Việt.
- Đổi phiên bản / thông số phần cứng thì sửa đồng bộ README, docs, tasks/plan.md, tasks/todo.md, và **file này**.
- Nhật ký `tasks/nhat-ky.md`: chỉ thêm mục mới theo ngày, không sửa mục cũ (đính chính ghi ở mục mới).
- **Viết để làm báo cáo** (người dùng chốt 23/10): `docs/quy-trinh-xay-dung.md` là nguồn chính cho chương phương pháp/quy trình của báo cáo nghiên cứu; dàn ý ở `docs/bao-cao-dan-y.md`. Xong mỗi task thì điền dòng *Kết quả thực tế* (số liệu + điều kiện đo + đường dẫn log) và *Vấn đề gặp & xử lý* (hiện tượng, nguyên nhân, cách xử lý) của các bước B/R liên quan; lưu log thô vào `tasks/logs/`, hình vào `docs/hinh/`; cập nhật cột “Trạng thái” trong dàn ý. Văn phong tài liệu kỹ thuật, không dùng ngôi thứ nhất.
- **Khi người dùng nói “update”** (người dùng chốt 23/10, áp dụng mọi lần, không cần hỏi lại):
  1. Đồng bộ toàn dự án: rà README, `docs/*`, `tasks/plan.md` + `tasks/todo.md` (tick tiến độ thật), `tasks/nhat-ky.md` (mục ngày hôm nay), `CLAUDE.md`, `meta-modem/` cho khớp nhau (phiên bản, số task, tên layer, đường dẫn, trạng thái).
  2. Không đưa `_ZynQ_Study/` và `graphs/` lên git.
  3. Commit trên nhánh mới → push → mở PR → merge `--rebase` vào `main` → xoá nhánh → `main` local khớp `origin/main`. Tác giả commit `ntdang <nguyendang992002@gmail.com>` (máy chưa có `git config` user).
  4. Báo lại link PR/commit và những gì đã đồng bộ.

## Tự động hóa

- **Graph:** git hook `post-commit` chạy `graphify update .` (chỉ code, không LLM) → `graphify-out/` (gitignored). `.graphifyignore` loại `_ZynQ_Study/`. Tài liệu `.md` thay đổi thì chạy `/graphify . --update` khi cần.
- **CLAUDE.md:** Stop hook (`.claude/hooks/check-claude-md.sh`) nhắc cập nhật file này khi README/docs/tasks/meta-modem mới hơn nó. Nếu thay đổi không ảnh hưởng nội dung ở đây thì `touch CLAUDE.md`.
