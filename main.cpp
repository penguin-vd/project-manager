#include "database.h"
#include "ui_library.h"

namespace fs = std::filesystem;

void locate_projects(sqlite3 *db, std::vector<project> *projects) {
    std::string home_dir = std::getenv("HOME");
    if (home_dir.empty()) {
        exit_program(db, "HOME directory not found");
    }

    fs::path home_path(home_dir + "/Documents");
    int id = 0;
    for (const auto &entry : fs::recursive_directory_iterator(home_path)) {
        if (entry.is_directory() && entry.path().filename() == ".git") {
            const auto path = entry.path().parent_path();
            insert_projects(db, path.filename(), path);
            projects->push_back({id, path.filename(), path});
            id++;
        }
    }
}

int main_menu(std::vector<std::string> &buffer, std::vector<project> &projects,
              int screen_width) {
    int y = 0;
    char ch;
    while (true) {
        clear_screen();
        clear_buffer(buffer, screen_width);
        insert_into_buffer(buffer, 1, 1, "Projects:");
        insert_colored(buffer, 1, 2,
                       "Use arrows to navigate the menu. Press ENTER to open a "
                       "project, and press \'q\' to quit.",
                       "\033[3;36m");
        for (size_t i = 0; i < projects.size(); i++) {
            if (static_cast<size_t>(y) == i) {
                insert_colored(buffer, 1, i + 3, projects[i].name, "\033[7;3m");
            } else {
                insert_into_buffer(buffer, 1, i + 3, projects[i].name);
            }
        }

        add_border(buffer, screen_width);
        draw_buffer(buffer);
        set_cursor_pos(2, y + 4);
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
            break;
        }
    }
    return y;
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

int yes_or_no(std::vector<std::string> &buffer, const std::string message,
              int screen_width) {
    int width = 48;
    int height = 8;
    std::vector<std::string> center_buffer(height, std::string(width, ' '));
    int start_x = (screen_width / 2) - (width / 2);
    int start_y = (buffer.size() / 2) - (height / 2);

    char ch;
    int y_or_n = 0;

    int middle = width / 2;
    int hh_width = middle / 2;
    int message_pos = middle - (message.length() / 2);
    int yes_pos = hh_width - 1;
    int no_pos = middle + hh_width - 2;
    while (true) {
        clear_buffer(center_buffer, width);
        int message_space =
            insert_colored(center_buffer, message_pos, 2, message, "\033[7;3m");
        int choice_space = insert_colored(center_buffer, yes_pos, 4, "Yes",
                                          y_or_n == 0 ? "\033[7;3m" : "");
        choice_space += insert_colored(center_buffer, no_pos + choice_space, 4,
                                       "No", y_or_n == 1 ? "\033[7;3m" : "");

        add_border(center_buffer, width);
        for (int y = 0; y < height; y++) {
            insert_into_buffer(buffer, start_x, start_y + y, center_buffer[y]);
        }

        int pos = start_x + width;
        buffer[start_y + 2].insert(pos + message_space,
                                   std::string(message_space, ' '));
        buffer[start_y + 4].insert(pos + choice_space + 1,
                                   std::string(choice_space, ' '));

        clear_screen();
        draw_buffer(buffer);
        ch = getchar();
        if (ch == 27) {
            ch = getchar();
            if (ch != 91) {
                continue;
            }

            ch = getchar();
            switch (ch) {
                case 67:  // RIGHT ARROW
                    y_or_n = 1;
                    break;
                case 68:  // LEFT ARROW
                    y_or_n = 0;
                    break;
            }
        } else if (ch == 'q') {
            y_or_n = 1;
            break;
        } else if (ch == '\n') {
            break;
        }
        buffer[start_y + 2].replace(pos + message_space, message_space, "");
        buffer[start_y + 4].replace(pos + choice_space, choice_space, "");
    }
    clear_buffer(buffer, screen_width);
    return y_or_n;
}

std::string input_popup(std::vector<std::string> &buffer,
                        const std::string message, int screen_width) {
    int width = 48;
    int height = 10;
    std::vector<std::string> center_buffer(height, std::string(width, ' '));
    int start_x = (screen_width / 2) - (width / 2);
    int start_y = (buffer.size() / 2) - (height / 2);

    char ch;
    std::string input_buffer;
    std::vector<std::string> input_box(4, std::string(38, ' '));

    int middle = width / 2;
    int message_pos = middle - (message.length() / 2);
    int input_pos = 6;
    while (true) {
        clear_buffer(center_buffer, width);

        int message_space =
            insert_colored(center_buffer, message_pos, 2, message, "\033[7;3m");

        bool longer = input_buffer.length() > 36;
        insert_into_buffer(
            input_box, 1, 1,
            input_buffer.substr(0, longer ? 36 : input_buffer.length()));

        if (longer) {
            insert_into_buffer(
                input_box, 1, 2,
                input_buffer.substr(36, input_buffer.length() - 36));
        }

        add_border(input_box, 38);
        for (int y = 0; y < 4; y++) {
            insert_into_buffer(center_buffer, input_pos, 4 + y, input_box[y]);
        }

        add_border(center_buffer, width);
        for (int y = 0; y < height; y++) {
            insert_into_buffer(buffer, start_x, start_y + y, center_buffer[y]);
        }

        int pos = start_x + width;
        buffer[start_y + 2].insert(pos + message_space,
                                   std::string(message_space, ' '));

        clear_screen();
        draw_buffer(buffer);

        int cursor_y = start_y + 4 + 1;
        if (longer) {
            cursor_y++;
        }
        int cursor_x = start_x + input_pos + ((input_buffer.length() + 1) % 36);

        set_cursor_pos(cursor_x + 1, cursor_y + 1);
        ch = getchar();
        if (ch == 27) {
            ch = getchar();
            ch = getchar();
        } else if (ch == 127) {
            input_buffer.pop_back();
        } else if (ch == '\n') {
            break;
        } else if (ch >= ' ' && ch <= '~') {
            if (input_buffer.length() < 64) {
                input_buffer += ch;
            }
        }
        buffer[start_y + 2].replace(pos + message_space, message_space, "");
    }
    clear_buffer(buffer, screen_width);
    return input_buffer;
}

void project_menu(std::vector<std::string> &buffer, int screen_width,
                  int screen_height, project &project, sqlite3 *db) {
    int middle = screen_width / 2;
    int left_offset = screen_width - middle - middle;
    int left_width = middle + left_offset - 1;
    std::vector<std::string> left_buffer(screen_height,
                                         std::string(left_width, ' '));
    std::vector<std::string> right_buffer(screen_height,
                                          std::string(middle, ' '));

    std::string git_status = get_git_status(project);
    std::vector<std::string> git_status_lines;

    size_t pos = 0;
    std::string delimiter = "\n";
    std::string token;
    project.id = get_project_id(db, project);
    std::vector<todo> todos = get_todos(db, project);

    while ((pos = git_status.find(delimiter)) != std::string::npos) {
        token = git_status.substr(0, pos);
        git_status_lines.push_back(token);
        git_status.erase(0, pos + delimiter.length());
    }

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
        clear_buffer(buffer, screen_width);
        clear_buffer(left_buffer, left_width);
        clear_buffer(right_buffer, middle);

        // LEFT BUFFER
        insert_colored(left_buffer, 1, 1, "Project Tree",
                       x == 0 ? "\033[7;1m" : "\033[1m");
        std::string left_info =
            "Navigate using the arrow keys. Press ENTER over a file you wish "
            "to edit, and \'q\' to quit.";
        if (left_info.length() < left_width - 1) {
            insert_colored(left_buffer, 1, 2, left_info, "\033[3;36m",
                           left_width - 1 - left_info.size());
        } else {
            size_t pos = 0;
            while (left_info.size() > pos) {
                std::string info = left_info.substr(pos, left_width - 1);
                insert_colored(left_buffer, 1, 2 + info_offset, info,
                               "\033[3;36m", left_width - 1 - info.size());
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

        insert_colored(left_buffer, 1, 4 + info_offset, project.name,
                       left_y == -1 && x == 0 ? "\033[7;91m" : "\033[91m");
        add_tree_to_buffer(left_buffer, tree, 1, tree_offset + info_offset,
                           project.path.length(), x == 0 ? left_y : -1,
                           tree_starting_index);

        // RIGHT BUFFER
        info_offset = 0;
        insert_colored(right_buffer, 1, 1, "Todo List",
                       x == 1 ? "\033[7;1m" : "\033[1m");
        std::string right_info =
            "When focusing this section press \'a\' to add a todo entry and "
            "ENTER to remove an entry.";
        int max_info_width = middle - 1;
        if (right_info.size() < max_info_width) {
            insert_colored(right_buffer, 1, 2, right_info, "\033[3;36m",
                           max_info_width - right_info.size());
        } else {
            size_t pos = 0;
            while (right_info.size() > pos) {
                std::string info = right_info.substr(pos, max_info_width - 1);
                insert_colored(right_buffer, 1, 2 + info_offset, info,
                               "\033[3;36m", max_info_width - info.size());
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
            if (i == static_cast<size_t>(right_y)) {
                insert_colored(right_buffer, 1, 4 + info_offset + i,
                               todos[i + todo_starting_index].task,
                               x == 1 ? "\033[7;3m" : "");
            } else {
                insert_into_buffer(right_buffer, 1, 4 + info_offset + i,
                                   todos[i + todo_starting_index].task);
            }
        }

        // GIT STATUS
        insert_colored(right_buffer, 1, screen_height - 10, "Git status",
                       "\033[1m");
        draw_horizontal_line(right_buffer, 1, middle, screen_height - 9, '-');

        for (size_t i = 0; i < git_status_lines.size(); i++) {
            if (i == 7 && git_status_lines.size() > 7) {
                insert_into_buffer(right_buffer, 1, screen_height - 8 + i,
                                   "...");
                break;
            }
            insert_into_buffer(right_buffer, 1, screen_height - 8 + i,
                               git_status_lines[i]);
        }

        combine_buffers(buffer, left_buffer, right_buffer);
        add_border(buffer, screen_width);

        draw_buffer(buffer);
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

                system(cmd.c_str());
            } else {  // right buffer submit
                if (todos.size() == 0) continue;

                int res = yes_or_no(buffer, "Are you sure?", screen_width);
                if (res == 0) {
                    remove_todo(db, todos[right_y]);
                    todos = get_todos(db, project);
                    if (right_y != 0) right_y--;
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

int main() {
    sqlite3 *db;
    int rc;

    rc = sqlite3_open("projects.db", &db);

    if (rc) {
        std::cout << "Can't open database: " << sqlite3_errmsg(db) << std::endl;
        return -1;
    }

    setup_database(db);

    std::vector<project> projects = {};
    locate_projects(db, &projects);

    set_raw_mode();

    int screen_width, screen_height;
    get_console_size(screen_width, screen_height);
    clear_screen();

    std::vector<std::string> buffer(screen_height,
                                    std::string(screen_width, ' '));
    clear_buffer(buffer, screen_width);
    while (true) {
        int index = main_menu(buffer, projects, screen_width);

        if (index == -1) {
            break;
        }
        project_menu(buffer, screen_width, screen_height, projects[index], db);
    }
    clear_screen();
    reset_raw_mode();
    sqlite3_close(db);
}
