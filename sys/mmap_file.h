#ifndef __MMAP_FILE_H__
#define __MMAP_FILE_H__

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/mman.h>
#include <libgen.h>


#include <string>

// mmap 的方式定点对文件进行读写
// 不加锁
template<class T>
class CMmapFile
{
public:
    CMmapFile(const char * file_name, const int max_node_num, bool read_only = false, bool ignore_exise = false)
        :
        __file_name(file_name),
        __max_node_num(max_node_num),
        __data_array(NULL),
        __mmap_length(max_node_num * sizeof(T)),
        __read_only(read_only),
        __ignore_exist(ignore_exise)
        {
        }

    CMmapFile()
        {
        }
        
    ~CMmapFile()
        {
            fini();
        }

    int init(){
            int ret = 0;
            int fd = 0;
            bool create = false;
            int size = file_size();
            
            if (size < 0) {
                if (__read_only){                   
                    snprintf(__error_text, sizeof(__error_text), "file not exist");
                    return -1;
                }
                
                create = true;
            }

            if (size >= 0 && size != __mmap_length){
                if (__ignore_exist){
                    create = true;
                } else {
                    snprintf(__error_text, sizeof(__error_text), "file size error [%d] != exist[%d]",
                        __mmap_length, size);

                    return -1;
                }
            }

            // 创建目录
            char dir_name[256];
            strcpy(dir_name, __file_name.c_str());
            strcpy(dir_name, dirname(dir_name));
            struct stat stStat;
            if (stat(dir_name, &stStat) != 0) {
                if (mkdir(dir_name, 0775) < 0) {
                    fprintf(stderr, "mkdir %s: %s\n", dir_name, strerror(errno));
                    return -1;
                }
            }
            
            if (create){
                fd = open(__file_name.c_str(), O_EXCL | O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR);
            } else {
                fd = open(__file_name.c_str(), O_RDWR );
            }

            if (fd < 0) {
                snprintf(__error_text, sizeof(__error_text), "open [%s] error [%s]",
                    __file_name.c_str(), strerror(errno));

                return -1;
            }

            if (create) {
                // 文件清0
                u_char item = 0;
                char sTmp[10000];
                memset(sTmp, 0, sizeof(sTmp));                
                   
                //循环，把整个文件置零
                for (int i = 0; i < __mmap_length / (int)sizeof(sTmp); i++)   write(fd, (char*)sTmp, sizeof(sTmp));                
                for (int i = 0; i < __mmap_length % (int)sizeof(sTmp); i++)   write(fd, (char*)&item, sizeof(item));  
            }

            __data_array = (T *)mmap(0, __mmap_length, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    
            if(__data_array == (T *)-1) 
            {
                    snprintf(__error_text, sizeof(__error_text), "mmap faild,error=%s.\n", strerror(errno));
                    ret = -1;
            }

            close(fd);

            return ret;
        }

    int init(const char * file_name, const int max_node_num, bool read_only = false, bool ignore_exise = false) {
            __file_name = file_name;
            __max_node_num = max_node_num;
            __mmap_length = max_node_num * sizeof(T);
            __read_only = read_only;
            __ignore_exist = ignore_exise;

            return init();
        }
        
    const char * get_error_text() const {return __error_text;}

    // 文件大小
    int file_size(){
            struct stat file_state;

            // 检查文件是否存在,判断大小是否匹配
            if (::stat(__file_name.c_str(), &file_state) < 0){  
                return -1;
            }
            return (int)file_state.st_size;
        }

    // 直接获取某个节点指针
    T * getIndexPtr(int index){
            if (index >= __max_node_num || __data_array == NULL){      
                snprintf(__error_text, sizeof(__error_text), "set error index[%d] max_node[%d] date_ptr[%p]",
                    index, __max_node_num, __data_array);
            
                return NULL;
            }
            
            return __data_array + index;
        }

    T * getHeadPtr() {
            if (__read_only) {
                return NULL;
            }

            return __data_array;
        }

    // 设置某个节点
    int set(int index, const T * p) {
            if (__read_only) {
                return -1;
            }
            
            
            if (index >= __max_node_num || __data_array == NULL){      
                snprintf(__error_text, sizeof(__error_text), "set error index[%d] max_node[%ld] date_ptr[%p]",
                    index, __max_node_num, __data_array);
            
                return -1;
            }

            memcpy(__data_array + index, p, sizeof(T));

            return 0;
        }

    // 获取某个节点信息
    int get(int index, T * p){
            if (index >= __max_node_num || __data_array == NULL){     
                snprintf(__error_text, sizeof(__error_text), "set error index[%d] max_node[%ld] date_ptr[%p]",
                    index, __max_node_num, __data_array);
            
                return -1;
            }

            memcpy(p, __data_array + index, sizeof(T));

            return 0;
        }
    
    int fini(){
            if (__data_array != NULL)     
            {
                munmap(__data_array, __mmap_length);
                __data_array = NULL;
            }

            return 0;
        }

protected:
    std::string     __file_name;
    int             __max_node_num;
    T*              __data_array;

    int             __mmap_length;

    bool            __read_only;
    bool            __ignore_exist;

    char            __error_text[256];
};

#endif

