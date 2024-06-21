#include "ui_library.h"
#include <unistd.h>
#include <filesystem>
#include <fcntl.h>

termios OLDT;
bool HAS_OLDT = false;
const std::string END_STYLE("\033[0m");

void set_cursor_pos(int x, int y) {
    std::cout << "\033[" << y << ";" << x << "H";
}

void clear_screen() { std::cout << "\033[2J\033[1;1H"; }
void enable_cursor() { std::cout << "\033[?25h"; }
void disable_cursor() { std::cout << "\033[?25l"; }

void get_console_size(int &width, int &height) {
    struct winsize w;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
    width = w.ws_col;
    height = w.ws_row;
}

bool kbhit() {
    int flags = fcntl(STDIN_FILENO, F_GETFL, 0);
    fcntl(STDIN_FILENO, F_SETFL, flags | O_NONBLOCK);   
    
    int ch = getchar();

    fcntl(STDIN_FILENO, F_SETFL, flags);
    
    if (ch != EOF) {
        ungetc(ch, stdin);
        return true;
    }

    return false;
}

bool wait_for_input() {
    timeval tv = { 0L, 25000L };
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(STDIN_FILENO, &fds);
    fflush(stdout);
    return select(STDIN_FILENO + 1, &fds, NULL, NULL, &tv) > 0;
}

void draw_buffer(const std::vector<std::string> &buffer,
                 const std::vector<styling> &styling) {
    set_cursor_pos(1, 1);
    for (size_t y = 0; y < buffer.size(); y++) {
        bool has_style = false;
        std::string new_buffer = buffer[y];
        size_t offset = 0;
        for (const auto &style : styling) {
            if (style.y != y) continue;
            has_style = true;
            new_buffer.insert(style.x + offset, style.styling);
            offset += style.styling.size();
        }

        if (!has_style) {
            std::cout << buffer[y];
        } else {
            std::cout << new_buffer;
        }
    }
}

void clear_buffer(std::vector<std::string> &buffer, int width) {
    for (auto &line : buffer) {
        line = std::string(width, ' ');
    }
}

void insert_into_buffer(std::vector<std::string> &buffer, int x, int y,
                        const std::string message) {
    for (size_t i = 0; i < message.length() && x + i < buffer[y].size(); ++i) {
        buffer[y][x + i] = message[i];
    }
}

void insert_colored(std::vector<std::string> &buffer,
                    std::vector<styling> &styling, int x, int y,
                    const std::string message, const std::string colors) {
    insert_into_buffer(buffer, x, y, message);
    styling.push_back({static_cast<size_t>(x), static_cast<size_t>(y), colors});
    styling.push_back({x + message.size(), static_cast<size_t>(y), END_STYLE});
}

std::vector<styling> combine_buffers(std::vector<std::string> &main,
                                     std::vector<std::string> &left,
                                     std::vector<styling> &left_style,
                                     std::vector<std::string> &right,
                                     std::vector<styling> &right_style,
                                     int left_width) {
    for (size_t i = 0; i < main.size(); i++) {
        main[i] = left[i] + "|" + right[i];
    }

    std::vector<styling> styling = left_style;
    for (auto &style : right_style) {
        style.x += left_width + 1;
        styling.push_back(style);
    }

    return styling;
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
                        std::vector<styling> &styling,
                        const std::vector<fs::path> tree, int x, int y,
                        int length, int highlight, int start_index) {
    for (size_t i = start_index; i < tree.size(); i++) {
        const fs::path path = tree[i];
        if (static_cast<size_t>(y) == buffer.size() - 1) break;
        if (i == static_cast<size_t>(highlight)) {
            insert_colored(buffer, styling, x, y, path.string().substr(length),
                           "\033[7;3m");
        } else {
            insert_into_buffer(buffer, x, y, path.string().substr(length));
        }
        y++;
    }
}

std::vector<fs::path> file_tree(const fs::path &path) {
    std::vector<fs::path> tree;
    std::vector<std::string> exclude_elements = {
        ".git", "obj", "bin", ".config", "__pycache__", "node_modules"};
    for (const auto &entry : fs::directory_iterator(path)) {
        bool skip = false;
        for (const auto &excluded : exclude_elements) {
            if (entry.path().filename().string() == excluded) {
                skip = true;
                break;
            }
        }

        if (skip) continue;

        if (entry.is_directory()) {
            if (entry.path().filename().string()[0] != '.' &&
                entry.path().filename().string()[0] != '_') {
                tree.push_back(entry);
                std::vector<fs::path> sub_tree = file_tree(entry.path());
                tree.insert(tree.end(), sub_tree.begin(), sub_tree.end());
            }
        } else {
            tree.push_back(entry);
        }
    }
    return tree;
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
