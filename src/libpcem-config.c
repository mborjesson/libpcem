#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "config.h"
#include "pcem.h"
#include "ibm.h"
#include "paths.h"

char config_file_default[256];
char config_name[256];

extern pcem_config_get_t pcem_config_get_func;
extern pcem_config_set_t pcem_config_set_func;

typedef struct list_t
{
        struct list_t *next;
} list_t;

static list_t global_config_head;
static list_t machine_config_head;

typedef struct section_t
{
        struct list_t list;
        
        char name[256];
        
        struct list_t entry_head;
} section_t;

typedef struct entry_t
{
        struct list_t list;
        
        char name[256];
        char data[256];
} entry_t;

#define list_add(new, head)                             \
        {                                               \
                struct list_t *next = head;             \
                                                        \
                while (next->next)                      \
                        next = next->next;              \
                                                        \
                (next)->next = new;                     \
                (new)->next = NULL;                     \
        }

void config_dump(int is_global)
{
        section_t *current_section;
        list_t *head = is_global ? &global_config_head : &machine_config_head;
                
        pclog("Config data :\n");
        
        current_section = (section_t *)head->next;
        
        while (current_section)
        {
                entry_t *current_entry;
                
                pclog("[%s]\n", current_section->name);
                
                current_entry = (entry_t *)current_section->entry_head.next;
                
                while (current_entry)
                {
                        pclog("%s = %s\n", current_entry->name, current_entry->data);

                        current_entry = (entry_t *)current_entry->list.next;
                }

                current_section = (section_t *)current_section->list.next;
        }
}

static section_t *create_section(list_t *head, const char *name)
{
        section_t *new_section = malloc(sizeof(section_t));
        
        memset(new_section, 0, sizeof(section_t));
        strncpy(new_section->name, name, 256);
        list_add(&new_section->list, head);
        
        return new_section;
}

static entry_t *create_entry(section_t *section, const char *name)
{
        entry_t *new_entry = malloc(sizeof(entry_t));
        memset(new_entry, 0, sizeof(entry_t));
        strncpy(new_entry->name, name, 256);
        list_add(&new_entry->list, &section->entry_head);
        
        return new_entry;
}

static list_t *get_head(int is_global)
{
        return is_global ? &global_config_head : &machine_config_head;
}
        
static section_t *find_section(list_t *head, const char *name)
{
        section_t *current_section;
        char blank[] = "";

        current_section = (section_t *)head->next;

        if (!name)
                name = blank;

        while (current_section)
        {
                if (!strncmp(current_section->name, name, 256))
                        return current_section;
                
                current_section = (section_t *)current_section->list.next;
        }
        return NULL;
}

static entry_t *find_entry(section_t *section, const char *name)
{
        entry_t *current_entry;
        
        current_entry = (entry_t *)section->entry_head.next;
        
        while (current_entry)
        {
                if (!strncmp(current_entry->name, name, 256))
                        return current_entry;

                current_entry = (entry_t *)current_entry->list.next;
        }
        return NULL;
}

void config_init(list_t *head)
{
        memset(head, 0, sizeof(list_t));

        section_t *current_section = malloc(sizeof(section_t));
        memset(current_section, 0, sizeof(section_t));
        list_add(&current_section->list, head);
}

void pcem_config_init()
{
        config_init(&global_config_head);
        config_init(&machine_config_head);
}

void config_load(int is_global, char *fn)
{
}


void config_free_head(list_t *head)
{
        section_t *current_section;
        current_section = (section_t *)head->next;
        
        while (current_section)
        {
                section_t *next_section = (section_t *)current_section->list.next;
                entry_t *current_entry;
                
                current_entry = (entry_t *)current_section->entry_head.next;
                
                while (current_entry)
                {
                        entry_t *next_entry = (entry_t *)current_entry->list.next;
                        
                        free(current_entry);
                        current_entry = next_entry;
                }

                free(current_section);                
                current_section = next_section;
        }

}

void pcem_config_close()
{
        config_free_head(&global_config_head);
        config_free_head(&machine_config_head);
}

void config_free(int is_global)
{
}

#ifdef PIC
int cpu_use_dynarec_value = -1;
#endif

int config_get_int(int is_global, char *head, char *name, int def)
{
        int value = def;
        if (pcem_config_get_func && pcem_config_get_func(PCEM_CONFIG_TYPE_INT, is_global, head, name, &value))
        {
                list_t *h = get_head(is_global);
                section_t *section;
                entry_t *entry;

                section = find_section(h, head);
                
                if (!section)
                        section = create_section(h, head);
                        
                entry = find_entry(section, name);

                if (!entry)
                        entry = create_entry(section, name);
                
                sprintf(entry->data, "%i", value);

#ifdef PIC
                if (strcmp("cpu_use_dynarec", name) == 0)
                {
                        cpu_use_dynarec_value = value;
                        return 0;
                }
#endif

                return value;
        }
        return def;
}

float config_get_float(int is_global, char *head, char *name, float def)
{
        float value = def;
        if (pcem_config_get_func && pcem_config_get_func(PCEM_CONFIG_TYPE_FLOAT, is_global, head, name, &value))
        {
                list_t *h = get_head(is_global);
                section_t *section;
                entry_t *entry;

                section = find_section(h, head);

                if (!section)
                        section = create_section(h, head);

                entry = find_entry(section, name);

                if (!entry)
                        entry = create_entry(section, name);

                sprintf(entry->data, "%f", value);

                return value;
        }
        return def;
}

char *config_get_string(int is_global, char *head, char *name, char *def)
{
        char value[256];
        if (pcem_config_get_func && pcem_config_get_func(PCEM_CONFIG_TYPE_STRING, is_global, head, name, &value))
        {
                list_t *h = get_head(is_global);
                section_t *section;
                entry_t *entry;

                section = find_section(h, head);
                
                if (!section)
                        section = create_section(h, head);
                        
                entry = find_entry(section, name);

                if (!entry)
                        entry = create_entry(section, name);
                
                strncpy(entry->data, value, 256);

                return entry->data;
        }
        return def;
}

void config_set_int(int is_global, char *head, char *name, int val)
{
#ifdef PIC
        if (strcmp("cpu_use_dynarec", name) == 0 && cpu_use_dynarec_value >= 0)
                val = cpu_use_dynarec_value;
#endif

        if (pcem_config_set_func && pcem_config_set_func(PCEM_CONFIG_TYPE_INT, is_global, head, name, &val))
        {
                list_t *h = get_head(is_global);
                section_t *section;
                entry_t *entry;

                section = find_section(h, head);
                
                if (!section)
                        section = create_section(h, head);
                        
                entry = find_entry(section, name);

                if (!entry)
                        entry = create_entry(section, name);
                
                sprintf(entry->data, "%i", val);
        }
}

void config_set_float(int is_global, char *head, char *name, float val)
{
        if (pcem_config_set_func && pcem_config_set_func(PCEM_CONFIG_TYPE_FLOAT, is_global, head, name, &val))
        {
                list_t *h = get_head(is_global);
                section_t *section;
                entry_t *entry;

                section = find_section(h, head);

                if (!section)
                        section = create_section(h, head);

                entry = find_entry(section, name);

                if (!entry)
                        entry = create_entry(section, name);

                sprintf(entry->data, "%f", val);
        }
}

void config_set_string(int is_global, char *head, char *name, char *val)
{
        if (pcem_config_set_func && pcem_config_set_func(PCEM_CONFIG_TYPE_STRING, is_global, head, name, val))
        {
                list_t *h = get_head(is_global);
                section_t *section;
                entry_t *entry;

                section = find_section(h, head);
                
                if (!section)
                        section = create_section(h, head);
                        
                entry = find_entry(section, name);

                if (!entry)
                        entry = create_entry(section, name);
                
                strncpy(entry->data, val, 256);
        }
}

char *get_filename(char *s)
{
        int c = strlen(s) - 1;
        while (c > 0)
        {
                if (s[c] == '/' || s[c] == '\\')
                        return &s[c + 1];
                c--;
        }
        return s;
}

void append_filename(char *dest, char *s1, char *s2, int size)
{
        sprintf(dest, "%s%s", s1, s2);
}

void append_slash(char *s, int size)
{
        int c = strlen(s) - 1;
        if (s[c] != '/' && s[c] != '\\')
        {
                if (c < size - 2)
                        strcat(s, "/");
                else
                        s[c] = '/';
        }
}

void put_backslash(char *s)
{
        int c = strlen(s) - 1;
        if (s[c] != '/' && s[c] != '\\')
        {
                s[c + 1] = '/';
                s[c + 2] = 0;
        }
}

char *get_extension(char *s)
{
        int c = strlen(s) - 1;

        if (c <= 0)
                return s;

        while (c && s[c] != '.')
                c--;

        if (!c)
                return &s[strlen(s)];

        return &s[c + 1];
}

void config_save(int is_global, char *fn)
{
        
}

void *pcem_config_load(const char *fn)
{
        char *v;
        FILE *f = fopen(fn, "rt");
        if (!f)
                return 0;

        section_t *current_section;
        list_t *head = malloc(sizeof(list_t));
        
        memset(head, 0, sizeof(list_t));

        current_section = malloc(sizeof(section_t));
        memset(current_section, 0, sizeof(section_t));
        list_add(&current_section->list, head);

        while (1)
        {
                int c;
                char buffer[256];

                v = fgets(buffer, 255, f);
                if (feof(f)) break;
                
                c = 0;
                
                while (buffer[c] == ' ' && buffer[c])
                      c++;

                if (!buffer[c]) continue;
                
                if (buffer[c] == '#') /*Comment*/
                        continue;

                if (buffer[c] == '[') /*Section*/
                {
                        section_t *new_section;
                        char name[256];
                        int d = 0;
                        
                        c++;
                        while (buffer[c] != ']' && buffer[c])
                                name[d++] = buffer[c++];

                        if (buffer[c] != ']')
                                continue;
                        name[d] = 0;
                        
                        new_section = malloc(sizeof(section_t));
                        memset(new_section, 0, sizeof(section_t));
                        strncpy(new_section->name, name, 256);
                        list_add(&new_section->list, head);
                        
                        current_section = new_section;
                        
//                        pclog("New section : %s %p\n", name, (void *)current_section);
                }
                else
                {
                        entry_t *new_entry;
                        char name[256];
                        int d = 0, data_pos;

                        while (buffer[c] != '=' && buffer[c] != ' ' && buffer[c])
                                name[d++] = buffer[c++];
                
                        if (!buffer[c]) continue;
                        name[d] = 0;

                        while ((buffer[c] == '=' || buffer[c] == ' ') && buffer[c])
                                c++;
                        
                        if (!buffer[c]) continue;
                        
                        data_pos = c;
                        while (buffer[c])
                        {
                                if (buffer[c] == '\n')
                                        buffer[c] = 0;
                                c++;
                        }

                        new_entry = malloc(sizeof(entry_t));
                        memset(new_entry, 0, sizeof(entry_t));
                        strncpy(new_entry->name, name, 256);
                        strncpy(new_entry->data, &buffer[data_pos], 256);
                        list_add(&new_entry->list, &current_section->entry_head);

//                        pclog("New data under section [%s] : %s = %s\n", current_section->name, new_entry->name, new_entry->data);
                }
        }
        
        fclose(f);

        return head;
}

void pcem_config_save(void *handle, const char *fn)
{
        FILE *f = fopen(fn, "wt");
        section_t *current_section;
        list_t *head = (list_t *) handle;
                
        current_section = (section_t *)head->next;
        
        while (current_section)
        {
                entry_t *current_entry;
                
                if (current_section->name[0])
                        fprintf(f, "\n[%s]\n", current_section->name);
                
                current_entry = (entry_t *)current_section->entry_head.next;
                
                while (current_entry)
                {
                        fprintf(f, "%s = %s\n", current_entry->name, current_entry->data);

                        current_entry = (entry_t *)current_entry->list.next;
                }

                current_section = (section_t *)current_section->list.next;
        }
        
        fclose(f);
}

void pcem_config_free(void *handle)
{
        section_t *current_section;
        list_t *head = (list_t *) handle;
        current_section = (section_t *)head->next;
        
        while (current_section)
        {
                section_t *next_section = (section_t *)current_section->list.next;
                entry_t *current_entry;
                
                current_entry = (entry_t *)current_section->entry_head.next;
                
                while (current_entry)
                {
                        entry_t *next_entry = (entry_t *)current_entry->list.next;
                        
                        free(current_entry);
                        current_entry = next_entry;
                }

                free(current_section);                
                current_section = next_section;
        }

        free(head);
}

int pcem_config_set(void *handle, int type, const char* section, const char* key, void *value)
{
        list_t *h = (list_t *) handle;
        section_t *s;
        entry_t *entry;

        s = find_section(h, section);

        if (!s)
                s = create_section(h, section);
                
        entry = find_entry(s, key);

        if (!entry)
                entry = create_entry(s, key);

        if (type == PCEM_CONFIG_TYPE_STRING)
                strncpy(entry->data, (const char *)value, 256);
        else if (type == PCEM_CONFIG_TYPE_INT)
                sprintf(entry->data, "%i", *((int *)value));
        else if (type == PCEM_CONFIG_TYPE_FLOAT)
                sprintf(entry->data, "%f", *((float *)value));
        
        return 1;
}

int pcem_config_get(void *handle, int type, const char* section, const char* key, void *value)
{
        list_t *head = (list_t *) handle;
        section_t *s;
        entry_t *entry;

        s = find_section(head, section);
        
        if (!s)
                return 0;
                
        entry = find_entry(s, key);

        if (!entry)
                return 0;
        
        if (type == PCEM_CONFIG_TYPE_STRING)
                strncpy((char *)value, entry->data, 256);
        else if (type == PCEM_CONFIG_TYPE_INT)
                sscanf(entry->data, "%i", (int *)value);
        else if (type == PCEM_CONFIG_TYPE_FLOAT)
                sscanf(entry->data, "%f", (float *)value);

        return 1;
}

char pcem_home[512];
void *global_config_handle;
void *machine_config_handle;

char machine_config_path[512];
char global_config_path[512];

void get_pcem_path(char *s, int size)
{
        strncpy(s, pcem_home, 512);
}

int config_get_value_impl(int type, int is_global, const char *section, const char *name, void *value)
{
        void *handle = is_global ? global_config_handle : machine_config_handle;

        return pcem_config_get(handle, type, section, name, value);
}

int config_set_value_impl(int type, int is_global, const char *section, const char *name, void *value)
{
        void *handle = is_global ? global_config_handle : machine_config_handle;

        return pcem_config_set(handle, type, section, name, value);
}

void config_save_impl(int is_global)
{
        void *handle = is_global ? global_config_handle : machine_config_handle;
        const char *path = is_global ? global_config_path : machine_config_path;
        
        pcem_config_save(handle, path);
}

int pcem_config_simple_init(const char *global_config, const char *machine_config, int read_only)
{
        strncpy(global_config_path, global_config, 512);
        strncpy(machine_config_path, machine_config, 512);
        global_config_handle = pcem_config_load(global_config_path);
        if (!global_config_handle)
                return 1;
        machine_config_handle = pcem_config_load(machine_config_path);
        if (!machine_config_handle)
        {
                pcem_config_free(global_config_handle);
                return 2;
        }

        pcem_callback_config_get(config_get_value_impl);
        pcem_callback_config_set(config_set_value_impl);
        if (!read_only) pcem_callback_config_save(config_save_impl);

        strncpy(pcem_home, global_config_path, 512);

        char *p = get_filename(pcem_home);
        *p = 0;

        paths_init();
        
        return 0;
}

void pcem_config_simple_close()
{
        if (global_config_handle)
                pcem_config_free(global_config_handle);
        global_config_handle = 0;
        if (machine_config_handle)
                pcem_config_free(machine_config_handle);
        machine_config_handle = 0;
}
