#include <stdio.h>
#include <assert.h>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdbool.h>
#include <ctype.h>
#include <string.h>

typedef struct child_list child_list_t;
typedef struct process process_t;


bool show_pids = false;
bool numeric_sort = true;
bool show_version = false;

struct process {
	pid_t  pid;
	char   comm[256];
	char   state;
	pid_t  ppid;
	//process_t*  parent;
	child_list_t* child;
};

struct child_list {
	process_t*		body;
	child_list_t*	next;
};


process_t* visited[1024];
int len = 0;
void * root = NULL;

bool has_visited(pid_t p) {
	if (len == 0) return false;
	for(int i = 0; i < len; i++) {
		process_t* ptr = (process_t*) visited[i];
		if (ptr->pid == p)
			return true;
	}
	return false;
}

void insert_child(process_t* p, process_t* c) {
    child_list_t* cur = p->child;
    child_list_t* new_node = malloc(sizeof(child_list_t));
    new_node->body = c;
    new_node->next = NULL;

    if (numeric_sort) {
        if (cur == NULL || c->pid < cur->body->pid) {
            new_node->next = cur;
            p->child = new_node;
        } else {
            while (cur->next != NULL && c->pid > cur->next->body->pid) {
                cur = cur->next;
            }
            new_node->next = cur->next;
            cur->next = new_node;
        }
    } else {
        new_node->next = cur;
        p->child = new_node;
    }
}


void read_process_dir() {
    DIR *dir;
    struct dirent * entry;

    dir = opendir("/proc/");
    if (dir == NULL) { 
            printf("Unable to open /proc directory");
            return;
    }
 
    while ((entry = readdir(dir)) != NULL) {
		// printf("enter dir:%s\n", entry->d_name);
        // Check if the directory entry is a numeric value (a process ID)
        if (entry->d_type == DT_DIR && isdigit(entry->d_name[0])) {
            char stat_path[2048];
            FILE *fp;

            // Construct the path to the process' stat file
            snprintf(stat_path, sizeof(stat_path), "/proc/%s/stat", entry->d_name);
	    fp = fopen(stat_path, "r");
            if (fp == NULL) {
                continue;
            }
            // 从文件中读取数据
			pid_t  pid;
			char   comm[256];
			char   state;
			pid_t  ppid;

    		if (fscanf(fp, "%d %255s %c %d", &pid, comm, &state, &ppid) == 4) {
			if (!has_visited(pid)) {
			    process_t* prop = malloc(sizeof(process_t));
			    prop->pid = pid;
	
			    size_t len = strlen(comm);
			    if (comm[0] == '(' && comm[len - 1] == ')') {
				strncpy(prop->comm, comm + 1, len - 2);
				prop->comm[len - 2] = '\0';
			    } else {
				strncpy(prop->comm, comm, sizeof(prop->comm) - 1);
				prop->comm[sizeof(prop->comm) - 1] = '\0';
			    }
	
			    prop->state = state;
			    prop->ppid = ppid;
			    prop->child = NULL;
	
			    visited[len++] = (void*)prop;
			}
		} else {
			printf("fscanf failed\n");
		}
		fclose(fp);
        }
    }

    int res = closedir(dir);
}

void construct_tree() {
	for(size_t i = 0; i < len; i++) {
		process_t* p = (process_t *) visited[i];
		if (p->pid == 1)
			root = visited[i];
		if (p->ppid != 0) {
			for(size_t j = 0; j < len; j++) {
				process_t* pp = (process_t *) visited[j];
				if (pp->pid == p->ppid) {
					insert_child(pp, p);
					break;
				}
			}
		}
	}
}

void dfs(process_t* p, int depth) {
	if (p == NULL) return;
	for (int i = 0; i < depth; i++)
		printf("\t");
	printf("%s(%d)\n", p->comm, p->pid);
	child_list_t* cur = p->child;
	int d = depth + 1;
	while (cur->next != NULL) {
		dfs(cur->next->body, d);
		cur = cur->next;
	}
}	

void print() {
//	for(int i = 0; i < len; i++) {
//		process_t* ptr = (process_t *) visited[i];
//		printf("process: %s, pid: %d, ppid: %d\n", ptr->comm, ptr->pid, ptr->ppid);
//	}
	dfs(root, 0);
}


int main(int argc, char *argv[]) {
  for (int i = 0; i < argc; i++) {
    //assert(argv[i]);
    printf("argv[%d] = %s\n", i, argv[i]);
  }
    // assert(!argv[argc]i);
	read_process_dir();
	construct_tree();
	print();
  return 0;
}
