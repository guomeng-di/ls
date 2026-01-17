// ls任务的实现：
// 1 -a 列出目录下所有文件，包括隐藏文件
// 2 -l 列出文件详细信息
// 3 -R 递归列出目录及其子目录中文件
// 4 -t 按照最后修改时间由新到旧排序
// 5 -r 目录反向排序 ASCII码从大到小
// 6 -i 输出文件的i节点的索引信息
// 7 -s 在文件名后输出文件大小
#include <stdio.h>
#include <stdlib.h>//qsort
#include <string.h>//sizeof
#include <dirent.h>//opendir readdtr
#include <sys/stat.h>//判断文件类型时的S_ISDIR等
#include <sys/types.h>//mode_t（文件权限类型）inode time等文件信息
#include <unistd.h>//readlink 获取符号链接指向的实际目标文件的路径
#include <time.h>//时间
#include <pwd.h>//用户名
#include <grp.h>//组名
#include <getopt.h>//optind
// 定义颜色
#define COLOR_EXEC    "\033[31m"    // 目录颜色
#define COLOR_FIFO    "\033[35m"    // 软链接颜色
#define COLOR_LINK    "\033[36m"    // 可执行文件颜色
#define COLOR_BLOCK   "\033[34m"    // .和..目录项颜色
#define COLOR_RESET   "\033[0m"     // 默认重置颜色
#define MAX1 256
#define MAX2 1024
int a=0,l=0,R=0,t=0,r=0,i=0,s=0;
typedef struct{
    char name[MAX1];//文件名
    char arr[MAX2];//绝对路径
    struct stat st;//文件属性
}File_info;
int compare_shunxu(const void* a,const void* b){
   File_info* x=(File_info*)a;
   File_info* y=(File_info*)b;
   int n=0;
   if(t) n=y->st.st_mtime-x->st.st_mtime;//从新到旧
   else if(s&&!n) n=x->st.st_size-y->st.st_size; //从小到大
   else if(i&&!n) n=x->st.st_ino-y->st.st_ino;//从小到大
   else if(!n)n=strcmp(x->name, y->name);//按照文件名排序
   return r?-n:n;
}
char* get_file_color(const char* filename, mode_t file_mode) 
{   char* result_color=COLOR_RESET;  //默认返回重置颜色
    int m=(strcmp(filename, ".")==0)||(strcmp(filename, "..")==0)?1:0;//判断是否为.或..
    int n=((file_mode&S_IXUSR)||(file_mode&S_IXGRP)||(file_mode&S_IXOTH))?1:0;//判断是否有执行权限
//按优先级判断文件类型和属性
    if (S_ISDIR(file_mode)) result_color=COLOR_EXEC;
    else if (S_ISLNK(file_mode)) result_color=COLOR_FIFO;
    else if (n)result_color=COLOR_LINK;
    else if (m)result_color=COLOR_BLOCK;
    return result_color;
}
void llong_achieve(File_info*file,mode_t mode)//-l
{   char perm[11];
    if(S_ISDIR(mode))perm[0]='d';
    else if(S_ISLNK(mode))perm[0]='l';
    else if(S_ISBLK(mode))perm[0]='b';
    else if(S_ISCHR(mode))perm[0]='c';
    else if(S_ISFIFO(mode))perm[0]='p';
    else if(S_ISSOCK(mode))perm[0]='s';
    else perm[0]='-';
// 所有者权限
    perm[1]=(mode&S_IRUSR)?'r':'-';
    perm[2]=(mode&S_IWUSR)?'w':'-';
    perm[3]=(mode&S_IXUSR)?'x':'-';
// 组权限
    perm[4]=(mode&S_IRGRP)?'r':'-';
    perm[5]=(mode&S_IWGRP)?'w':'-';
    perm[6]=(mode&S_IXGRP)?'x':'-';
// 其他用户权限
    perm[7]=(mode&S_IROTH)?'r':'-';
    perm[8]=(mode&S_IWOTH)?'w':'-';
    perm[9]=(mode&S_IXOTH)?'x':'-';
    perm[10]='\0';
//用户名 
    struct passwd* pw=getpwuid(file->st.st_uid);
    char user_name[32]="unknown";
    if(pw) strncpy(user_name, pw->pw_name, sizeof(user_name)-1);
//组名
    struct group* gr=getgrgid(file->st.st_gid);
    char group_name[32]="unknown";
    if(gr) strncpy(group_name, gr->gr_name, sizeof(group_name)-1);
//格式化时间
    char time_buf[32];
    strftime(time_buf, sizeof(time_buf), "%Y-%m-%d %H:%M", localtime(&file->st.st_mtime));
//show -l
    if(i) printf("%10ld ", file->st.st_ino);
    if(s) printf("%5ld ", file->st.st_blocks/2);
    printf("%s %2ld %s %s %5ld %s ",perm,file->st.st_nlink,user_name,group_name, file->st.st_size,time_buf);
    printf("%s%s%s",get_file_color(file->name,file->st.st_mode),file->name,COLOR_RESET );
    if (S_ISLNK(file->st.st_mode)){
        char link[MAX2];
        ssize_t len= readlink(file->arr,link,sizeof(link) - 1);
        if (len!=-1){
            link[len] = '\0';
            printf(" -> %s", link);
        }
    }
    printf("\n");
}
void all_achieve(const char*dir1){
    //File_info ddir[MAX2]; 将栈数组改为堆内存避免递归栈溢出）
    File_info* ddir=(File_info*)malloc(MAX2*sizeof(File_info));
    if(!ddir) return;
   int temp=0;
   DIR* dir=opendir(dir1);
   if(!dir) return;
   struct dirent *entry=NULL;
   while(entry=readdir(dir)){
    if(temp>=MAX2) break;
    if(entry->d_name[0]=='.'&&!a) continue;//特殊文件
    //snprintf(ddir[temp].arr, sizeof(ddir[temp].arr),"%s/%s",dir1,entry->d_name);//存绝对路径
    if(dir1[strlen(dir1)-1]!='/')
     snprintf(ddir[temp].arr,sizeof(ddir[temp].arr),"%s/%s",dir1,entry->d_name);
   else
     snprintf(ddir[temp].arr,sizeof(ddir[temp].arr),"%s%s",dir1,entry->d_name);
     strncpy(ddir[temp].name,entry->d_name,sizeof(ddir[temp].name)-1);
     ddir[temp].name[sizeof(ddir[temp].name)-1]='\0';//存文件名
    if (lstat(ddir[temp].arr,&ddir[temp].st)==-1) continue;//判断有无权限
   temp++;//存文件个数
    }
 closedir(dir);
 if(temp>0) qsort(ddir,temp, sizeof(File_info),compare_shunxu);//每次考试结构体排序都用
    for(int j=0;j<temp;j++){
        File_info *p=&ddir[j];
        if(!l){
        if(i)printf("%10ld",p->st.st_ino);
        if(s)printf("%5ld",p->st.st_blocks/2);
        printf("%s%s%s\n",get_file_color(p->name,p->st.st_mode),p->name,COLOR_RESET);
        }
        else llong_achieve(p,p->st.st_mode);
        // if(R&&S_ISDIR(p->st.st_mode)&&strcmp(p->name,".")&&strcmp(p->name, "..")&& !S_ISLNK(p->st.st_mode)&&access(p->arr, R_OK|X_OK)==0){
        //     printf("\n%s:\n",p->arr);
        //     all_achieve(p->arr);  
        //   }
        // 递归处理子目录（-R参数）
        if (R) {//严谨判断目录!!（排除软链接/./../无权限目录）
            if (S_ISDIR(p->st.st_mode) &&!S_ISLNK(p->st.st_mode) &&strcmp(p->name, ".")&&strcmp(p->name, "..")&&access(p->arr,R_OK|X_OK)==0){
                printf("\n%s:\n", p->arr);
                all_achieve(p->arr);  // 递归调用
            }
    }
}
    free(ddir);//释放堆内存（避免内存泄漏）
}
// 解析命令行参数
void argument(int argc, char *argv[], char **dir) {
    int n;
    *dir=(char*)malloc(MAX2*sizeof(char));
    if(!*dir)exit(EXIT_FAILURE);
    strcpy(*dir,".");
    while ((n=getopt(argc,argv,"alRtris"))!=-1){
        switch(n){
            case'a':a=1;break;
            case'l':l=1;break;
            case'R':R=1;break;
            case't':t=1;break;
            case'r':r=1;break;
            case'i':i=1;break;
            case's':s=1;break;
            default:free(*dir);free(*dir); exit(EXIT_FAILURE);
        }
    }//optind是表示下一个参数索引
        if (optind<argc){
        strncpy(*dir,argv[optind],MAX2-1);
       (*dir)[MAX2-1]='\0';
        }
}
 int main(int argc,char* argv[]){
    char *dir = NULL;
    argument(argc,argv,&dir);//解析参数
    all_achieve(dir);//处理目标目录
    free(dir);
    return 0;
 }