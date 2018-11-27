/* Read memory of another process

Author: COKPOWEHEU, 27.11.2018

Compile with mingw:
$ i686-w64-mingw32-gcc main.cpp -o prog32.exe -gdwarf-2 -Os -Wall -static-libgcc -lmingw32 -mconsole -mwindows

Флаги запуска:
  -r parh/name.exe : запустить программу и читать ее память
  -t path/name.exe : запустить программу если она еще не запущена (сообщить в stderr о том, была ли запущена)
  -T path/name.exe : запустить программу если она еще не запущена (не сообщать)
  -n name.exe : искать уже запущенную программу по имени
  -p pid : искать запущенную программу по PID (если начинается с 0x то в шестнадцатеричном формате)
  -m [name]:[type]@[addr] : читать переменную типа [type] по адресу [addr] и присвоить ей имя [name]
    [name] может быть пустым
    допустимые типы: i32, u32, i64, u64, f (float), d (double), s (string), x[N], X[N] (массив из N шестнадцатеричных значений)
    Пример: -m str:s@0065fd48 -m i:i32:0065fd5c -m :x4@0065fd5c
  -h : подсказка
  
Avaible flags:
  -r parh/name.exe : run program 'path/name.exe' and read it
  -t path/name.exe : try to find process and run it if failed
  -T path/name.exe : try to find process and run it if failed (with logs to stderr
  -n name.exe : read memory by name
  -p pid : read memory by pid (decimal or 0x - hexadecimal)
  -m [name]:[type]@[addr] : read memory of type [type] at address [addr]
    name shall be empty
    type: i32, u32, i64, u64, f (float), d (double), s (string), x[N], X[N] (array of N hexadecimal values)
    example: -m str:s@0065fd48 -m i:i32:0065fd5c -m :x4@0065fd5c
  -h : this help
 */
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <windows.h>
#include <tlhelp32.h>

typedef struct{
  char name[100];
  char vtype[100];
  void *addr;
}vars_t;

vars_t *vars = NULL;
size_t vars_idx = 0;
size_t vars_count=0;
HANDLE target = 0;

__attribute__((__destructor__)) void free_target(){
  if(target != 0)CloseHandle(target);
}

char alloc_vars(size_t cnt){
  vars_t *temp = (vars_t*)realloc((void*)vars, cnt*sizeof(vars_t));
  if(temp == NULL)return -1;
  vars = temp;
  vars_count = cnt;
  return 0;
}
__attribute__((__destructor__)) void free_vars(){
  if(vars)free(vars);
  vars=NULL;
}

void addvar(size_t idx, char descr[]){
  if(descr[0] == ':'){
    sscanf(descr, ":%99[^@]@%p", vars[idx].vtype, &(vars[idx].addr));
    vars[idx].name[0]=0;
  }else{
    sscanf(descr, "%99[^:]:%99[^@]@%p", vars[idx].name, vars[idx].vtype, &(vars[idx].addr));
  }
}

void start_proc(char name[]){
  char cmd[1000];
  sprintf(cmd, "start %s", name);
  system(cmd);
  Sleep(1000);
}

HANDLE ProcByName( char *name ){
  HANDLE proc_snap;
  PROCESSENTRY32 proc;
  
  proc_snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0); 
  if( proc_snap == INVALID_HANDLE_VALUE ){
    fprintf(stderr, "E: CreateToolhelp32Snapshot\n");
    return 0;
  }
  proc.dwSize = sizeof(PROCESSENTRY32);
  
  int cnt=0;
  do{
    cnt++;
    if( !Process32Next(proc_snap, &proc) ){
      fprintf(stderr, "E: Process [%s] not found from %i\n", name, cnt);
      CloseHandle(proc_snap);
      return 0;
    }
  }while( lstrcmpi( proc.szExeFile, name ) );
  
  CloseHandle( proc_snap );
  return OpenProcess(PROCESS_VM_READ, 0, proc.th32ProcessID);
};

HANDLE ProcByPid(DWORD pid){
  return OpenProcess(PROCESS_VM_READ, 0, pid);
}

#define vtpeq(tp) (strcmp(vars[i].vtype, tp)==0)
char display_vars(HANDLE proc){
  SIZE_T cnt;
  for(size_t i=0; i<vars_idx; i++){
    if(vars[i].name[0]!=0)printf("%s:", vars[i].name);
    if(vtpeq("i32")){
      int32_t val;
      ReadProcessMemory( proc, vars[i].addr, &val, sizeof(val), &cnt );
      printf("%li", (signed long int)val);
    }else if(vtpeq("u32")){
      uint32_t val;
      ReadProcessMemory( proc, vars[i].addr, &val, sizeof(val), &cnt );
      printf("%lu", (unsigned long int)val);
    }else if(vtpeq("i64")){
      int64_t val;
      ReadProcessMemory( proc, vars[i].addr, &val, sizeof(val), &cnt );
      printf("%li", (signed long int)val);
    }else if(vtpeq("u64")){
      uint64_t val;
      ReadProcessMemory( proc, vars[i].addr, &val, sizeof(val), &cnt );
      printf("%lu", (unsigned long int)val);
    }else if(vtpeq("f")){
      float val;
      ReadProcessMemory( proc, vars[i].addr, &val, sizeof(val), &cnt );
      printf("%f", val);
    }else if(vtpeq("d")){
      double val;
      ReadProcessMemory( proc, vars[i].addr, &val, sizeof(val), &cnt );
      printf("%lf", val);
    }else if(vars[i].vtype[0] == 'x' || vars[i].vtype[0] == 'X'){
      unsigned int max;
      unsigned char ch;
      char fmt[] = "%.2  ";
      char *addr = (char*)vars[i].addr;
      fmt[3] = vars[i].vtype[0];
      sscanf(&vars[i].vtype[1], "%u", &max);
      for(;max>0;max--){
        ReadProcessMemory( proc, addr++, &ch, 1, &cnt );
        printf(fmt, ch);
      }
    }else if(vtpeq("s")){
      char ch;
      char *addr = (char*)vars[i].addr;
      do{
        ReadProcessMemory( proc, addr++, &ch, 1, &cnt );
        if(ch != 0)putchar(ch);
      }while(ch != 0);
    }else{
      printf(" unknown type [%s] with address [%p]", vars[i].vtype, vars[i].addr);
    }
    putchar('\n');
  }
  return 1;
}

void help(char name[]){
  printf("Usage: %s [arguments]\n"
         "\t-r path/name.exe : run program 'path/name.exe' and read it\n"
         "\t-t path/name.exe : try to find process and run it if failed\n"
         "\t-T path/name.exe : try to find process and run it if failed (with logs to stderr\n"
         "\t-n name.exe : read memory by name\n"
         "\t-p pid : read memory by pid (decimal or 0x - hexadecimal)\n"
         "\t-m [name]:[type]@[addr] : read memory of type [type] at address [addr]\n"
         "\t\tname shall be empty\n"
         "\t\ttype: i32, u32, i64, u64, f (float), d (double), s (string), x[N], X[N] (array of N hexadecimal values)\n"
         "\t\texample: -m str:s@0065fd48 -m i:i32:0065fd5c -m :x4@0065fd5c\n"
         "\t-h : show this help\n"
         , name );
}

#define argeq(name) (strcmp(argv[i], name)==0)
int main(int argc, char **argv){
  if(argc < 2){help(argv[0]); return 0;}
  alloc_vars(argc); vars_idx=0;
  for(int i=1; i<argc; i++){
    if(argeq("-r")){
      i++;
      if(i>=argc){fprintf(stderr, "Process name does not set\n"); return 0;}
      char *name;
      name = strrchr(argv[i], '/');
      if(name == NULL){
        name = strrchr(argv[i], '\\');
        if(name == NULL)name = argv[i]; //считаем, что запуск из того же каталога
      }
      start_proc( argv[i] );
      target = ProcByName( name+1 );
    }else if(argeq("-t") || argeq("-T")){
      i++;
      if(i>=argc){fprintf(stderr, "Process name does not set\n"); return 0;}
      char *name;
      name = strrchr(argv[i], '/');
      if(name == NULL){
        name = strrchr(argv[i], '\\');
        if(name == NULL)name = argv[i]; //считаем, что запуск из того же каталога
      }
      target = ProcByName( name+1 );
      if(target != 0){
        if(argv[i-1][1]=='T')fprintf(stderr, "Process already running\n");
      }else{
        if(argv[i-1][1]=='T')fprintf(stderr, "Starting process\n");
        start_proc( argv[i] );
      }
      target = ProcByName( name+1 );
    }else if(argeq("-n")){
      i++;
      if(i>=argc){fprintf(stderr, "Process name does not set\n"); return 0;}
      if(target != 0)CloseHandle( target );
      target = ProcByName(argv[i]);
    }else if(argeq("-p")){
      i++;
      if(i>=argc){fprintf(stderr, "Process PID does not set\n"); return 0;}
      if(target != 0)CloseHandle( target );
      DWORD pid;
      if((argv[i][0]=='0' && argv[i][1]=='x') || argv[i][0]=='$'){
        sscanf(&argv[i][2], "%lx", &pid);
      }else{
        sscanf(argv[i], "%lu", &pid);
      }
      target = ProcByPid(pid);
    }else if(argeq("-m")){
      i++;
      if(i>=argc){fprintf(stderr, "variable description does not set\n"); return 0;}
      addvar(vars_idx++, argv[i]);
    }else if(argeq("-h")){
      help(argv[0]); return 0;
    }
  }

  if(target == 0){
    fprintf(stderr, "Can not find process\n");
    return 0;
  }
  
  display_vars(target);
  
  CloseHandle( target );
}
