# Simplified NetBSD `ls(1)` in C

## Mục tiêu

Cài đặt một phiên bản rút gọn của `ls(1)` từ đầu bằng C và các API filesystem, theo tập option trong manual NetBSD 10.1 được giao. Chương trình không gọi `system("ls")` và không thực thi chương trình `ls` có sẵn.

## Cấu trúc

- `main.c`: xử lý operand, chia file/thư mục, xử lý đệ quy `-R`.
- `options.c`, `options.h`: phân tích command-line options bằng `getopt()`.
- `filesystem.c`, `filesystem.h`: đọc directory bằng `opendir()`/`readdir()`, lấy metadata bằng `lstat()`.
- `sort.c`, `sort.h`: sắp xếp theo tên, thời gian hoặc kích thước.
- `display.c`, `display.h`: định dạng output, long format, inode, block count, tên owner/group, symlink.
- `Makefile`: build tường minh từ các file nguồn được cho phép; không dùng wildcard `*.c`.
- `tests/`: tạo dữ liệu và so sánh đầu ra với `ls` hệ thống.

## Build và chạy

```sh
make clean
make
./ls
./ls -la tests/testdata
./ls -lni tests/testdata
./ls -R tests/testdata
```

Dùng `make clean` để xóa binary `ls` và các file `.o`. Các file build được loại khỏi Git bằng `.gitignore`.

## Các option trong phạm vi bài

`-A -a -c -d -F -f -h -i -k -l -n -q -R -r -S -s -t -u -w`.

- `-A`, `-a`: chọn các entry ẩn.
- `-c`, `-u`: chọn ctime hoặc atime cho sort/long-format time.
- `-d`, `-R`: hiển thị directory như operand hoặc duyệt đệ quy; option xuất hiện sau cùng quyết định.
- `-F`: thêm ký tự phân loại loại file.
- `-f`, `-r`, `-S`, `-t`: điều khiển sắp xếp.
- `-h`, `-k`, `-s`: điều khiển cách hiển thị block size; `-h` cũng làm dung lượng của `-l` dễ đọc hơn.
- `-i`: inode; `-l`: long format; `-n`: long format với UID/GID dạng số.
- `-q`, `-w`: thay ký tự không in được bằng `?` hoặc in raw; option sau cùng quyết định.

## Kiểm thử

Tạo fixture:

```sh
sh tests/setup_testdata.sh
```

Chạy thủ công các option và so sánh với system `ls`:

```sh
/bin/ls -la tests/testdata
./ls -la tests/testdata
sh tests/compare.sh
```

`tests/compare.sh` báo số test khớp chính xác. Những sai khác về định dạng, timestamp, total hoặc hành vi theo đặc thù NetBSD cần được xem xét trước khi ghi tỷ lệ tương đồng trong báo cáo. Không ghi phần trăm dựa trên dự đoán; dùng kết quả test thực tế.

## Giới hạn cần ghi nhận

Đây là một implementation học tập, không cam kết giống tuyệt đối mọi chi tiết định dạng của NetBSD `ls`. Các khác biệt ở căn cột, cách tính block theo biến môi trường, timestamp biên, special device files hoặc loại filesystem đặc thù cần được đánh giá trên NetBSD mục tiêu.

## Thông tin nộp bài

- Sinh viên: điền tên và mã sinh viên.
- GitHub: thay bằng URL repository thực tế, ví dụ `https://github.com/USERNAME/Name_studentID_midterm`.
- Kết quả test: cập nhật sau khi chạy trên NetBSD.
