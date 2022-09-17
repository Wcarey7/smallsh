# smallsh
A shell written in C with a subset of features of well-known shells, such as bash.


&ndash; Provides a prompt for running commands  
&ndash; Handles blank lines and comments, which are lines beginning with the `#` character  
&ndash; Provides expansion for the variable `$$`  
&ndash; Executes 3 commands `exit`, `cd`, and `status` via code built into the shell  
&ndash; Executes other commands by creating new processes using a function from the `exec` family of functions  
&ndash; Supports input and output redirection  
&ndash; Supports running commands in foreground and background processes  
&ndash; Implements custom handlers for 2 signals, `SIGINT` and `SIGTSTP`  
