#include <stdio.h>
#include <dirent.h>
#include <limits.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>


#define KNRM  "\x1B[0m"
#define KRED  "\x1B[31m"
#define KGRN  "\x1B[32m"
#define KYEL  "\x1B[33m"
#define KBLU  "\x1B[34m"
#define KMAG  "\x1B[35m"
#define KCYN  "\x1B[36m"
#define KWHT  "\x1B[37m"

static char* colors[8] = {
    KNRM,
    KRED,
    KGRN,
    KYEL,
    KBLU,
    KMAG,
    KCYN,
    KWHT
};

void depth_color(int depth)
{
    while (depth >= 8)
        depth -= 8;
    printf("%s", colors[depth]);
}



/// Returns the total amount of remaining entries
/// until readdir returns 0 when called on the passed DIR*
int remaining_entries(DIR* dirp)
{
    long int pos = telldir(dirp);
    int c = 0;
    while (readdir(dirp))
        c++;
    seekdir(dirp, pos);
    return c;
}


/// Skips "." and ".." because I don't like when they're displayed
int skip_ent(struct dirent* dir)
{
    return strcmp(dir->d_name, "." ) == 0
        || strcmp(dir->d_name, "..") == 0;
}


void read_ents(DIR* dirp, struct dirent** ents, int entc)
{
    while (entc)
    {
        struct dirent* ent = readdir(dirp);
        
        if (!skip_ent(ent))
            ents[--entc] = ent;
    }
}


void swap(struct dirent** o1, struct dirent** o2)
{
    struct dirent* temp = *o1;
    *o1 = *o2;
    *o2 = temp;
}


void sort(
    struct dirent** ents, const int entc,
    int (*should_swap)(struct dirent*, struct dirent*)
)
{
    int changed;
    do
    {        
        changed = 0;
        for (int i = 0; i < entc-1; i++)
        {
            if (!should_swap(ents[i], ents[i+1]))
                continue;

            swap(&ents[i], &ents[i+1]);
            changed = 1;
        }
    } while (changed);
}


int sort_type(struct dirent* a, struct dirent* b)
{
    // Skip if both items are the same type
    if (a->d_type == b->d_type)
        return 0;
    
    // Skip if a directory comes before something else
    // Sort directories first
    if (a->d_type == DT_DIR
     && b->d_type != DT_DIR)
        return 0;
    
    // Skip if both items are not a directory
    if (a->d_type != DT_DIR
     && b->d_type != DT_DIR)
        return 0;
    
    return 1;
}

int sort_alpha(struct dirent* a, struct dirent* b)
{
    return strcmp(a->d_name, b->d_name) > 0;
}

void sort_ents(struct dirent** ents, const int entc)
{
    sort(ents, entc, &sort_alpha);
    sort(ents, entc, &sort_type);
}


void recursive_list_dir(const char* path, const int depth,
    int* draw_line, int max_depth)
{
    if (depth == max_depth)
        return;
    
    // Open the passed directory
    DIR* dirp = opendir(path);
    if (!dirp)
        return;

    // Calculate how many items in directory,
    // then remove 2 to account for "." and ".."
    const int entc = remaining_entries(dirp) - 2;

    // Read all entries and sort the array alphabetically
    struct dirent* ents[entc];
    read_ents(dirp, ents, entc);
    sort_ents(ents, entc);


    for (int c = 0; c <= entc-1; c++)
    {
        struct dirent* dirent = ents[c];
        int last = c == entc-1;

        for (int i = 0; i < depth; i++)
        {
            depth_color(i);
            if (draw_line[i])
                printf("|   ");
            else
                printf("    ");
        }
        depth_color(depth);

        if (last)
            printf("└──");
        else
            printf("├──");
        
        if (dirent->d_type != DT_DIR)
        {
            printf("%s\n", dirent->d_name);
        }
        else
        {
            printf("/%s\n", dirent->d_name);
            
            char d_path[FILENAME_MAX];
            sprintf(d_path, "%s/%s", path, dirent->d_name);

            draw_line[depth] = !last;

            recursive_list_dir(d_path, depth+1, draw_line, max_depth);
        }
    }
}

int digits_only(const char *s)
{
    while (*s) {
        if (isdigit(*s++) == 0) return 0;
    }

    return 1;
}


void main(int argc, char* argv[])
{
    if (argc > 1 && strcmp(argv[1], "-h") == 0)
    {
        printf("List Directory Kinda Thing.\nRecursively lists the contents of a directory (with a lot of colors).\n\nUsage: lskt path [options]\nOptions:\n\t-r\t\tsets the maximum depth reached by the search\n\nEagles can fly,\nHumans can try,\nBut that's about it.\n");
    }
    else
    {
        int maxdepth = 1;
        char* path = ".";

        for (int i = 0; i < argc; i++)
            if (strcmp(argv[i], "-r") == 0)
            {
                if (i+1 < argc && digits_only(argv[i+1]))
                    maxdepth = atoi(argv[++i]);
            }
            else if (strcmp(argv[i], "-p") == 0)
            {
                if (i+1 < argc)
                    path = argv[++i];
                else
                {
                    perror("Path is required after `-p`");
                }
            }
        
        if (maxdepth == 0)
            return;

        int draw_line[maxdepth];
        for (int i = 0; i < maxdepth; i++)
            draw_line[i] = 0;

        recursive_list_dir(path, 0, draw_line, maxdepth);
    }
}