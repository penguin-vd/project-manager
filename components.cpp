#include "components.h"

#include <cstdlib>

std::string get_git_status(project &project) {
    chdir(project.path.c_str());
    std::string result = "";
    FILE *pipe = popen("git status -bs", "r");
    if (pipe) {
        char buffer[128];
        while (!feof(pipe)) {
            if (fgets(buffer, 128, pipe) != nullptr) {
                result += buffer;
            }
        }
    }
    return result;
}

void main_menu(sqlite3 *db, std::vector<std::string> &buffer,
               std::vector<project> &projects, int screen_width,
               int screen_height) {
    int y = 0;
    char ch;
    std::vector<int> projects_with_todo;
    std::vector<styling> style_buffer;
    for (size_t i = 0; i < projects.size(); i++) {
        if (!get_todos(db, projects[i]).empty()) {
            projects_with_todo.push_back(projects[i].id);
        }
    }

    while (true) {
        clear_screen();
        int temp_width, temp_height;
        get_console_size(temp_width, temp_height);
        if (temp_width != screen_width || temp_height != screen_height) {
            screen_width = temp_width;
            screen_height = temp_height;
            buffer = std::vector(screen_height, std::string(screen_width, ' '));
        } else {
            clear_buffer(buffer, screen_width);
        }
        style_buffer.clear();

        insert_into_buffer(buffer, 1, 1, "Projects:");
        insert_colored(buffer, style_buffer, 1, 2,
                       "Use arrows to navigate the menu. Press ENTER to open a "
                       "project, and press \'q\' to quit.",
                       "\033[3;36m");

        draw_horizontal_line(buffer, 0, screen_width, 3, '-');
        for (size_t i = 0; i < projects.size(); i++) {
            bool has_todo = false;
            bool is_selected = static_cast<size_t>(y) == i;
            for (size_t x = 0; x < projects_with_todo.size(); x++) {
                if (projects_with_todo[x] == projects[i].id) {
                    has_todo = true;
                    break;
                }
            }

            if (has_todo && is_selected) {
                insert_colored(buffer, style_buffer, 1, i + 4, projects[i].name,
                               "\033[91;7;3m");
            } else if (has_todo) {
                insert_colored(buffer, style_buffer, 1, i + 4, projects[i].name,
                               "\033[91m");
            } else if (is_selected) {
                insert_colored(buffer, style_buffer, 1, i + 4, projects[i].name,
                               "\033[7;3m");

            } else {
                insert_into_buffer(buffer, 1, i + 4, projects[i].name);
            }
        }

        add_border(buffer, screen_width);
        draw_buffer(buffer, style_buffer);
        if (!wait_for_input()) continue;

        ch = getchar();
        if (ch == 27) {
            ch = getchar();
            if (ch != 91) {
                continue;
            }

            ch = getchar();
            switch (ch) {
                case 65:  // UP ARROW
                    if (y == 0) break;
                    y--;
                    break;
                case 66:  // DOWN ARROW
                    if (static_cast<size_t>(y) == projects.size() - 1) break;
                    y++;
                    break;
            }
        } else if (ch == 'q') {
            y = -1;
            break;
        } else if (ch == '\n') {
            project_menu(db, buffer, screen_width, screen_height, projects[y]);
        } else if (ch == 'f') {
            popup(buffer, "Fetching all projects", screen_width);
            clear_screen();
            draw_buffer(buffer, {});

            for (size_t i = 0; i < projects.size(); i++) {
                std::string info = "Fetching: (" + std::to_string(i) + "/" +
                                   std::to_string(projects.size()) + ")";
                popup(buffer, info, screen_width, false);
                chdir(projects[i].path.c_str());
                system("git fetch");
            }

            popup(buffer, "Done fetching all projects", screen_width);
        }
    }
}

void project_menu(sqlite3 *db, std::vector<std::string> &buffer,
                  int screen_width, int screen_height, project &project) {
    int middle = screen_width / 2;
    size_t left_offset = screen_width - middle - middle;
    size_t left_width = middle + left_offset - 1;
    std::vector<std::string> left_buffer(screen_height,
                                         std::string(left_width, ' '));
    std::vector<std::string> right_buffer(screen_height,
                                          std::string(middle, ' '));
    std::vector<styling> left_style_buffer;
    std::vector<styling> right_style_buffer;

    std::string git_status = get_git_status(project);
    std::vector<std::string> git_status_lines;

    size_t p = 0;
    std::string delimiter = "\n";
    std::string token;
    while ((p = git_status.find(delimiter)) != std::string::npos) {
        token = git_status.substr(0, p);
        git_status_lines.push_back(token);
        git_status.erase(0, p + delimiter.length());
    }

    project.id = get_project_id(db, project);
    std::vector<todo> todos = get_todos(db, project);

    const std::vector<fs::path> tree = file_tree(project.path);
    char ch;
    int x = 0;
    int left_y = -1;
    int right_y = 0;
    int tree_offset = 5;
    int info_offset = 0;
    int start_scrolling = 5;
    int tree_starting_index = 0;
    int todo_starting_index = 0;
    size_t todo_height = screen_height - 4 - 9;
    while (true) {
        info_offset = 0;
        int temp_width, temp_height;
        get_console_size(temp_width, temp_height);
        if (temp_width != screen_width || temp_height != screen_height) {
            screen_width = temp_width;
            screen_height = temp_height;
            todo_height = screen_height - 4 - 9;
            middle = screen_width / 2;
            left_offset = screen_width - middle - middle;
            left_width = middle + left_offset - 1;
            left_buffer =
                std::vector(screen_height, std::string(left_width, ' '));
            right_buffer = std::vector(screen_height, std::string(middle, ' '));
            buffer = std::vector(screen_height, std::string(screen_width, ' '));
        } else {
            clear_buffer(buffer, screen_width);
            clear_buffer(left_buffer, left_width);
            clear_buffer(right_buffer, middle);
        }
        left_style_buffer.clear();
        right_style_buffer.clear();

        // LEFT BUFFER
        insert_colored(left_buffer, left_style_buffer, 1, 1, "Project Tree",
                       x == 0 ? "\033[7;1m" : "\033[1m");
        std::string left_info =
            "Navigate using the arrow keys. Press ENTER over a file you wish "
            "to edit, and \'q\' to quit.";
        if (left_info.length() < left_width - 1) {
            insert_colored(left_buffer, left_style_buffer, 1, 2, left_info,
                           "\033[3;36m");
        } else {
            size_t pos = 0;
            while (left_info.size() > pos) {
                std::string info = left_info.substr(pos, left_width - 1);
                insert_colored(left_buffer, left_style_buffer, 1,
                               2 + info_offset, info, "\033[3;36m");
                pos += left_width - 1;
                info_offset++;
            }
            info_offset--;
        }
        draw_horizontal_line(left_buffer, 0, left_width, 3 + info_offset, '-');

        if (buffer.size() - tree_starting_index >
                static_cast<size_t>(screen_height - tree_offset - info_offset -
                                    1) ||
            tree_starting_index > left_y - start_scrolling) {
            if (left_y > start_scrolling) {
                tree_starting_index = left_y - start_scrolling;
            } else if (left_y == start_scrolling) {
                tree_starting_index = 0;
            }
        }

        insert_colored(left_buffer, left_style_buffer, 1, 4 + info_offset,
                       project.name,
                       left_y == -1 && x == 0 ? "\033[7;91m" : "\033[91m");
        add_tree_to_buffer(left_buffer, left_style_buffer, tree, 1,
                           tree_offset + info_offset, project.path.length(),
                           x == 0 ? left_y : -1, tree_starting_index);

        // RIGHT BUFFER
        info_offset = 0;
        insert_colored(right_buffer, right_style_buffer, 1, 1, "Todo List",
                       x == 1 ? "\033[7;1m" : "\033[1m");
        std::string right_info =
            "When focusing this section press \'a\' to add a todo entry and "
            "ENTER to remove an entry.";
        size_t max_info_width = middle - 1;
        if (right_info.size() < max_info_width) {
            insert_colored(right_buffer, right_style_buffer, 1, 2, right_info,
                           "\033[3;36m");
        } else {
            size_t pos = 0;
            while (right_info.size() > pos) {
                std::string info = right_info.substr(pos, max_info_width - 1);
                insert_colored(right_buffer, right_style_buffer, 1,
                               2 + info_offset, info, "\033[3;36m");
                pos += max_info_width - 1;
                info_offset++;
            }
            info_offset--;
        }

        draw_horizontal_line(right_buffer, 0, middle, 3 + info_offset, '-');

        if (buffer.size() - todo_starting_index > todo_height ||
            todo_starting_index > right_y - start_scrolling) {
            if (right_y > start_scrolling) {
                todo_starting_index = right_y - start_scrolling;
            } else if (right_y == start_scrolling) {
                todo_starting_index = 0;
            }
        }

        for (size_t i = 0;
             i + todo_starting_index < todos.size() && i < todo_height; i++) {
            if (i + todo_starting_index == static_cast<size_t>(right_y)) {
                insert_colored(right_buffer, right_style_buffer, 1,
                               4 + info_offset + i,
                               todos[i + todo_starting_index].task,
                               x == 1 ? "\033[7;3m" : "");
            } else {
                insert_into_buffer(right_buffer, 1, 4 + info_offset + i,
                                   todos[i + todo_starting_index].task);
            }
        }

        // GIT STATUS
        insert_colored(right_buffer, right_style_buffer, 1, screen_height - 10,
                       "Git status", "\033[1m");
        draw_horizontal_line(right_buffer, 1, middle, screen_height - 9, '-');

        for (size_t i = 0; i < git_status_lines.size(); i++) {
            if (i == 6 && git_status_lines.size() > i) {
                insert_into_buffer(right_buffer, 1, screen_height - 8 + i,
                                   "...");
                break;
            }
            insert_into_buffer(right_buffer, 1, screen_height - 8 + i,
                               git_status_lines[i]);
        }

        std::vector<styling> style_buffer =
            combine_buffers(buffer, left_buffer, left_style_buffer,
                            right_buffer, right_style_buffer, left_width);
        add_border(buffer, screen_width);

        draw_buffer(buffer, style_buffer);

        if (!wait_for_input()) continue;

        ch = getchar();
        if (ch == 27) {
            ch = getchar();
            if (ch != 91) {
                continue;
            }

            ch = getchar();
            switch (ch) {
                case 65:           // UP ARROW
                    if (x == 0) {  // left buffer movement
                        if (left_y == -1) break;
                        left_y--;
                        break;
                    }  // else right buffer movement
                    if (right_y == 0) break;
                    right_y--;
                    break;
                case 66:           // DOWN ARROW
                    if (x == 0) {  // left buffer movement
                        if (static_cast<size_t>(left_y) == tree.size() - 1)
                            break;
                        left_y++;
                        break;
                    }  // else right button movement
                    if (static_cast<size_t>(right_y) == todos.size() - 1) break;
                    right_y++;
                    break;
                case 67:  // RIGHT ARROW
                    x = 1;
                    break;
                case 68:  // LEFT ARROW
                    x = 0;
                    break;
            }
        } else if (ch == 'q') {
            left_y = -1;
            break;
        } else if (ch == '\n') {
            if (x == 0) {  // left buffer submit
                std::string cmd = "nvim ";
                if (left_y == -1) {
                    cmd += project.path;
                } else {
                    cmd += tree[left_y];
                }
                enable_cursor();
                system(cmd.c_str());
                disable_cursor();
            } else {  // right buffer submit
                if (todos.size() == 0) continue;

                if (choice_popup(buffer, todos[right_y].task, "Delete",
                                 "Cancel", screen_width) == 0) {
                    if (choice_popup(buffer, "Are you sure?", "Yes", "No",
                                     screen_width) == 0) {
                        remove_todo(db, todos[right_y]);
                        todos = get_todos(db, project);
                        if (right_y != 0) right_y--;
                    }
                }
            }
        } else if (ch == 'a' && x == 1) {
            std::string input =
                input_popup(buffer, "Enter a task:", screen_width);
            if (input == "") continue;
            add_todo(db, project, input);
            todos = get_todos(db, project);
        } else if (ch == 'c') {
            exit_program(db, project.path, 0);
        }
    }
}

void popup(std::vector<std::string> &buffer, const std::string message,
           int screen_width, bool has_ok) {
    int width = 32;
    int height = 7;

    std::vector<std::string> center_buffer(height, std::string(width, ' '));
    std::vector<styling> style_buffer = {};
    int start_x = (screen_width / 2) - (width / 2);
    int start_y = (buffer.size() / 2) - (height / 2);

    int middle = width / 2;
    int message_pos = middle - (message.size() / 2);

    insert_colored(center_buffer, style_buffer, message_pos, 2, message,
                   "\033[1;38;5;99m");
    if (has_ok) {
        insert_colored(center_buffer, style_buffer, middle - 1, 4, "Ok",
                       "\033[7;3m");
    }

    add_border(center_buffer, width);

    for (int y = 0; y < height; y++) {
        insert_into_buffer(buffer, start_x, start_y + y, center_buffer[y]);
    }

    for (auto &style : style_buffer) {
        style.x += start_x;
        style.y += start_y;
    }

    clear_screen();
    draw_buffer(buffer, style_buffer);

    if (has_ok) {
        char ch;
        while (true) {
            ch = getchar();
            if (ch == '\n') break;
        }
    }
}

int choice_popup(std::vector<std::string> &buffer, const std::string message,
                 const std::string left, const std::string right,
                 int screen_width) {
    int width = 48;
    int height = 9;
    std::vector<std::string> center_buffer(height, std::string(width, ' '));
    std::vector<styling> style_buffer;
    int start_x = (screen_width / 2) - (width / 2);
    int start_y = (buffer.size() / 2) - (height / 2);

    char ch;
    int choice = 0;

    int middle = width / 2;
    int hh_width = middle / 2;
    int left_pos = hh_width - (left.size() / 2);
    int right_pos = middle + hh_width - (right.size() / 2);
    int task_padding = 16;
    size_t max_width_task = width - task_padding;
    while (true) {
        int temp_width, temp_height;
        get_console_size(temp_width, temp_height);
        if (temp_width != screen_width) {
            screen_width = temp_width;
            buffer = std::vector(temp_height, std::string(screen_width, ' '));
            start_x = (screen_width / 2) - (width / 2);
            start_y = (buffer.size() / 2) - (height / 2);
        }
        clear_buffer(center_buffer, width);
        style_buffer.clear();

        size_t size = message.size();
        if (size < max_width_task) {
            int message_pos = middle - (size / 2);
            insert_colored(center_buffer, style_buffer, message_pos, 2, message,
                           "\033[1;38;5;99m");
        } else {
            size_t pos = 0;
            int task_offset = 0;
            const std::string task = message;
            while (size > pos) {
                std::string info = task.substr(pos, max_width_task);
                int message_pos = middle - (info.size() / 2);
                insert_colored(center_buffer, style_buffer, message_pos,
                               2 + task_offset, info, "\033[1;38;5;99m");
                pos += max_width_task;
                task_offset++;
            }
            task_offset--;
        }

        draw_horizontal_line(center_buffer, 0, width, 4, '-');

        insert_colored(center_buffer, style_buffer, left_pos, 6, left,
                       choice == 0 ? "\033[7;3m" : "");
        insert_colored(center_buffer, style_buffer, right_pos, 6, right,
                       choice == 1 ? "\033[7;3m" : "");

        add_border(center_buffer, width);
        for (int y = 0; y < height; y++) {
            insert_into_buffer(buffer, start_x, start_y + y, center_buffer[y]);
        }

        for (auto &style : style_buffer) {
            style.x += start_x;
            style.y += start_y;
        }

        clear_screen();
        draw_buffer(buffer, style_buffer);

        if (!wait_for_input()) continue;

        ch = getchar();
        if (ch == 27) {
            if (!kbhit()) continue;
            ch = getchar();
            ch = getchar();
            switch (ch) {
                case 67:  // RIGHT ARROW
                    choice = 1;
                    break;
                case 68:  // LEFT ARROW
                    choice = 0;
                    break;
            }
        } else if (ch == 'q') {
            choice = 1;
            break;
        } else if (ch == '\n') {
            break;
        }
    }
    return choice;
}

std::string input_popup(std::vector<std::string> &buffer,
                        const std::string message, int screen_width) {
    enable_cursor();
    int width = 48;
    int height = 10;
    std::vector<std::string> center_buffer(height, std::string(width, ' '));
    std::vector<styling> style_buffer;
    int start_x = (screen_width / 2) - (width / 2);
    int start_y = (buffer.size() / 2) - (height / 2);

    char ch;
    std::string input_buffer;
    std::vector<std::string> input_box(4, std::string(38, ' '));

    int middle = width / 2;
    int message_pos = middle - (message.length() / 2);
    std::string info = "press ESC to quit.";
    int info_pos = middle - (info.length() / 2);
    int input_pos = 6;
    while (true) {
        int temp_width, temp_height;
        get_console_size(temp_width, temp_height);
        if (temp_width != screen_width) {
            screen_width = temp_width;
            buffer = std::vector(temp_height, std::string(screen_width, ' '));
            start_x = (screen_width / 2) - (width / 2);
            start_y = (buffer.size() / 2) - (height / 2);
        }
        clear_buffer(center_buffer, width);
        clear_buffer(input_box, 38);
        style_buffer.clear();

        insert_colored(center_buffer, style_buffer, message_pos, 2, message,
                       "\033[1;38;5;99m");
        insert_colored(center_buffer, style_buffer, info_pos, 3, info,
                       "\033[3;36m");

        bool longer = input_buffer.length() > 36;
        insert_into_buffer(
            input_box, 1, 1,
            input_buffer.substr(0, longer ? 36 : input_buffer.size()));

        if (longer) {
            insert_into_buffer(
                input_box, 1, 2,
                input_buffer.substr(36, input_buffer.size() - 36));
        }

        add_border(input_box, 38);
        for (int y = 0; y < 4; y++) {
            insert_into_buffer(center_buffer, input_pos, 4 + y, input_box[y]);
        }

        add_border(center_buffer, width);

        for (int y = 0; y < height; y++) {
            insert_into_buffer(buffer, start_x, start_y + y, center_buffer[y]);
        }

        for (auto &style : style_buffer) {
            style.x += start_x;
            style.y += start_y;
        }

        clear_screen();
        draw_buffer(buffer, style_buffer);

        int cursor_y = start_y + 4 + 1;
        if (input_buffer.length() > 35) {
            cursor_y++;
        }
        int cursor_x = start_x + input_pos + ((input_buffer.length()) % 36) + 1;

        set_cursor_pos(cursor_x + 1, cursor_y + 1);

        if (!wait_for_input()) continue;

        ch = getchar();
        if (ch == 27) {
            if (!kbhit()) {
                return "";
            }
            ch = getchar();
            if (kbhit()) {
                ch = getchar();
            }
        } else if (ch == 127) {
            if (!input_buffer.empty()) {
                input_buffer.pop_back();
            }
        } else if (ch == '\n') {
            break;
        } else if (ch >= ' ' && ch <= '~') {
            if (input_buffer.length() < 64) {
                input_buffer += ch;
            }
        }
    }
    clear_buffer(buffer, screen_width);
    disable_cursor();
    return input_buffer;
}
