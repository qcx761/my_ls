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

// 文件类型的颜色代码
#define RESET   "\033[0m"
#define BLUE    "\033[34m"
#define GREEN   "\033[32m"
#define CYAN    "\033[36m"
#define MAGENTA "\033[35m"

// 定义结构体
typedef struct {
    int show_all;
    int long_format;
    int recursive;
    int sort_by_time;
    int reverse;
    int show_inode;
    int show_size;
}ls_options;

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

// struct dirent {
//     ino_t d_ino;           // 条目的inode号
//     off_t d_off;           // 到下一个dirent的偏移量
//     unsigned short d_reclen; // 记录的长度
//     unsigned char d_type;  // 文件类型（例如普通文件、目录等）
//     char d_name[256];      // 条目的名称（文件或目录）
// };

// 获取文件颜色
const char* get_file_color(const char* path) {   //颜色调用     //printf("%s%s%s\n", get_file_color(file_path), file_path, RESET);
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
        return RESET;  // 普通文件为默认颜色
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

void display_file(ls_options *opts,char *name,struct stat buf){
    if(!opts->show_all&&*name=='.'){
        return;
    }
    //-i选项：显示inode号
    if (opts->show_inode){
        printf("%ld ",buf.st_ino);
    }
    if (opts->long_format){
        print_permissions(buf.st_mode);  //打印权限
        printf(" %ld ",buf.st_nlink);   //打印硬链接数
        printf("%s ",getpwuid(buf.st_uid)->pw_name);  //打印文件所有者
        printf("%s ",getgrgid(buf.st_gid)->gr_name);  //打印文件所属组
        // -s选项：显示文件大小
        if(opts->show_size){
            printf("%6ld ",buf.st_size);  //打印文件大小
        }
        // 打印文件修改时间
        struct tm *t = localtime(&buf.st_mtime);
        printf("%d-%02d-%02d %02d:%02d ", t->tm_year + 1900, t->tm_mon + 1, t->tm_mday, t->tm_hour, t->tm_min);
    }
    // 获取文件颜色（根据文件类型设置颜色）
    const char *color = get_file_color(name);  // 获取文件的颜色
    // 打印文件名并加上颜色
    printf("%s%s%s\n", color, name, RESET);  // name 为文件名，color 为文件颜色，RESET 恢复颜色
}



// struct file
// {
//     char filename[256];  // 存储文件名
//     struct stat STA;     // 存储文件的状态信息（如权限、大小、时间等）
// };


void display_dir(ls_options *opts,char *path,struct stat buf){
    //文件接受名字与opts指针和stat
    







}

void handle_path(ls_options *opts,char *path){
    struct stat Stat;
    if(lstat(path,&Stat)==-1){
        perror("lstat");
        return;
    }
    if(S_ISDIR(Stat.st_mode)){//判断是否为目录
    display_dir(opts,path,Stat);
    }
    else{
    char *name = basename(path); // 使用 basename 提取文件名
    display_file(opts,name,Stat);
    }
}

void init_ls_options(ls_options *opts) {
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
    int n;
    for(int i=1;i<argc;i++){
        if(argv[i][0]!='-'){  // 找到第一个非选项参数
            n=i;
            break;
        }
    }
    if(n==argc){
        handle_path(opts,"./");
    }
    for(;n<argc;n++){
        handle_path(opts,argv[n]);
    }
    free(opts);
    return 0;
}