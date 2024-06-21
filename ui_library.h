#ifndef UI_LIB_H
#define UI_LIB_H
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>

#include <filesystem>
#include <iostream>
#include <string>
#include <vector>
#endif

namespace fs = std::filesystem;

struct styling {
    size_t x;
    size_t y;
    const std::string styling;
};

void set_cursor_pos(int x, int y);
void clear_screen();
void enable_cursor();
void disable_cursor();
void get_console_size(int &width, int &height);
bool wait_for_input();
bool kbhit();
void draw_buffer(const std::vector<std::string> &buffer,
                 const std::vector<styling> &styling);
void clear_buffer(std::vector<std::string> &buffer, int width);
void insert_into_buffer(std::vector<std::string> &buffer, int x, int y,
                        const std::string message);
void insert_colored(std::vector<std::string> &buffer,
                    std::vector<styling> &styling, int x, int y,
                    const std::string message, const std::string colors);
std::vector<styling> combine_buffers(std::vector<std::string> &main,
                                     std::vector<std::string> &left,
                                     std::vector<styling> &left_style,
                                     std::vector<std::string> &right,
                                     std::vector<styling> &right_style,
                                     int left_width);
void add_border(std::vector<std::string> &buffer, int width);
void draw_horizontal_line(std::vector<std::string> &buffer, int x1, int x2,
                          int y, char character);
void add_tree_to_buffer(std::vector<std::string> &buffer,
                        std::vector<styling> &styling,
                        const std::vector<fs::path> tree, int x, int y,
                        int length, int highlight, int start_index);
std::vector<fs::path> file_tree(const fs::path &path);
void set_raw_mode();
void reset_raw_mode();
