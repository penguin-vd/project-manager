#ifndef UI_LIB_H
#define UI_LIB_H
#include <iostream>
#include <vector>
#include <string>
#include <sys/ioctl.h>
#include <unistd.h>
#include <termios.h>
#include <filesystem>
#endif

namespace fs = std::filesystem;

void set_cursor_pos(int x, int y);
void clear_screen();
void get_console_size(int &width, int &height);
void draw_buffer(const std::vector<std::string> &buffer);
void clear_buffer(std::vector<std::string> &buffer, int width);
void insert_into_buffer(std::vector<std::string> &buffer, int x, int y, const std::string message, bool allow_overflow = false);
int insert_colored(std::vector<std::string> &buffer, int x, int y, const std::string message, const std::string colors, int diff = -1);
void combine_buffers(std::vector<std::string> &main, std::vector<std::string> &left, std::vector<std::string> &right);
void add_border(std::vector<std::string> &buffer, int width);
void draw_horizontal_line(std::vector<std::string> &buffer, int x1, int x2, int y, char character);
void add_tree_to_buffer(std::vector<std::string> &buffer, const std::vector<fs::path> tree,
        int x, int y, int length, int highlight, int start_index);
void set_raw_mode();
void reset_raw_mode();
