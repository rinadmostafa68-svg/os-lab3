/*
 * Template file.
 * You will need to add more code here to execute the command table.
 *
 * NOTE: You are responsible for fixing any bugs this code may have!
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <signal.h>
#include <fcntl.h>
#include <time.h>
#include <glob.h>
#include <wordexp.h>
#include <vector>
#include <string>
#include <cctype>
#include <iostream>
#include <sstream>
#include "command.h"
#include "tokenizer.h"
og termination[cite: 4]
void sigchld_handler(int sig)
{
    int pid;
    while ((pid = waitpid(-1, NULL, WNOHANG)) > 0)
    {
        FILE *logFile = fopen("shell.log", "a");
        if (logFile != NULL)
        {
            time_t now = time(NULL);
            fprintf(logFile, "Child process %d terminated at %s", pid, ctime(&now));
            fclose(logFile);
        }
    }
}

void parse(std::vector<Token> &tokens)  // parse implementation
{
    Command::_currentCommand.clear();

    Command::_currentSimpleCommand = new SimpleCommand();
    Command::_currentCommand.insertSimpleCommand(Command::_currentSimpleCommand);

    size_t i = 0;
    while (i < tokens.size())
    {
        Token token = tokens[i];

        if (token.type == TOKEN_COMMAND || token.type == TOKEN_ARGUMENT)
        {
            Command::_currentSimpleCommand->insertArgument(strdup(token.value.c_str()));
        }
        else if (token.type == TOKEN_PIPE)
        {
            Command::_currentSimpleCommand = new SimpleCommand();
            Command::_currentCommand.insertSimpleCommand(Command::_currentSimpleCommand);
        }
        else if (token.type == TOKEN_REDIRECT)
        {
            i++;
            if (i < tokens.size())
            {
                Command::_currentCommand._outFile = strdup(tokens[i].value.c_str());
            }
        }
        else if (token.type == TOKEN_APPEND)
        {
            i++;
            if (i < tokens.size())
            {
                Command::_currentCommand._outFile = strdup(tokens[i].value.c_str());
                Command::_currentCommand._append = 1;
            }
        }
        else if (token.type == TOKEN_INPUT)
        {
            i++;
            if (i < tokens.size())
            {
                Command::_currentCommand._inputFile = strdup(tokens[i].value.c_str());
            }
        }
        else if (token.type == TOKEN_ERROR)
        {
            i++;
            if (i < tokens.size())
            {
                Command::_currentCommand._errFile = strdup(tokens[i].value.c_str());
            }
        }
        else if (token.type == TOKEN_REDIRECT_AND_ERROR)
        {
            i++;
            if (i < tokens.size())
            {
                Command::_currentCommand._outFile = strdup(tokens[i].value.c_str());
                Command::_currentCommand._errFile = strdup(tokens[i].value.c_str());
                Command::_currentCommand._out_error = 1;
            }
        }
        else if (token.type == TOKEN_BACKGROUND)
        {
            Command::_currentCommand._background = 1;
        }
        else if (token.type == TOKEN_EOF)
        {
            break;
        }

        i++;
    }

    // If the user just pressed Enter or typed an empty line, don't execute
    if (Command::_currentCommand._numberOfSimpleCommands > 0 &&
        Command::_currentCommand._simpleCommands[0]->_numberOfArguments == 0)
    {
        Command::_currentCommand.clear();
        return;
    }

    Command::_currentCommand.execute();
}

SimpleCommand::SimpleCommand()
{
    _numberOfAvailableArguments = 5;
    _numberOfArguments = 0;
    _arguments = (char **)malloc(_numberOfAvailableArguments * sizeof(char *));
}

void SimpleCommand::insertArgument(char *argument) //bonus part
{
    // Check if the argument contains wildcard characters '*' or '?'
    if (strchr(argument, '*') != NULL || strchr(argument, '?') != NULL)
    {
        glob_t glob_result;
        // GLOB_NOCHECK: if no match is found, return the pattern itself
        int ret = glob(argument, GLOB_NOCHECK, NULL, &glob_result);
        if (ret == 0)
        {
            for (size_t i = 0; i < glob_result.gl_pathc; i++)
            {
                if (_numberOfAvailableArguments == _numberOfArguments + 1)
                {
                    _numberOfAvailableArguments *= 2;
                    _arguments = (char **)realloc(_arguments,
                                                  _numberOfAvailableArguments * sizeof(char *));
                }

                _arguments[_numberOfArguments] = strdup(glob_result.gl_pathv[i]);
                _arguments[_numberOfArguments + 1] = NULL;
                _numberOfArguments++;
            }
            globfree(&glob_result);
            free(argument); // Free the original pattern string
            return;
        }
        globfree(&glob_result);
    }

    // Default insertion if no wildcards are present or glob failed
    if (_numberOfAvailableArguments == _numberOfArguments + 1)
    {
        _numberOfAvailableArguments *= 2;
        _arguments = (char **)realloc(_arguments,
                                      _numberOfAvailableArguments * sizeof(char *));
    }

    _arguments[_numberOfArguments] = argument;
    _arguments[_numberOfArguments + 1] = NULL;

    _numberOfArguments++;
}
Command::Command()
{
    _numberOfSimpleCommands = 0;
    _numberOfAvailableSimpleCommands = 1;
    _simpleCommands = (SimpleCommand **)
        malloc(_numberOfAvailableSimpleCommands * sizeof(SimpleCommand *));

    _outFile = 0;
    _inputFile = 0;
    _errFile = 0;
    _out_error = 0;
    _background = 0;
    _append = 0;
}

void Command::insertSimpleCommand(SimpleCommand *simpleCommand)
{
    if (_numberOfAvailableSimpleCommands == _numberOfSimpleCommands)
    {
        _numberOfAvailableSimpleCommands *= 2;
        _simpleCommands = (SimpleCommand **)realloc(_simpleCommands,
                                                    _numberOfAvailableSimpleCommands * sizeof(SimpleCommand *));
    }

    _simpleCommands[_numberOfSimpleCommands] = simpleCommand;
    _numberOfSimpleCommands++;
}

void Command::clear()
{
    for (int i = 0; i < _numberOfSimpleCommands; i++)
    {
        for (int j = 0; j < _simpleCommands[i]->_numberOfArguments; j++)
        {
            free(_simpleCommands[i]->_arguments[j]);
        }

        free(_simpleCommands[i]->_arguments);
        free(_simpleCommands[i]);
    }

    if (_outFile)
    {
        free(_outFile);
    }

    if (_inputFile)
    {
        free(_inputFile);
    }

    if (_errFile)
    {
        free(_errFile);
    }

    _numberOfSimpleCommands = 0;
    _outFile = 0;
    _inputFile = 0;
    _errFile = 0;
    _out_error = 0;
    _background = 0;
    _append = 0;
}

void Command::print()
{
    printf("\n\n");
    printf("              COMMAND TABLE                \n");
    printf("\n");
    printf("  #   Simple Commands\n");
    printf("  --- ----------------------------------------------------------\n");

    for (int i = 0; i < _numberOfSimpleCommands; i++)
    {
        printf("  %-3d ", i);
        for (int j = 0; j < _simpleCommands[i]->_numberOfArguments; j++)
        {
            printf("\"%s\" \t", _simpleCommands[i]->_arguments[j]);
        }
    }

    printf("\n\n");
    printf("  Output       Input        Error        Err&Out       Background\n");
    printf("  ------------ ------------ ------------ ------------ ------------\n");
    printf("  %-12s %-12s %-12s %-12s %-12s\n", _outFile ? _outFile : "default",
           _inputFile ? _inputFile : "default", _errFile ? _errFile : "default", _out_error == 1 ? _errFile : "default"
           ,_background ? "YES" : "NO");
    printf("\n\n");
}

void Command::execute()  // excute implementzation
{
    if (_numberOfSimpleCommands == 0)
    {
        clear();
        return;
    }

    std::string firstCmd = _simpleCommands[0]->_arguments[0];

    if (firstCmd == "exit")
    {
        printf("Good bye!!\n"); //[cite: 4]
        exit(0);                //[cite: 4]
    }

    if (firstCmd == "cd")
    {
        const char *path = (_simpleCommands[0]->_numberOfArguments > 1) 
                           ? _simpleCommands[0]->_arguments[1] 
                           : getenv("HOME"); //[cite: 4]
        if (path && chdir(path) < 0)
        {
            perror("cd failed");
        }
        clear();
        return;
    }

    int defaultin = dup(0);
    int defaultout = dup(1);
    int defaulterr = dup(2);

    int fdin;
    if (_inputFile)
    {
        fdin = open(_inputFile, O_RDONLY);
        if (fdin < 0)
        {
            perror("Error opening input file");
            clear();
            return;
        }
    }
    else
    {
        fdin = dup(defaultin);
    }

    int ret = 0;
    int fdpipe[2];

    for (int i = 0; i < _numberOfSimpleCommands; i++)
    {
        dup2(fdin, 0);
        close(fdin);

        if (i == _numberOfSimpleCommands - 1)
        {
            if (_outFile)
            {
                int fdout;
                if (_append)
                {
                    fdout = open(_outFile, O_WRONLY | O_CREAT | O_APPEND, 0666);
                }
                else
                {
                    fdout = open(_outFile, O_WRONLY | O_CREAT | O_TRUNC, 0666);
                }

                if (fdout < 0)
                {
                    perror("Error opening output file");
                    clear();
                    return;
                }
                dup2(fdout, 1);
                close(fdout);
            }
            else
            {
                dup2(defaultout, 1);
            }

            if (_errFile)
            {
                int fderr;
                if (_out_error)
                {
                    fderr = open(_errFile, O_WRONLY | O_CREAT | O_APPEND, 0666);
                }
                else
                {
                    fderr = open(_errFile, O_WRONLY | O_CREAT | O_TRUNC, 0666);
                }
                dup2(fderr, 2);
                close(fderr);
            }
            else
            {
                dup2(defaulterr, 2);
            }
        }
        else
        {
            if (pipe(fdpipe) < 0)
            {
                perror("pipe error");
                clear();
                return;
            }
            fdin = fdpipe[0];
            dup2(fdpipe[1], 1);
            close(fdpipe[1]);
        }

        ret = fork();
        if (ret == 0)
        {
            // Reset SIGINT to default for child processes so they can be interrupted
            signal(SIGINT, SIG_DFL);

            if (strcmp(_simpleCommands[i]->_arguments[0], "printenv") == 0)
            {
                extern char **environ;
                char **env = environ;
                while (*env)
                {
                    printf("%s\n", *env);
                    env++;
                }
                _exit(0);
            }

            execvp(_simpleCommands[i]->_arguments[0], _simpleCommands[i]->_arguments);
            perror("execvp failed");
            _exit(1);
        }
        else if (ret < 0)
        {
            perror("fork failed");
            clear();
            return;
        }
    }

    dup2(defaultin, 0);
    dup2(defaultout, 1);
    dup2(defaulterr, 2);
    close(defaultin);
    close(defaultout);
    close(defaulterr);

    if (!_background)
    {
        int status;
        waitpid(ret, &status, 0);
    }

    clear();
}

void Command::prompt()
{
    while (true)
    {
        printf("myshell>");
        fflush(stdout);
        std::string input;
        
        if (!std::getline(std::cin, input))
        {
            if (std::cin.eof())
            {
                break;
            }
            std::cin.clear(); // Clear error state if interrupted by SIGINT
            continue;
        }

        std::vector<Token> tokens = tokenize(input);
        parse(tokens);
    }
}

Command Command::_currentCommand;
SimpleCommand *Command::_currentSimpleCommand;

int main()
{
    // Ignore SIGINT (Ctrl+C) in the shell with SA_RESTART[cite: 3, 4]
    struct sigaction sa_int;
    sa_int.sa_handler = SIG_IGN;
    sigemptyset(&sa_int.sa_mask);
    sa_int.sa_flags = SA_RESTART;
    if (sigaction(SIGINT, &sa_int, NULL) < 0)
    {
        perror("sigaction SIGINT error");
        exit(1);
    }

    // Register SIGCHLD handler for process termination logging[cite: 3, 4]
    struct sigaction sa_chld;
    sa_chld.sa_handler = sigchld_handler;
    sigemptyset(&sa_chld.sa_mask);
    sa_chld.sa_flags = SA_RESTART | SA_NOCLDSTOP;
    if (sigaction(SIGCHLD, &sa_chld, NULL) < 0)
    {
        perror("sigaction SIGCHLD error");
        exit(1);
    }

    Command::_currentCommand.prompt();
    return 0;
}