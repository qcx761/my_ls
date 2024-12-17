#include <stdio.h>         // printf(), perror()
#include <stdlib.h>        // exit(), malloc(), free()
#include <string.h>        // strlen(), strcpy(), strcat(), strcmp()
#include <unistd.h>        // read(), write(), close(), lseek()
#include <fcntl.h>         // open(), O_RDONLY, O_WRONLY, O_CREAT
#include <sys/types.h>     // off_t, mode_t, uid_t, etc.
#include <sys/stat.h>      // stat(), lstat(), chmod(), S_IFDIR, etc.
#include <dirent.h>        // opendir(), readdir(), closedir()
#include <time.h>          // localtime(), strftime(), ctime()
#include <pwd.h>           // getpwuid()
#include <grp.h>           // getgrgid()
#include <errno.h>         // errno, perror
#include <linux/limits.h>  // PATH_MAX
#include<libgen.h>         // basename()
#include <sys/ioctl.h>      //获取终端长度


// 文件类型的颜色代码
#define RESET   "\033[0m"
#define BLUE    "\033[34m"
#define GREEN   "\033[32m"
#define CYAN    "\033[36m"
#define MAGENTA "\033[35m"
#define YELLOW  "\033[33m"

// 定义结构体（命令行参数）
typedef struct{
    int show_all;
    int long_format;
    int recursive;
    int sort_by_time;
    int reverse;
    int show_inode;
    int show_size;
}ls_options;

const char* get_file_color(const char* path);
void print_permissions(mode_t mode);
void display_file(ls_options *opts,char *name,struct stat buf,const char *color);
int compare_by_time(const void *a, const void *b);
int compare_by_name(const void *a, const void *b);
void display_dir(ls_options *opts, char *path);
void handle_path(ls_options *opts,char *path);
void init_ls_options(ls_options *opts);
int search_n(ls_options *opts,char *name,struct stat buf);
void get_terminal_width(int *width);


// -a: 显示所有文件，包括以点（.）开头的隐藏文件。
// -l: 使用长格式列出文件的详细信息，包括权限、所有者、文件大小和最后修改时间等。
// -R: 递归列出所有子目录中的内容。
// -t: 按时间顺序排序，默认是按修改时间排序，从最新到最旧。
// -r: 反向排序，通常与其他排序选项一起使用（如 -t）。
// -i: 显示文件的 inode 号码。
// -s: 显示文件的大小（以块为单位）。

// struct stat {
//     dev_t     st_dev;     /* 文件所在设备 */
//     ino_t     st_ino;     /* inode 节点号 */
//     mode_t    st_mode;    /* 文件类型和权限 */
//     nlink_t   st_nlink;   /* 文件的硬链接数 */
//     uid_t     st_uid;     /* 文件所有者的用户ID */
//     gid_t     st_gid;     /* 文件所有者的组ID */
//     dev_t     st_rdev;    /* 设备文件的设备号 */
//     off_t     st_size;    /* 文件的总字节数 */
//     blksize_t st_blksize; /* 文件系统的块大小 */
//     blkcnt_t  st_blocks;  /* 文件占用的块数 */
//     time_t    st_atime;   /* 最后访问时间 */
//     time_t    st_mtime;   /* 最后修改时间 */
//     time_t    st_ctime;   /* 最后状态改变时间（例如权限变化） */
// };

// struct dirent{
//     ino_t d_ino;           // 条目的inode号
//     off_t d_off;           // 到下一个dirent的偏移量
//     unsigned short d_reclen; // 记录的长度
//     unsigned char d_type;  // 文件类型（例如普通文件、目录等）
//     char d_name[256];      // 条目的名称（文件或目录）
// };

//没有l就对齐输出，判断一行能放几个
int search_n(ls_options *opts,char *name,struct stat buf){
    int length=0;
    if(opts->show_size){
        length+=5;
    }
    if(opts->show_inode){
        length+=9;
    }
    int terminal_width;  // 变量用于存储终端宽度
    // 将变量的地址传递给函数
    get_terminal_width(&terminal_width);
    int actual_length=0;
    for (int i=0;name[i]!='\0';) {
        if((unsigned char)name[i]>=0x80) {
            actual_length+=2;  // 汉字显示占两个字符宽度
            i+=3;  // UTF-8 编码汉字占 3 字节
        }else{
            actual_length+=1;  // ASCII 占一个字符宽度
            i++;
        }
    }
    length+=actual_length;  // 文件名总长度
    int n=terminal_width/length;  // 计算每行最多文件数
    return n;
}

//获取终端宽度
void get_terminal_width(int *width) {
    struct winsize ws;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0) {
        *width = ws.ws_col;
    } else {
        *width = 80;  // 默认宽度
    }
}

// 获取文件颜色
const char* get_file_color(const char* path) {
    struct stat file_stat;
    if (stat(path, &file_stat) == -1) {
        return RESET;  // 文件不存在，返回默认颜色
    }

    // 判断文件类型
    if (S_ISDIR(file_stat.st_mode)) {
        return BLUE;   // 文件夹为蓝色
    } else if (S_ISLNK(file_stat.st_mode)) {
        return GREEN;  // 符号链接为绿色
    } else if (S_ISREG(file_stat.st_mode)) {
        // 判断是否为可执行文件
        if (file_stat.st_mode & S_IXUSR || file_stat.st_mode & S_IXGRP || file_stat.st_mode & S_IXOTH) {
            return YELLOW; // 可执行文件为黄色
        }
        return RESET; // 普通文件为默认颜色
    }
    return RESET; // 默认颜色
}

//获取权限
void print_permissions(mode_t mode) {
    // 根据文件类型打印字符
    if (S_ISSOCK(mode)) {
        printf("s"); // 套接字文件
    } else if (S_ISFIFO(mode)) {
        printf("f"); // FIFO文件（管道）
    } else if (S_ISDIR(mode)) {
        printf("d"); // 目录文件
    } else if (S_ISREG(mode)) {
        printf("-"); // 普通文件
    } else if (S_ISBLK(mode)) {
        printf("b"); // 块设备文件
    } else if (S_ISCHR(mode)) {
        printf("c"); // 字符设备文件
    } else if (S_ISLNK(mode)) {
        printf("l"); // 符号链接
    }
    // 打印权限
    printf((mode & S_IRUSR) ? "r" : "-"); // 用户读权限
    printf((mode & S_IWUSR) ? "w" : "-"); // 用户写权限
    printf((mode & S_IXUSR) ? "x" : "-"); // 用户执行权限
    printf((mode & S_IRGRP) ? "r" : "-"); // 组读权限
    printf((mode & S_IWGRP) ? "w" : "-"); // 组写权限
    printf((mode & S_IXGRP) ? "x" : "-"); // 组执行权限
    printf((mode & S_IROTH) ? "r" : "-"); // 其他用户读权限
    printf((mode & S_IWOTH) ? "w" : "-"); // 其他用户写权限
    printf((mode & S_IXOTH) ? "x" : "-"); // 其他用户执行权限
}

void display_file(ls_options *opts,char *name,struct stat buf,const char *color){
    if(!opts->show_all&&*name=='.'){
        return;
    }
    //-i选项：显示inode号
    if(opts->show_inode){
        printf("%8ld ",buf.st_ino);
    }
    // -s选项：显示文件的块数
    if(opts->show_size){
        printf("%4ld ",buf.st_blocks/2);  // 打印文件占用的块数(512区块转化为1024区块要除以2)
    }
    if(opts->long_format){
        print_permissions(buf.st_mode);  //打印权限
        printf(" %3ld ",buf.st_nlink);   //打印硬链接数
        printf("%8s ",getpwuid(buf.st_uid)->pw_name);  //打印文件所有者
        printf("%8s ",getgrgid(buf.st_gid)->gr_name);  //打印文件所属组
        // 打印文件大小
        printf("%11ld ",buf.st_size);
        // 打印文件修改时间
        struct tm *t=localtime(&buf.st_mtime);
        printf("%d-%02d-%02d %02d:%02d ",t->tm_year+1900,t->tm_mon+1,t->tm_mday,t->tm_hour,t->tm_min);
    
    // 打印文件名并加上颜色
    printf("%s%s%s\n",color,name,RESET);  // name 为文件名，color 为文件颜色，RESET 恢复颜色
    }
    if(!opts->long_format){
        printf("%s%s%s ",color,name,RESET);  // name 为文件名，color 为文件颜色，RESET 恢复颜色
    }
}


// struct dirent {
//     ino_t d_ino;           // 条目的inode号
//     off_t d_off;           // 到下一个dirent的偏移量
//     unsigned short d_reclen; // 记录的长度
//     unsigned char d_type;  // 文件类型（例如普通文件、目录等）
//     char d_name[256];      // 条目的名称（文件或目录）
// };


typedef struct{
    char name[256];      // 文件名
    char full_path[PATH_MAX]; // 完整路径
    struct stat stat_buf; // 文件状态
}file_entry;


// 按修改时间排序
int compare_by_time(const void *a, const void *b) {
    const file_entry *fa=(const file_entry *)a;
    const file_entry *fb=(const file_entry *)b;
    
    if (fa->stat_buf.st_mtime!=fb->stat_buf.st_mtime) {
        return fb->stat_buf.st_mtime-fa->stat_buf.st_mtime;  // 从大到小排序
    }
    return strcmp(fa->name,fb->name);  // 时间相同则按名称排序
}

// 按文件名排序，隐藏文件去除开头的 '.'
int compare_by_name(const void *a, const void *b) {
    const file_entry *fa=(const file_entry *)a;
    const file_entry *fb=(const file_entry *)b;

    // 获取实际的文件名，去掉开头的 '.'
    const char *name_a=(fa->name[0]=='.')?fa->name+1:fa->name;
    const char *name_b=(fb->name[0]=='.')?fb->name+1:fb->name;

    return strcmp(name_a,name_b);
}

void display_dir(ls_options *opts, char *path) {
    DIR *dir=opendir(path);
    if(dir==NULL){
        perror("opendir");
        return;
    }

    struct dirent *entry;
    struct stat file_stat;

    file_entry *entries=NULL;
    size_t count=0;

    // 收集目录内容
    while((entry=readdir(dir))!=NULL){
        if(!opts->show_all&&entry->d_name[0]=='.'){
            continue;
        }

        // 动态分配存储空间
        file_entry *temp=realloc(entries,(count + 1)*sizeof(file_entry));
        if (temp == NULL){
            perror("realloc");
            free(entries);
            closedir(dir);
            return;
        }
        entries = temp;
        // 填充文件信息
        snprintf(entries[count].full_path, sizeof(entries[count].full_path), "%s/%s", path, entry->d_name);
        strncpy(entries[count].name, entry->d_name, sizeof(entries[count].name));

        if (lstat(entries[count].full_path, &file_stat) == -1) {
            perror("lstat");
            continue;
        }
        entries[count].stat_buf = file_stat;
        count++;
    }
    closedir(dir);

    // 排序目录内容
    if (opts->sort_by_time) {
        qsort(entries, count, sizeof(file_entry), compare_by_time);  // 按时间排序
    } else {
        qsort(entries, count, sizeof(file_entry), compare_by_name);  // 按名称排序
    }

    // 逆序排序（-r 选项）
    if (opts->reverse) {
        for (size_t i = 0; i < count / 2; i++) {
            file_entry temp = entries[i];
            entries[i] = entries[count - i - 1];
            entries[count - i - 1] = temp;
        }
    }

    // 获取终端一行最多能输出几个文件名
    int arr[count];
    int max = 0;
    // 遍历目录中的每个文件，计算其对齐信息
    for (size_t i = 0; i < count; i++) {
        arr[i] = search_n(opts, entries[i].name, entries[i].stat_buf); // 使用文件名和文件状态
    }
    // 获取最大值
    max = arr[0]; // 初始化为第一个文件的对齐值
    for (size_t i = 1; i < count; i++) {
        if (arr[i] < max) {
            max = arr[i];
        }
    }

    // 输出文件信息
    for (size_t i = 0; i < count; i++) {
            // 获取文件对应的颜色
            const char *color = get_file_color(entries[i].full_path);
            // 输出文件信息
            display_file(opts, entries[i].name, entries[i].stat_buf, color);
            if(!opts->long_format){
                int terminal_width;  // 变量用于存储终端宽度
                // 将变量的地址传递给函数
                get_terminal_width(&terminal_width);
                int max_name_len=terminal_width/max;
                if(opts->show_inode){
                    max_name_len-=9;
                }
                if(opts->show_size){
                    max_name_len-=5;
                }
                int file_len = strlen(entries[i].name);
                // 如果文件名长度小于最大长度，添加空格补齐
                int spaces = max_name_len - file_len;
//文件有汉字对齐有偏差
                for (int j = 0; j < spaces; j++) {
                    printf(" ");  // 输出补齐的空格
                }
            // 非长格式时，控制输出的对齐
                if (!opts->long_format) {
                    if ((i + 1) % max == 0) { // 每行输出 `max` 个文件后换行
                        printf("\n");
                    }
                }
        }
    }
    // 如果最后一行未满，手动换行
    if (!opts->long_format && count % max != 0){
        printf("\n");
    }

    //处理-R
    for (size_t i = 0; i < count; i++){
        if (opts->recursive && S_ISDIR(entries[i].stat_buf.st_mode) && strcmp(entries[i].name, ".") != 0 && strcmp(entries[i].name, "..") != 0) {
            // 如果是目录且递归选项开启，递归处理子目录
            handle_path(opts, entries[i].full_path);
        }
    }
// 释放动态数组
free(entries);
}



void handle_path(ls_options *opts,char *path){
    struct stat Stat;
    if(lstat(path,&Stat)==-1){
        perror("lstat");
        return;
    }
    printf("\n%s:\n", path);  // 显示当前路径
    if(S_ISDIR(Stat.st_mode)){//判断是否为目录
    display_dir(opts,path);
    }
    else{
    // 获取文件颜色（根据文件类型设置颜色）
    const char *color = get_file_color(path);  // 获取文件的颜色
    char *name = basename(path); // 使用 basename 提取文件名
    display_file(opts,name,Stat,color);
    }
}

void init_ls_options(ls_options *opts){   //初始化
    opts->show_all = 0;         // `-a` 选项：显示所有文件，包括隐藏文件
    opts->long_format = 0;      // `-l` 选项：显示详细信息
    opts->recursive = 0;        // `-R` 选项：递归列出子目录内容
    opts->sort_by_time = 0;     // `-t` 选项：按时间排序
    opts->reverse = 0;          // `-r` 选项：逆序输出
    opts->show_inode = 0;       // `-i` 选项：显示 inode 号
    opts->show_size = 0;        // `-s` 选项：显示文件大小
}

int main(int argc,char *argv[]){
    ls_options *opts = malloc(sizeof(ls_options));
    if (opts == NULL) {
    perror("malloc");
    exit(1);  // 退出程序
    }
    init_ls_options(opts);
    int opt;
    while((opt=getopt(argc,argv,"alRtris"))!=-1){     //getopt来分割
        switch(opt){
            case 'a':
                opts->show_all=1;
                break;
            case 'l':
                opts->long_format=1;
                break;
            case 'R':
                opts->recursive=1;
                break;
            case 't':
                opts->sort_by_time=1;
                break;
            case 'r':
                opts->reverse=1;
                break;
            case 'i':
                opts->show_inode=1;
                break;
            case 's':
                opts->show_size=1;
                break;
            default:
                break;
        }
    }
    int m=0;
    for(int i=1;i<argc;i++){
        if(argv[i][0]!='-'){
            handle_path(opts,argv[i]);
            m++;
        }
    }
    if(m==0){
        handle_path(opts,"./"); //没有路径则为当前目录
    }
    free(opts);
    return 0;
}