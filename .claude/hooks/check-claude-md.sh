#!/bin/sh
# Stop hook: nhắc cập nhật CLAUDE.md khi tài liệu/layer chính mới hơn nó.
input=$(cat)
# Đã nhắc một lần trong lượt này thì thôi, tránh lặp vô hạn.
echo "$input" | grep -q '"stop_hook_active": *true' && exit 0
cd "$CLAUDE_PROJECT_DIR" 2>/dev/null || cd "$(dirname "$0")/../.." || exit 0
[ -f CLAUDE.md ] || exit 0
changed=$(find README.md docs tasks meta-modem -type f -newer CLAUDE.md 2>/dev/null | head -5 | tr '\n' ' ')
[ -z "$changed" ] && exit 0
printf '{"decision":"block","reason":"Các file sau mới hơn CLAUDE.md: %s. Cập nhật CLAUDE.md nếu phiên bản, cấu trúc, thông số E316 hoặc quy ước đã đổi; nếu không ảnh hưởng thì chạy: touch CLAUDE.md"}\n' "$changed"
