#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

typedef int (*func)(void);

int mk_sharedlib(char* func_file, char* mk_lib_file);
int calculate_expression();

int main(int argc, char* argv[]) {
  static char line[4096];     // input
  char dir[40] = ".//tem//";  // dir to store files
  int len = strlen(dir);
  char* mk_sharedlib_file =
      ".//mk_sharedlib.sh";  // Script for compiling shared libraries

  while (1) {
    printf("crepl> ");
    fflush(stdout);
    if (!fgets(line, sizeof(line), stdin)) {
      break;
    }
    // judge input
    if (strstr(line, "int") == line) {
      // The input is a function
      char tmpline[50];
      char* func_name;
      // Get input function name to name file and function
      strncpy(tmpline, line, 50);
      strtok(tmpline, " {(");
      func_name = strtok(NULL, " {(");

      if (func_name != NULL) {
        strcat(dir, func_name);
        strcat(dir, ".c");
        FILE* fp = fopen(dir, "w");
        fputs(line, fp);  // input func into file
        fclose(fp);
        fp = NULL;
        int judeg = mk_sharedlib(dir, mk_sharedlib_file);
        dir[len] = '\0';
        if (judeg == -1) {
          continue;
        }
        printf("Got %zu chars.\n", strlen(line));
      } else {
        printf("NO FUNCNAME!\n");
        continue;
      }
      
    } else {
      // The input is an expression
      // Wrap the expression as a function and write to a file
      char* TmpfileName = ".//tem//wrapper_0.c";
      FILE* fp = fopen(TmpfileName, "w");
      fprintf(fp, "int %s(){return %s;}", "Tmp_wrapper", line);
      fclose(fp);
      fp = NULL;
      int judge2 = mk_sharedlib(TmpfileName, mk_sharedlib_file);
      if (judge2 == -1) {
        continue;
      }
      int output = calculate_expression();
      printf("%d\n", output);
      remove(TmpfileName);
    }
  }
  char* ar[] = {NULL};
  execvp(".//rm.sh", ar);
}

int mk_sharedlib(char* func_file, char* mk_lib_file) {
  pid_t fid = fork();
  int status;
  char* arg[] = {NULL};
  if (fid < 0) {
    printf("FunctionInputError\n");
    return -1;
  } else if (fid == 0) {
    // Compile the shared library
    execvp(mk_lib_file, arg);
    exit(1);
  } else if (fid > 0) {
    wait(&status);
    if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
      // printf("Child process exec() success!\n");
    } else {
      printf("Grammatical errors\n");
      remove(func_file);
      return -1;
    }
  }
  return 0;
}

int calculate_expression() {
  int output;
  void* handle = dlopen(".//sharedlib.so", RTLD_LAZY);
  if (!handle) {
    printf("LinkError\n");
    dlclose(handle);
    return 0;
  }
  func tem = (func)dlsym(handle, "Tmp_wrapper");
  if (tem == NULL) {
    printf("FuncError\n");
    return 0;
  }
  output = tem();
  dlclose(handle);
  return output;
}