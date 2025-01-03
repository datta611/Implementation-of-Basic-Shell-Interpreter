# Enhanced Shell Interpreter

## Overview

This project implements a feature-rich shell interpreter in C as part of an Operating Systems course. It replicates many functions of a typical Unix shell, including command execution, pipelines, redirection, job control, and command history.

## Features

- Execute standalone and pipelined commands.
- Built-in commands:
  - `cd`: Change directory.
  - `history`: Display previously executed commands.
  - `jobs`: List running background jobs.
  - `kill`: Terminate a specific job.
- Background job execution (`&`).
- Input/output redirection (`<` and `>`).
- Signal handling for `Ctrl+C` and `Ctrl+Z`.

## How to Compile and Run

1. Clone the repository:

   ```bash
   git clone https://github.com/your-username/enhanced-shell.git
   cd enhanced-shell
   ```

2. Compile the code:

   ```bash
   gcc shell.c -o shell
   ```

3. Run the shell:
   ```bash
   ./shell
   ```

## Example Commands

- Execute a command:
  ```bash
  ls -la
  ```
- Use pipelines:
  ```bash
  ls | grep "test" | wc -l
  ```
- Redirect input/output:
  ```bash
  cat < input.txt > output.txt
  ```
- Run background jobs:
  ```bash
  sleep 10 &
  ```
- View history:
  ```bash
  history
  ```
- List background jobs:
  ```bash
  jobs
  ```
- Terminate a job:
  ```bash
  kill <job-id>
  ```

## Project Structure

- `shell.c`: Main source code.
- `README.md`: Project documentation.
- `.gitignore`: Files and directories to exclude from the repository.

## License

This project is licensed under the MIT License.
