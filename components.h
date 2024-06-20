#ifndef COMPONENTS_H
#define COMPONENTS_H
#include "database.h"
#include "ui_library.h"
#endif

std::string get_git_status(project &project);
void main_menu(sqlite3 *db, std::vector<std::string> &buffer,
               std::vector<project> &projects, int screen_width,
               int screen_height);
void project_menu(sqlite3 *db, std::vector<std::string> &buffer,
                  int screen_width, int screen_height, project &project);
void popup(std::vector<std::string> &buffer, const std::string message,
           int screen_width, bool has_ok = true);
int choice_popup(std::vector<std::string> &buffer, const std::string message,
                 const std::string left, const std::string right,
                 int screen_width);
std::string input_popup(std::vector<std::string> &buffer,
                        const std::string message, int screen_width);
