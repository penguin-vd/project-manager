#include "components.h"

namespace fs = std::filesystem;

void locate_projects(sqlite3 *db, std::vector<project> *projects) {
    std::string project_dir = std::getenv("PROJECTS_DIR");

    if (project_dir.empty()) {
        std::string home_dir = std::getenv("HOME");
        if (home_dir.empty()) {
            exit_program(db, "HOME directory not found");
        }
        project_dir = home_dir;
    }

    fs::path home_path(project_dir);
    for (const auto &entry : fs::recursive_directory_iterator(home_path)) {
        if (entry.is_directory() && entry.path().filename() == ".git") {
            const auto path = entry.path().parent_path();
            insert_projects(db, path.filename(), path);
            project project = {-1, path.filename(), path};
            project.id = get_project_id(db, project);
            projects->push_back(project);
        }
    }
}

int main() {
    sqlite3 *db;
    int rc;

    std::string db_location = std::getenv("PROJECTS_DB");
    if (db_location.empty()) {
        db_location = "projects.db";
    }

    rc = sqlite3_open(db_location.c_str(), &db);

    if (rc) {
        std::cout << "Can't open database: " << sqlite3_errmsg(db) << std::endl;
        return -1;
    }

    disable_cursor();
    setup_database(db);

    std::vector<project> projects = {};
    locate_projects(db, &projects);

    set_raw_mode();

    int screen_width, screen_height;
    get_console_size(screen_width, screen_height);

    std::vector<std::string> buffer(screen_height,
                                    std::string(screen_width, ' '));

    main_menu(db, buffer, projects, screen_width, screen_height);

    enable_cursor();
    clear_screen();
    reset_raw_mode();
    sqlite3_close(db);
}
