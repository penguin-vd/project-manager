#include "ui_library.h"

#include <unistd.h>

#include <filesystem>

termios OLDT;
bool HAS_OLDT = false;

void set_cursor_pos(int x, int y) {
    std::cout << "\033[" << y << ";" << x << "H";
}

void clear_screen() { std::cout << "\033[2J\033[1;1H"; }

void get_console_size(int &width, int &height) {
    struct winsize w;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
    width = w.ws_col;
    height = w.ws_row;
}

void draw_buffer(const std::vector<std::string> &buffer) {
    set_cursor_pos(1, 1);
    for (size_t i = 0; i < buffer.size(); i++) {
        std::cout << buffer[i];
    }
}

void clear_buffer(std::vector<std::string> &buffer, int width) {
    for (auto &line : buffer) {
        line = std::string(width, ' ');
    }
}

void insert_into_buffer(std::vector<std::string> &buffer, int x, int y,
                        const std::string message, bool allow_overflow) {
    size_t i;
    for (i = 0; i < message.length() && x + i < buffer[y].size(); ++i) {
        buffer[y][x + i] = message[i];
    }

    if (i < message.length() && allow_overflow) {
        for (; i < message.length(); ++i) {
            buffer[y].push_back(message[i]);
        }
    }
}

int insert_colored(std::vector<std::string> &buffer, int x, int y,
                   const std::string message, const std::string colors,
                   int diff) {
    const std::string end("\033[0m");
    insert_into_buffer(buffer, x, y, colors + message + end, true);
    if (diff == -1) {
        for (size_t i = 0; i < colors.length() + end.length(); i++) {
            buffer[y].push_back(' ');
        }
    } else {
        for (size_t i = 0; i < static_cast<size_t>(diff) &&
                           i < colors.length() + end.length();
             i++) {
            buffer[y].push_back(' ');
        }
    }

    return colors.length() + end.length();
}

void combine_buffers(std::vector<std::string> &main,
                     std::vector<std::string> &left,
                     std::vector<std::string> &right) {
    for (size_t i = 0; i < main.size(); i++) {
        main[i] = left[i] + "|" + right[i];
    }
}

void add_border(std::vector<std::string> &buffer, int width) {
    for (size_t i = 0; i < buffer.size(); i++) {
        if (i == 0) {
            buffer[i] = "";
            for (int x = 0; x < width; x++) {
                if (x == 0) {
                    buffer[i] += "+";
                } else if (x == width - 1) {
                    buffer[i] += "+";
                } else {
                    buffer[i] += "-";
                }
            }
        } else if (i == buffer.size() - 1) {
            buffer[i] = "";
            for (int x = 0; x < width; x++) {
                if (x == 0) {
                    buffer[i] += "+";
                } else if (x == width - 1) {
                    buffer[i] += "+";
                } else {
                    buffer[i] += "-";
                }
            }
        } else {
            buffer[i].replace(0, 1, "|");
            buffer[i].replace(buffer[i].length() - 1, 1, "|");
        }
    }
}

void draw_horizontal_line(std::vector<std::string> &buffer, int x1, int x2,
                          int y, char character) {
    if (x1 > x2) {
        std::swap(x1, x2);
    }

    if (y < 0 || static_cast<size_t>(y) >= buffer.size()) {
        return;
    }

    for (int x = x1; x <= x2; x++) {
        if (x >= 0 && static_cast<size_t>(x) < buffer[y].length()) {
            buffer[y][x] = character;
        }
    }
}

void add_tree_to_buffer(std::vector<std::string> &buffer,
                        const std::vector<fs::path> tree, int x, int y,
                        int length, int highlight, int start_index) {
    for (size_t i = start_index; i < tree.size(); i++) {
        const fs::path path = tree[i];
        if (static_cast<size_t>(y) == buffer.size() - 1) break;
        if (i == static_cast<size_t>(highlight)) {
            insert_colored(buffer, x, y, path.string().substr(length),
                           "\033[7;3m");
        } else {
            insert_into_buffer(buffer, x, y, path.string().substr(length));
        }
        y++;
    }
}

void set_raw_mode() {
    termios newt;
    tcgetattr(STDIN_FILENO, &OLDT);
    newt = OLDT;
    newt.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    HAS_OLDT = true;
}

void reset_raw_mode() {
    if (HAS_OLDT) {
        tcsetattr(STDIN_FILENO, TCSANOW, &OLDT);
    }
}
