# Unix Shell

Simple Unix Shell program written for CSC 360 at the University of Victoria. Achieved a grade of 100%.

This assignment was focused on gaining experience with process creation and management. As such this shell implementation reads user commands and spawns processes to execute them, along with supporting output redirection for the result of these commands and piping of the output between commands. 

This program is written in C99. A text file titled '.sh360rc' must be contained in it’s directory. The first line of '.sh360rc' should contain the prompt given to the user for input, and the following lines must contain absolute paths to directories containing the binaries for commands the shell is expected to execute, with each path on its own line.  

The core loop of the program resides in the Main method. Here we prompt for and parse user input. 

The function getPath() takes a user command and searches the directories in '.sh360rc' for the executable binary of this command. 

The function exec() executes a command if no output redirection or piping is needed. exec() calls getPath() to determine the location of the command, then forks into a child process to execute it. 

The function execOR() facilitates output redirection.

execPP() and setPipes() facilitate piping between processes.


