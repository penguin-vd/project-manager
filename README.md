# Project Manager 🗂️

Welcome to a simple **Project Manager** - a program for managing downloaded GitHub projects from the terminal!

## Why a Project Manager? 🤔 

Managing multiple GitHub projects can become cumbersome, especially when working directly from the terminal. This **Project Manager** helps streamline your workflow by:

- Automatically locating all git projects in a specified directory.
- Providing a clear overview of your projects directly from the terminal.
- Allowing you to track and manage todos for each project, ensuring nothing falls through the cracks.

## Features 📋

- **Project Management**: Seamlessly manage your projects with ease.
- **Terminal UI**: Enjoy a clean and intuitive terminal-based user interface.
- **SQLite Integration**: Store and retrieve project data efficiently using SQLite.

## Usage 💻

This tool is tailored for Linux users and has been tested on a Manjaro machine. To get started, follow these steps:

1. **Set Up Your Enviroment**:
   - The program will look for an environment variable called `PROJECTS_DIR`. If not found, it will default to the `HOME` directory.
   - Similarly, the program will look for an environment variable called `PROJECTS_DB`. If not found, it will create a database at the location it is run.

2. **Run the Program**:
   There are two ways to run the program:

   - **Run the Shell Script**:
     ```bash
     sh run.sh
     ```

   - **Use Makefile**:
     ```bash
     make
     build/pm
     ```

## File Structure 📂

- **main.cpp**: Main entry point of the project.
- **database.cpp**: Manages interactions with the SQLite database.
- **ui_library.cpp**: Handles terminal UI rendering and input management.
- **components.cpp**: Contains the core functionality and user interaction logic.

## Demo 🎬 

Here is a preview of the **Project Manager** in action!
![newdemo](https://github.com/penguin-vd/project-manager/assets/62374225/f9591145-f50f-4ebb-821a-e740cbabbb61)
