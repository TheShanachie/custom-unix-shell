#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <errno.h>
#include <stdbool.h>

// Define global constants
#define MAXLINELENGTH 1024
#define MAXARGNUM 128
#define MAXARGLEN 128
#define MAXPATHNUM 32
#define MAXPATHSIZE 512

// Define strings constants
#define QUERYSTR "lsh> "

// Define mode constants
#define SCRIPTMODE 2
#define BATCHMODE 1
#define INTERACTIVEMODE 0

// Define global variables
// For program path(s)
int program_path_count = 0;
char *program_paths[MAXPATHNUM];
char cwd[MAXPATHSIZE];

// For stream directive.
int mode;
FILE *in_stream;
FILE *out_stream;

// Funtion Defenitions (This may not be the right name for this 'procedure')
// TODO Comment and order these functions.
// TODO Switch return values to 'bool' where possible.
// TODO Create more functionality to register validity of path, permissions, and permissions variables/modes.
int close_input();
int set_input_mode(int argc, char *argv[]);
int parse_input_line(char *array[], FILE *stream);
int sub_parse(const char *input, char *del[], char *array[], int *index);
void print_error_message();
void print_query_message();
int get_current_working_directory();
int get_user_input(char *array[], FILE *stream);
char *validate_path(char *cmd);
int register_built_in_commands(int argc, char *argv[]);
const char *get_file_suffix(const char *path);
void register_arguments(int argc, char *argv[]);
int validate_input_format(int argc, char *argv[]);
int validate_io_redirect_format(int argc, char *argv[]);
int execute_programs(int argc, char *argv[]);
void clean_memory(int argc, char *argv[]);

// Program Main.
int main(int argc, char *argv[])
{
  // Make sure the right number of args are passed.
  if (argc > 2)
  {
    print_error_message();
    return 1;
  }

  // Set the input mode.
  if (set_input_mode(argc, argv) == 0)
  {
    print_error_message();
    return 1;
  }

  // Set the default program paths and the path count.
  program_paths[0] = strdup("");
  program_paths[1] = strdup("/bin/");
  program_paths[2] = strdup("/usr/bin/");
  program_path_count = 3;

  // Get the current working directory
  if (get_current_working_directory() == 0)
    return 0;

  // Open an event loop.
  while (1)
  {
    // Variables for retrieving arguments.
    char *array[MAXARGNUM];

    // Check for end-of-file.
    if (feof(in_stream))
    {
      // Deallocate the old program paths.
      for (int i = 0; i < program_path_count; i++)
      {
        free(program_paths[i]);
      }

      // exit the program.
      close_input();
      exit(0);
    }

    // Get next command input.
    int new_argc = get_user_input(array, in_stream);

    // register argument values.
    register_arguments(new_argc, array);

    // clear alloced memory for arguments
    for (int i = 0; i < new_argc; i++)
    {
      free(array[i]);
    }
  }

  // Clean alloced memory before exiting.
  // Deallocate the old program paths.
  for (int i = 0; i < program_path_count; i++)
  {
    free(program_paths[i]);
  }

  close_input();
  return 0;
}

/**
 * Set the input mode and stream for the program.
 *
 * Input:
 *    int argc: the number of arguments passed to main.
 *    char *argv[]: the arguments passed to main.
 *
 * Output:
 *    1 - If the input mode and stream were opened correctly.
 *    0 - If there was an error.
 */
int set_input_mode(int argc, char *argv[])
{
  // Set the mode
  mode = argc - 1;

  // Get the input stream.
  if (mode == INTERACTIVEMODE)
  {
    in_stream = stdin;
  }
  if (mode == BATCHMODE)
  {
    in_stream = fopen(argv[1], "r");
    if (in_stream == NULL)
    {
      return 0;
    }
  }
  return 1;
}

/**
 * If the input stream was opened at the beginning of the program, then it is closed.
 */
int close_input()
{
  if (mode == BATCHMODE)
    fclose(in_stream);
}

/**
 * Given an array for input arguments, this will parse next line in the provided file stream.
 *
 * Input:
 *    char *array[]: a fixed size (MAXLINELENGTH) array of character strings.
 *    FILE *restrict stream: the stream to read an input line from.
 *
 * Output:
 *    If the input line is read correctly, then the function returns a int type number
 *    of the arguments (Strings delimited by ' ', '\n', '\t', '\r'). Otherwise, the function
 *    returns -1, if there was an error.
 */
int parse_input_line(char *array[], FILE *stream)
{
  //  Variables for read.
  char *line = NULL;
  size_t len = 0;
  ssize_t nread;

  // Get the input line.
  if ((nread = getline(&line, &len, stream)) == -1)
  {
    free(line);
    if (!feof(stream))
    {
      // On failure.
      return -1;
    }
    else
    {
      // EOF, continue and wait for other check.
      return 0;
    }
  }
  else if (nread == 0)
  {
    free(line);
    return 0;
  }
  // Start the string parser.
  char *in_Ptr = line;
  char *o_Ptr;
  int i = 0;

  // Store each seperated string in the array.
  while ((o_Ptr = strsep(&in_Ptr, " \r\n\t")) != NULL)
  {
    // check that we have a valid number of arguments.
    if (i + 1 >= MAXARGNUM)
      return 0; // Failure.

    // update the new argument.
    if ((strcmp(o_Ptr, "") != 0) && (strlen(o_Ptr) + 1 <= MAXARGLEN))
    {
      // Add the next valid (non-empty) string.
      // Parse by these values.
      char *del[2];
      del[0] = strdup(">");
      del[1] = strdup("&");
      del[2] = NULL;

      // Add parsed values.
      int parse_res = sub_parse(o_Ptr, del, array, &i);

      // Free values used for parsing.
      free(del[0]);
      free(del[1]);

      // Check if parse failed.
      if (parse_res == 0)
        return 0; // Failure.
    }
  }

  // End the array with a empty character.
  array[i] = NULL;

  free(line); // Free alloced line.
  return i;   // Success.
}

/**
 * This function should parse a command string, delimited by one or more strings length 1. 
 * It will keep all delimeter characters as command strings as well. It will update an 
 * index and add the parsed values to the command argument array provided.
 *
 * Input:
 *    const char* input: Is a null terminated string.
 *    char *del[]: Is a list of null terminated delimiter strings of length 1.
 *    char *array[]: Is the array of command arguments to add to.
 *    int *index: Is a pointer to the index of the most recent command arg in array.
 *
 * Output:
 *    0 - if there was an error parsing the input string.
 *    1 - if the parsing method ran successfully.
 */
int sub_parse(const char *input, char *del[], char *array[], int *index)
{
  const char *token = input;
  char *delimiter = NULL;

  // Create the delim array.
  int min_ind = strlen(input);
  while (*token != '\0')
  {
    // Get the next delimiter.
    for (int i = 0; del[i] != NULL; i++)
    {
      const char *s = strstr(token, del[i]);
      size_t len = s - token;
      if (len < min_ind)
      {
        min_ind = len;
        delimiter = del[i];
      }
    }

    // check that we have a valid number of arguments.
    if (*index + 1 >= MAXARGNUM)
      return 0; // Failure.

    // Parse again.
    const char *end = NULL;
    if (delimiter != NULL)
    {
      end = strstr(token, delimiter);
    }

    // This is the last string. Add the string.
    if (end == NULL)
    {
      if (strcmp(token, "") != 0)
      {
        array[(*index)++] = strdup(token);
      }
      break;
    }

    // Add and continue parsing.
    size_t length = end - token;
    if (strncmp(token, "", length) != 0)
    {
      array[(*index)++] = strndup(token, length);
    }

    // Add the delimiter as a string.
    array[(*index)++] = strdup(delimiter);

    token = end + 1;
  }

  // Return success.
  return *index;
}

/**
 * This prints the error message for the program.
 */
void print_error_message()
{
  // Print error message.
  char error_message[30] = "An error has occurred\n";
  write(STDERR_FILENO, error_message, strlen(error_message));
}

/**
 * This prints the query message for the program.
 */
void print_query_message()
{
  // Print query message.
  if (mode == INTERACTIVEMODE)
    printf("\"%s\" - %s", cwd, QUERYSTR);
}

/**
 * This sets the current working diretory variable.
 *
 * Output:
 *    0 - if there was an error.
 *    1 - if the current working directory was set.
 */
int get_current_working_directory()
{
  // Set the current working directory.
  if (getcwd(cwd, sizeof(cwd)) != NULL)
  {
    // printf("Current working dir: \"%s\"\n", cwd); // Comment out in final program.
  }
  else
  {
    // There is an error retrieving the current working directory.
    print_error_message();
    return 0;
  }
  return 1;
}

/**
 * This will query the user for input string. The number of arguments given in the input string is returned.
 * If there is an error reading the input string, then -1 is returned. The arguments are stored int the array
 * parameter provided to the function.
 *
 * Input:
 *    char *array[]: a fixed size (MAXLINELENGTH) array of character strings.
 *    FILE *restrict stream: the stream to read an input line from.
 *
 * Output:
 *    If the input line is read correctly, then the function returns a int type number
 *    of the arguments (Strings delimited by ' ', '\n', '\t', '\r'). Otherwise, the function
 *    returns -1, if there was an error.
 */
int get_user_input(char *array[], FILE *stream)
{
  // Query the user.
  print_query_message();
  int argc = parse_input_line(array, stream);
  if (argc >= 0)
  { // Successful Query.
    return argc;
  }
  else
  {
    // Unsuccessful Query.
    return -1;
  }
}

/**
 * For any command, this will find if the command itself is executable or if there is an executable in the specified paths.
 * If the function finds that the command is a valid executable, then the function allocs space for a new return string.
 * This value should be freed after it is not needed.
 *
 * Input:
 *    char *cmd: a character string for executing an executable program.
 *
 * Output:
 *    an integer value
 *      If there is a valid executable within the path variables, return the relative path the executable.
 *      If the argument is not an executable command in the path locations, return NULL.
 */
char *validate_path(char *cmd)
{
  // For each path.
  for (int path_number = 0; path_number < program_path_count; path_number++)
  {
    // Variables.
    char *ans;
    int fd;

    // Create a temporary command string.
    char *temp_cmd = (char *)malloc(sizeof(char) * (MAXARGLEN + strlen(program_paths[path_number]) + 1));

    // Fill the temp command with the supposed command string.
    int index = 0;
    index += sprintf(&temp_cmd[index], "%s", program_paths[path_number]);

    // make sure the string is formatted correctly.
    char *temp = program_paths[path_number];
    if (strlen(temp) > 0 && temp[(strlen(temp) - 1)] != '/')
    {
      // Append an extra character to the path
      index += sprintf(&temp_cmd[index], "%s", "/");
    }

    // Append the cmd string.
    index += sprintf(&temp_cmd[index], "%s", cmd);

    // Check whether the command string can be accessed at this path.
    fd = access(temp_cmd, X_OK);
    if (fd != -1) // There was no error accessing the executable.
    {
      // If the command string is valid, return it.
      return temp_cmd;
    }
    else
    {
      // Otherwise, free the allocated memory.
      free(temp_cmd);
    }
  }

  // If none of the possibilites are executable.
  return NULL;
}

/**
 * This function checks whether the exit command is called and valid. If the exit command is valid, this function exits the program completely.
 * A valid call will only have one argument.
 *
 * Input:
 *    int argc: The count of argumemts passed to the program by the input string.
 *    char *array[]: a fixed size (MAXLINELENGTH) array of character strings which hold the input arguments.
 *
 * Output:
 *    An integer value -
 *      0 - if the argument passed to the function was not of the built-in arguments.
 *     -1 - if an error has occured, such as an invalid number of arguments.
 */
int register_exit_command(int argc, char *argv[])
{
  // If a valid 'exit' command has been called.
  if (strcmp(argv[0], "exit") == 0)
  {
    if (argc == 1)
    {
      // Clear all alloced memory.
      clean_memory(argc, argv);
      close_input();
      exit(0);
    }
    else
      return -1;
  }
  return 0; // Nothing happens.
}

/**
 * This function checks whether the cd command is called and valid. The arguments are valid if the first argument is
 * cd and the second is a path to change directories. A valid call will only have two arguments. If a valid call is
 * received, this function will change directories to the give path.
 *
 * All paths specified must be relative to the current directory.
 *
 * Input:
 *    int argc: The count of argumemts passed to the program by the input string.
 *    char *array[]: a fixed size (MAXLINELENGTH) array of character strings which hold the input arguments.
 *
 * Output:
 *    An integer value -
 *      0 - if the argument passed to the function was not of the built-in arguments.
 *      1 - if the argument passed to the funtion was valid.
 *     -1 - if an error has occured, such as an invalid number of arguments.
 */
int register_cd_command(int argc, char *argv[])
{
  // If a valid 'cd' command has been called.
  if (strcmp(argv[0], "cd") == 0) // The 'cd' command was called.
  {
    if (argc - 1 == 1) // The proper number of arguments were used.
    {
      // Make sure the string is in a valid format.
      int index = 0;
      char *temp_path = (char *)malloc(sizeof(char) * (strlen("./") + strlen(argv[1]) + 1));
      index += sprintf(&temp_path[index], "%s", "./");
      index += sprintf(&temp_path[index], "%s", argv[1]);

      if (chdir(temp_path) != 0)
      {
        free(temp_path);
        return -1;
      }
      else
      {
        free(temp_path);
        return 1;
      }
    }
    else
      return -1;
  }
  return 0; // Nothing happens.
}

/**
 * This function checks whether the path command is called and valid. The arguments are valid if the first argument is
 * path and any number of following arguments are give. A valid call must have greater than or equal to one arguments.
 * This function will remove any older path variables stored in the program and replace them with the arguments given
 * after the first.
 *
 * Input:
 *    int argc: The count of argumemts passed to the program by the input string.
 *    char *array[]: a fixed size (MAXLINELENGTH) array of character strings which hold the input arguments.
 *
 * Output:
 *    An integer value -
 *      0 - if the argument passed to the function was not of the built-in arguments.
 *      1 - if the argument passed to the funtion was valid.
 *     -1 - if an error has occured, such as an invalid number of arguments.
 */
int register_path_command(int argc, char *argv[])
{
  // If a valid 'path' command has been called.
  if (strcmp(argv[0], "path") == 0) // The 'path' command was called.
  {
    if (argc - 1 >= 0) // The proper number of arguments were used.
    {
      // Deallocate the old paths.
      for (int i = 0; i < program_path_count; i++)
      {
        free(program_paths[i]);
      }

      // The count get zero, since there are no paths at this point.
      program_path_count = 0;

      // Add the new program paths to the array.
      for (int i = 1; i < argc; i++)
      {
        program_paths[program_path_count++] = strdup(argv[i]);
      }

      // Success
      return 1;
    }
    else
      return -1;
  }
  return 0; // Nothing happens.
}

/**
 * This function takes the input arguments and checks if they are in a valid form of the built-in commands. If so, and they are in a
 * valid argument structure, the program will execute the command. The function will not mutate any variables given to it.
 *
 * Input:
 *    int argc: The count of argumemts passed to the program by the input string. This will be -1, if an error has occured previously.
 *    char *array[]: a fixed size (MAXLINELENGTH) array of character strings which hold the input arguments.
 *
 * Output:
 *    An integer value -
 *      0 - if the argument passed to the function was not of the built-in arguments.
 *      1 - if the arguments passed to the function did follow the built-in argument structure and the function was executed properly.
 *     -1 - if an error has occured.
 */
int register_built_in_commands(int argc, char *argv[])
{
  if (argc == -1)
    return argc; // If an error occured previously.

  int cmdVal; // Store the value that is returned by the checks.

  // If a valid 'exit' command has been called.
  cmdVal = register_exit_command(argc, argv);
  if (cmdVal != 0)
  {
    return cmdVal;
  }

  // If a valid 'cd' command has been called.
  cmdVal = register_cd_command(argc, argv);
  if (cmdVal != 0)
  {
    get_current_working_directory(); // Reset the current working directory.
    return cmdVal;
  }

  // If a valid 'path' command has been called.
  cmdVal = register_path_command(argc, argv);
  if (cmdVal != 0)
  {
    return cmdVal;
  }

  // No valid command.
  return 0;
}

/**
 * Register the arguments given to this program and proceed to the appropriate task.
 *
 * Input:
 *    int argc: the number of arguments given to the command line.
 *    char *argv[]: an array of the arguments passed to this program.
 */
void register_arguments(int argc, char *argv[])
{
  if (argc < 0)
  {
    // There was an error.
    print_error_message();
    return;
  }
  else if (argc == 0)
  {
    // There were no arguments passed.
    return;
  }
  else
  {
    // Try to check and register built in commands.
    int result = register_built_in_commands(argc, argv);
    if (result != 0)
    {
      if (result == -1)
        print_error_message(); // There was an error checking/registering built in commands.
      return;                  // The commands were executed.
    }

    // Check if the program(s) is executable.
    if (validate_input_format(argc, argv) == -1)
    {
      print_error_message();
      return;
    }
    else
    {
      // Execute the program calls.
      execute_programs(argc, argv);
    }

    return;
  }
}

/**
 * This function will parse input delimited by '&'. This function will check the internal paths for an executable
 * binary for each call of greater than or equal to one arguments delimited by the '&'. If there is a valid path,
 * the program continues. Otherwise, the function halts and returns. A valid input must be delimited by '&'.
 * There should be no empty calls between or after the delimiter. Otherwise, the function returns an error result.
 * Additionally, the function validates whether or not the proper standards for redirecting output are used.
 *
 * Input:
 *    int argc: the number of arguments given to command line.
 *    char *argv[]: an array of the arguments passed to this program.
 *
 * Output:
 *    0 - If all program calls were valid and have an executable path.
 *   -1 - If any program calls are invalid.
 */
int validate_input_format(int argc, char *argv[])
{
  // Create variables.
  int cnt = 0, current_cnt = 0;

  while (cnt < argc + 1)
  {
    // Check if the current portion is delimited.
    if (argv[cnt] == NULL || strcmp(argv[cnt], "&") == 0)
    {
      if (current_cnt > 0) // There are more than 0 arguments.
      {
        // Attempt to find the binary.
        char *fpath = validate_path(argv[cnt - current_cnt]);

        // Check if it worked.
        if (fpath == NULL)
        {
          return -1;
        }
        else
        {
          free(fpath);
        }

        // Check that there is a valid io redirect format.
        if (validate_io_redirect_format(current_cnt, &argv[cnt - current_cnt]) == -1)
        {
          return -1;
        }

        // Reset the current counts.
        current_cnt = 0;
      }
    }
    else
    {
      current_cnt++;
    }
    cnt++;
  }

  return 0;
}

/**
 * This function will take a sequence of program arguments and determine if
 * the arguments follow a valid IO redirect format.
 *
 * Input:
 *    int argc: the number of arguments given to command line.
 *    char *argv[]: an array of the arguments passed to this program.
 *
 * Output:
 *    1 - If no arguments were passed to the function.
 *    1 - If all program calls use a valid IO redirect format.
 *   -1 - If any program calls are invalid.
 */
int validate_io_redirect_format(int argc, char *argv[])
{
  // Variables to validate the redirecting strings.
  int cnt = 0;
  int ans = 0;

  // Iterate through a portion of the array.
  while (argv[cnt] != NULL && strcmp(argv[cnt], "&") != 0)
  {
    // If any argument besides arguments n-1 is a valid redirect strings.
    if (strcmp(argv[cnt], ">") == 0)
    {
      if (cnt == argc - 2 && cnt > 0)
      {
        // The format is valid.
        ans = 1;
      }
      else
      {
        return -1; // The format is incorrect.
      }
    }

    // increment count.
    ++cnt;
  }

  // The format is correct.
  return ans;
}

/**
 * This function executes a single process with the given arguments and argument count. 
 * It will handle the IO redirect if one is specified. This function assumes that the
 * provided arguments are valid and in a valid format. If an error occurs, this process
 * will exit immediatly and IO redirection will be unresolved. Thus, the state is 
 * uncertain in that case.
 * 
 * Input:
 *    int argc: the number of arguments given to command line.
 *    char *argv[]: an array of the arguments passed to this program.
 */
void execute_process(int argc, char *argv[])
{
  char *temp;
  int save_out;
  int out;

  // Check if a redirect is necessary.
  int redir = validate_io_redirect_format(argc, argv);

  if (redir == 1) // If an output file is provided.
  {
    // Open the file for output.
    out = open(argv[argc - 1], O_RDWR | O_CREAT | O_TRUNC, 0600);
    if (-1 == out) // There was an error opening the file.
    {
      exit(0); // Check that this is the correct return value.
    }

    save_out = dup(fileno(stdout));
    if (-1 == dup2(out, fileno(stdout)))
    {
      // Something went wrong.
      exit(0); // Check that this is the correct return value.
    }

    // Lastly, restrict the arguments.
    temp = argv[argc - 2];
    argv[argc - 2] = NULL;
  }

  // Run the process here, given the correct arguments.
  char *fpath = validate_path(argv[0]); // Should not return NULL at this point.
  execv(fpath, argv);                   // Execute the process.
  free(fpath);

  // End the process.
  if (redir == 1)
  {
    argv[argc - 2] = temp;
    fflush(stdout);
    close(out);
    dup2(save_out, fileno(stdout));
    close(save_out);
  }
  exit(0);
}

/**
 * This function will parse any programs delimited by the '&'. This function will fork and run the programs in parallel.
 * This function assumes that all programs delimited by the '&' are valid calls. If no arguments are passed between
 * delimiters, then the function skip that call.
 *
 * Input:
 *    int argc: the number of arguments given to command line.
 *    char *argv[]: an array of the arguments passed to this program.
 *
 * Output:
 *    1 - Always returns 1.
 *
 */
int execute_programs(int argc, char *argv[])
{
  int n = 0;
  int cnt = 0;
  int current_cnt = 0;

  while (cnt < argc + 1)
  {
    if (argv[cnt] == NULL || strcmp(argv[cnt], "&") == 0)
    {
      // Block the array.
      if (argv[cnt] != NULL)
      {
        free(argv[cnt]);
        argv[cnt] = NULL;
      }

      if (current_cnt > 0) // There is an adequate number of arguments.
      {
        // Start child process.
        pid_t rc;
        if ((rc = fork()) < 0)
        {
          // There was an error.
          abort();
        }
        else if (rc == 0)
        {
          // Run the i'th process.
          execute_process(current_cnt, &argv[cnt - current_cnt]);
        }
        else
        {
          // Update variables.
          n++;             // Number of programs grows.
          current_cnt = 0; // Number of current arguments goes back to zero.
        }
      }
    }
    else
    {
      current_cnt++; // Increment number of current arguments.
    }

    cnt++; // Increment count;
  }

  /* Wait for children to exit. */
  int status;
  pid_t pid;
  while (n > 0)
  {
    pid = wait(&status);
    // To show the state of child processes.
    // printf("Child with PID %ld exited with status 0x%x.\n", (long)pid, status);
    --n;
  }

  // return;
  return 0;
}

/**
 * Free all program allocated memory.
 * Should be called when exiting the program.
 *
 * Input:
 *    int argc: the number of arguments given command line.
 *    char *argv[]: an array of the arguments passed to this program.
 */
void clean_memory(int argc, char *argv[])
{
  // Deallocate the old program paths.
  for (int i = 0; i < program_path_count; i++)
  {
    if (program_paths[i] != NULL)
    {
      free(program_paths[i]);
    }
  }

  // clear alloced memory for arguments
  for (int i = 0; i < argc; i++)
  {
    free(argv[i]);
  }
}