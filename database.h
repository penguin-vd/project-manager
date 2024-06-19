#ifndef DATABASE_H
#define DATABASE_H
#include <sqlite3.h>
#include <string>
#include <vector>
#endif

struct project
{
    int id;
    const std::string name;
    const std::string path;
};

struct todo
{
    int id;
    int project_id;
    const std::string task;
};

int setup_database(sqlite3 *db);
int execute_simple_sql(sqlite3 *db, const char *sql);
int insert_projects(sqlite3 *db, const std::string name, const std::string path);
int get_project_id(sqlite3 *db, project &project);
std::vector<todo> get_todos(sqlite3 *db, const project project);
void add_todo(sqlite3 *db, const project project, const std::string task);
void remove_todo(sqlite3 *db, const todo todo);
void exit_program(sqlite3 *db, const std::string err_msg, int exit_code = 1);
