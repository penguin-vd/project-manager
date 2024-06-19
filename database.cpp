#include "database.h"

#include <iostream>

#include "ui_library.h"

void exit_program(sqlite3 *db, const std::string err_msg, int exitcode) {
    clear_screen();
    sqlite3_close(db);
    reset_raw_mode();
    set_cursor_pos(1, 1);
    std::cout << err_msg << std::endl;
    exit(exitcode);
}

int setup_database(sqlite3 *db) {
    int rc;
    const char *projects =
        "CREATE TABLE IF NOT EXISTS projects ( "
        "id INTEGER PRIMARY KEY AUTOINCREMENT, "
        "name TEXT NOT NULL, "
        "path TEXT UNIQUE "
        ");";

    rc = execute_simple_sql(db, projects);
    if (rc) {
        std::string err_msg = "Error: ";
        err_msg += sqlite3_errmsg(db);
        exit_program(db, err_msg);
    }

    const char *todos =
        "CREATE TABLE IF NOT EXISTS todos ( "
        "id INTEGER PRIMARY KEY AUTOINCREMENT, "
        "project_id INTEGER NOT NULL, "
        "task TEXT NOT NULL, "
        "FOREIGN KEY (project_id) REFERENCES projects (id) "
        ");";
    rc = execute_simple_sql(db, todos);
    if (rc) {
        std::string err_msg = "Error: ";
        err_msg += sqlite3_errmsg(db);
        exit_program(db, err_msg);
    }

    return 0;
}

int execute_simple_sql(sqlite3 *db, const char *sql) {
    int rc = sqlite3_exec(db, sql, 0, 0, NULL);
    if (rc != SQLITE_OK) {
        std::string err_msg = "Error: ";
        err_msg += sqlite3_errmsg(db);
        exit_program(db, err_msg);
    }
    return 0;
}

int insert_projects(sqlite3 *db, const std::string name,
                    const std::string path) {
    int rc;
    const char *sql =
        "INSERT OR IGNORE INTO projects (name, path) VALUES (?, ?)";
    sqlite3_stmt *stmt;
    rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        std::string err_msg = "Error: ";
        err_msg += sqlite3_errmsg(db);
        exit_program(db, err_msg);
    }

    sqlite3_bind_text(stmt, 1, name.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, path.c_str(), -1, SQLITE_TRANSIENT);

    rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE) {
        std::string err_msg = "Error: ";
        err_msg += sqlite3_errmsg(db);
        exit_program(db, err_msg);
    }
    sqlite3_finalize(stmt);
    return 0;
}

int get_project_id(sqlite3 *db, project &project) {
    int rc;
    const std::string sql =
        "SELECT id from projects WHERE path=\'" + project.path + "\'";
    sqlite3_stmt *stmt;
    rc = sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        std::string err_msg = "Error: ";
        err_msg += sqlite3_errmsg(db);
        exit_program(db, err_msg);
    }

    int id;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        id = sqlite3_column_int(stmt, 0);
    }

    return id;
}

std::vector<todo> get_todos(sqlite3 *db, const project project) {
    std::vector<todo> todos;
    int rc;
    const std::string sql = "SELECT id, task FROM todos WHERE project_id=" +
                            std::to_string(project.id);
    sqlite3_stmt *stmt;
    rc = sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        std::string err_msg = "Error: ";
        err_msg += sqlite3_errmsg(db);
        exit_program(db, err_msg);
    }

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        int id = sqlite3_column_int(stmt, 0);
        const std::string task = (const char *)sqlite3_column_text(stmt, 1);
        todos.push_back({id, project.id, task});
    }

    return todos;
}

void add_todo(sqlite3 *db, const project project, const std::string task) {
    int rc;
    const char *sql = "INSERT INTO todos (project_id, task) VALUES (?, ?)";
    sqlite3_stmt *stmt;
    rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        std::string err_msg = "Error: ";
        err_msg += sqlite3_errmsg(db);
        exit_program(db, err_msg);
    }

    sqlite3_bind_int(stmt, 1, project.id);
    sqlite3_bind_text(stmt, 2, task.c_str(), -1, SQLITE_TRANSIENT);

    rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE) {
        std::string err_msg = "Error: ";
        err_msg += sqlite3_errmsg(db);
        exit_program(db, err_msg);
    }
    sqlite3_finalize(stmt);
}

void remove_todo(sqlite3 *db, const todo todo) {
    int rc;
    const std::string sql =
        "DELETE FROM todos WHERE id=" + std::to_string(todo.id);
    sqlite3_stmt *stmt;
    rc = sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        std::string err_msg = "Error: ";
        err_msg += sqlite3_errmsg(db);
        exit_program(db, err_msg);
    }

    rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE) {
        std::string err_msg = "Error: ";
        err_msg += sqlite3_errmsg(db);
        exit_program(db, err_msg);
    }

    sqlite3_finalize(stmt);
}
