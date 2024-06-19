# Project Manager

Welcome to a simple **Project Manager** - a program for managing downloaded GitHub projects from the terminal!

## Why a Project Manager?

If you:
- Love the terminal 🖥️
- Need a straightforward tool to manage your GitHub projects efficiently 📈
- Are looking for something that's light and easy to use 🎈

Then this **Project Manager** is the perfect fit for you!

## Features

- **Project Management**: Seamlessly manage your projects with ease.
- **Terminal UI**: Enjoy a clean and intuitive terminal-based user interface.
- **SQLite Integration**: Store and retrieve project data efficiently using SQLite.

## Usage

Currently, this tool is tailored for Linux users and has been tested on a Manjaro machine. 
To get started, you will need a projects directory.
The program will look for an envirioment variable called 'PROJECTS_DIR' if not found it will default to the 'HOME' directory.
The program will also look for an envirioment variable call 'PROJECTS_DB' if not found it will create one at the location it is ran.
For running the program there are 2 ways:

1. **Run the Shell Script**
    ```bash
    sh run.sh
    ```

2. **Use Makefile**
    ```bash
    make
    build/pm
    ```

## File Structure

- **main.cpp**: Contains the core functionality and user interaction logic.
- **database.cpp**: Manages interactions with the SQLite database.
- **ui_library.cpp**: Handles terminal UI rendering and input management.


## Demo

Here is a preview of the **Project Manager** in action!
![demo](https://github.com/penguin-vd/project-manager/assets/62374225/dfcb2298-d370-448d-bd9b-b225941f7e84)
