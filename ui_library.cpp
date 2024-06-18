#include "ui_library.h"
#include <unistd.h>


void set_cursor_pos(int x, int y)
{
    std::cout << "\033[" << y << ";" << x << "H";
}

void clear_screen()
{
    std::cout << "\033[2J\033[1;1H";
}

void get_console_size(int &width, int &height)
{
    struct winsize w;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
    width = w.ws_col;
    height = w.ws_row;
}

void draw_buffer(const std::vector<std::string> &buffer)
{
    set_cursor_pos(1, 1);
    for (size_t i = 0; i < buffer.size(); i++) {
        std::cout << buffer[i];
    }
}

void clear_buffer(std::vector<std::string> &buffer, int width)
{
    for (auto& line : buffer) {
        line = std::string(width, ' ');
    }
}

void insert_into_buffer(std::vector<std::string> &buffer, int x, int y, const std::string message)
{
    for (size_t i = 0; i < message.length(); ++i) {
        buffer[y][x + i] = message[i];
    }
}

int insert_colored(std::vector<std::string> &buffer, int x, int y, const std::string message, const std::string colors)
{
    const std::string end("\033[0m");
    insert_into_buffer(buffer, x, y, colors + message + end);

    for (int i = 0; i < colors.length() + end.length(); i++) {
        buffer[y].push_back(' ');
    }

    return colors.length() + end.length();
}

void combine_buffers(std::vector<std::string> &main, std::vector<std::string> &left, std::vector<std::string> &right)
{
    for (int i = 0; i < main.size(); i++) {
        main[i] = left[i] + "|" + right[i];
    }
}

void add_border(std::vector<std::string> &buffer, int width)
{
    // ONLY USE AFTER ALL THE CHANGES TO THE BUFFER HAVE BEEN MADE
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

void draw_horizontal_line(std::vector<std::string> &buffer, int x1, int x2, int y, char character)
{
    if (x1 > x2) {
        std::swap(x1, x2);
    }

    if (y < 0 || y >= buffer.size()) {
        return;
    }

    for (int x = x1; x <= x2; x++) {
        if (x >= 0 && x < buffer[y].length()) {
            buffer[y][x] = character;
        }
    }
}

void set_raw_mode(termios &oldt)
{
    termios newt;
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
}

void reset_raw_mode(const termios &oldt)
{
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
}
