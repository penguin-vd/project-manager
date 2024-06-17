#include <cstdlib>
#include <stdio.h>
#include <sqlite3.h>
#include <string>
#include <filesystem>
#include <vector>
#include <iostream>
#include <cstdlib>
#include <unistd.h>

namespace fs = std::filesystem;

struct project
{
    int id;
    const std::string name;
    const std::string path;
};

int execute_sql(sqlite3 *db, const char *sql) 
{
    char *err_msg = nullptr;
    int rc = sqlite3_exec(db, sql, 0, 0, &err_msg);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "SQL error: %s\n", err_msg);
        sqlite3_free(err_msg);
        return 1;
    }
    return 0;
}

int setup_database(sqlite3 *db)
{
    int rc;
    const char *projects = "CREATE TABLE IF NOT EXISTS projects ( "
                           "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                           "name TEXT NOT NULL, "
                           "path TEXT UNIQUE "
                           ");";
    
    rc = execute_sql(db, projects);
    if (rc) {
        return rc;
    }

    const char *todos = "CREATE TABLE IF NOT EXISTS todo ( "
                        "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                        "project_id INTEGER NOT NULL, "
                        "task TEXT NOT NULL, "
                        "FOREIGN KEY (project_id) REFERENCES projects (id) "
                        ");";
    rc = execute_sql(db, todos);
    if (rc) {
        return rc;
    }

    return 0;
}

void insert_projects(sqlite3 *db, const std::string name, const std::string path)
{
    int rc;
    const char *sql = "INSERT OR IGNORE INTO projects (name, path) VALUES (?, ?)";
    sqlite3_stmt *stmt;
    rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Failed to prepare statement: %s\n", sqlite3_errmsg(db));
        return;
    }
    
    sqlite3_bind_text(stmt, 1, name.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, path.c_str(), -1, SQLITE_TRANSIENT);

    rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE) {
        fprintf(stderr, "Failed to insert project: %s\n", sqlite3_errmsg(db));
        return;
    }
    sqlite3_finalize(stmt);
}

void locate_projects(sqlite3 *db)
{
    std::string home_dir = std::getenv("HOME");
    if (home_dir.empty()) {
        fprintf(stderr, "HOME directory not found");
        exit(-1);
    }

    fs::path home_path(home_dir + "/Documents");
    for (const auto &entry : fs::recursive_directory_iterator(home_path)) {
        if (entry.is_directory() && entry.path().filename() == ".git") {
            const auto path = entry.path().parent_path();
            insert_projects(db, path.filename(), path);
        }
    }
}

void get_projects(sqlite3 *db, std::vector<project> *projects)
{
    int rc;
    const char *sql = "SELECT id, name, path FROM projects";
    sqlite3_stmt *stmt;
    rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Failed to prepare statement: %s\n", sqlite3_errmsg(db));
        return;
    }

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        int id = sqlite3_column_int(stmt, 0);
        const std::string name = (const char *)sqlite3_column_text(stmt, 1);
        const std::string path = (const char *)sqlite3_column_text(stmt, 2);
        projects->push_back({id, name, path});
    }
}


int main()
{
    sqlite3 *db;
    char *zErrMsg = 0;
    int rc;
    
    rc = sqlite3_open("projects.db", &db);

    if (rc) {
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
        return -1;
    }

    rc = setup_database(db);
    if (rc) {
        fprintf(stderr, "Failed to setup database\n");
        return -1;
    }
    
    locate_projects(db);

    std::vector<project> projects = {};
    get_projects(db, &projects);

    for (int i = 0; i < projects.size(); i++) {
        project p = projects[i];
        fprintf(stderr, "%i - %s\n", i, p.name.c_str());
    }

    int num;
    std::cout << "Enter a index: ";
    if ((std::cin >> num)) {
        if (num < 0 || num >= projects.size()) {
            sqlite3_close(db);
            return 0;
        }
        std::string command = "nvim " + projects[num].path;
        system(command.c_str());
    }


    sqlite3_close(db);
}
