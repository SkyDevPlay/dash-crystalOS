#include "fat.h"
#include "sys/lba.h"
#include "io.h"
#include "malloc.h"
#include "string.h"
#define MIN(x,y) ((x)<(y)?(x):(y))

struct fat_attr {
    u8 ReadOnly   : 1;
    u8 Hidden     : 1;
    u8 System     : 1;
    u8 VolumeName : 1;
    u8 Directory  : 1;
    u8 AchieveFlag: 1;
    u8 Reserved   : 2;
};

struct Date {
    u8 Day:   5;
    u8 Month: 4;
    u8 Year:  7;
};

struct Time {
    u8 Second: 5;
    u8 Minute: 6;
    u8 Hour:   5;
};

struct fat_dir_entry {
    char filename[8];
    char extension[3];
    struct fat_attr attr;
    u8 resv;
    u8 c_millis;
    struct Time c_time;
    struct Date c_date;
    struct Date a_date;
    u16 resv2;
    struct Time w_time;
    struct Date w_date;
    u16 cluster;
    u32 filesize;
};

static struct fat_bootsector *bs;
static u16                   *fat;
static struct fat_dir_entry  *root_entries;

static u32 data_addr;
static u32 fat_addr;
static u32 root_addr;
static u32 bytes_per_cluster;

static u16 find_free_cluster(void);

static u32 sectors_for_bytes(u32 bytes) { return (bytes + 511) / 512; }
static int is_eoc(u16 cluster) { return cluster >= 0xFFF8; }
static int valid_cluster(u16 cluster) {
    return bs && cluster >= 2 && cluster < 0xFFF7 &&
           cluster < (u32)bs->sectors_per_fat * bs->bytes_per_sector / sizeof(*fat);
}

int init_fs(u32 start_sector) {
    printf("init_fs: start_sector=%d\n", start_sector);

    bs = (struct fat_bootsector *)malloc(512);
    if (!bs) { printf("init_fs: bs malloc failed\n"); return -1; }

    if (ata_lba_read(start_sector, 1, bs) < 0) return -1;
    printf("init_fs: bs read, jump=%x bps=%d\n", bs->code_jump[0], bs->bytes_per_sector);

    if (!bs->code_jump[0]) { printf("init_fs: bad jump\n"); return -1; }
    if (bs->bytes_per_sector != 512 || !bs->sectors_per_cluster ||
        !bs->fat_count || !bs->sectors_per_fat) { printf("init_fs: invalid BPB\n"); return -2; }

    fat_addr  = start_sector + bs->resv_sectors;
    root_addr = fat_addr + bs->sectors_per_fat * bs->fat_count;
    data_addr = root_addr + sectors_for_bytes(bs->root_entry_count * sizeof(struct fat_dir_entry));
    bytes_per_cluster = bs->sectors_per_cluster * bs->bytes_per_sector;
    printf("init_fs: fat@%d root@%d data@%d bpc=%d\n", fat_addr, root_addr, data_addr, bytes_per_cluster);

    u32 fat_sectors = bs->sectors_per_fat;
    fat = (u16 *)malloc(fat_sectors * 512);
    if (!fat) { printf("init_fs: fat malloc failed\n"); return -1; }
    if (ata_lba_read(fat_addr, fat_sectors, fat) < 0) return -1;

    u32 root_sectors = sectors_for_bytes(bs->root_entry_count * sizeof(struct fat_dir_entry));
    root_entries = (struct fat_dir_entry *)malloc(root_sectors * 512);
    if (!root_entries) { printf("init_fs: root malloc failed\n"); return -1; }
    if (ata_lba_read(root_addr, root_sectors, root_entries) < 0) return -1;
    return 0;
}

static int sync_fat_sector(u16 cluster) {
    u32 byte_off   = (u32)cluster * sizeof(*fat);
    u32 sector_off = byte_off / bs->bytes_per_sector;
    u8 *sector_ptr = (u8 *)fat + sector_off * bs->bytes_per_sector;
    for (u8 copy = 0; copy < bs->fat_count; copy++)
        if (ata_lba_write(fat_addr + copy * bs->sectors_per_fat + sector_off, 1, sector_ptr) < 0)
            return -1;
    return 0;
}

static int sync_dir_entry(int idx) {
    u32 byte_off   = (u32)idx * sizeof(struct fat_dir_entry);
    u32 sector_off = byte_off / bs->bytes_per_sector;
    u8 *sector_ptr = (u8 *)root_entries + sector_off * bs->bytes_per_sector;
    return ata_lba_write(root_addr + sector_off, 1, sector_ptr);
}

static void format_entry(struct fat_dir_entry entry, char buf[13]) {
    int n = 8, e = 3, p = 0;
    while (n && entry.filename[n - 1] == ' ') n--;
    while (e && entry.extension[e - 1] == ' ') e--;
    for (int i = 0; i < n; i++) buf[p++] = entry.filename[i];
    if (e) buf[p++] = '.';
    for (int i = 0; i < e; i++) buf[p++] = entry.extension[i];
    buf[p] = 0;
}

static int filename_match(char *f1, char *f2) {
    while (*f1 || *f2) {
        char c1 = *f1;
        char c2 = *f2;
        if (c1 >= 'a' && c1 <= 'z') c1 -= 32;
        if (c2 >= 'a' && c2 <= 'z') c2 -= 32;
        if (c1 != c2) return 0;
        f1++;
        f2++;
    }
    return 1;
}

int read_file(char *filename) {
    char fname[13];
    int idx = -1;
    for (int i = 0; i < (int)bs->root_entry_count; i++) {
        struct fat_dir_entry entry = root_entries[i];
        format_entry(entry, fname);
        if (filename_match(fname, filename) && !entry.attr.Directory) {
            idx = i;
            break;
        }
    }
    if (idx < 0) return -1;

    u16 cur_cluster = root_entries[idx].cluster;
    u32 filesize    = root_entries[idx].filesize;
    if (filesize == 0) return 0;
    u8 *buf = malloc(bytes_per_cluster);
    if (!buf) return -1;

    while (filesize && valid_cluster(cur_cluster)) {
        if (ata_lba_read(data_addr + (cur_cluster - 2) * bs->sectors_per_cluster,
                         bs->sectors_per_cluster, buf) < 0) {
            free(buf);
            return -1;
        }
        u32 to_read = filesize > bytes_per_cluster ? bytes_per_cluster : filesize;
        for (u32 i = 0; i < to_read; i++) printf("%c", buf[i]);
        cur_cluster = fat[cur_cluster];
        filesize   -= to_read;
        if (is_eoc(cur_cluster)) break;
    }
    free(buf);
    return 0;
}

static void format_attrs(struct fat_attr attrs, char *buf) {
    buf[0] = attrs.ReadOnly    ? 'R' : '-';
    buf[1] = attrs.Hidden      ? 'H' : '-';
    buf[2] = attrs.System      ? 'S' : '-';
    buf[3] = attrs.VolumeName  ? 'V' : '-';
    buf[4] = attrs.Directory   ? 'D' : '-';
    buf[5] = attrs.AchieveFlag ? 'A' : '-';
    buf[6] = (attrs.Reserved & 0b01) ? 'x' : '-';
    buf[7] = (attrs.Reserved & 0b10) ? 'y' : '-';
    buf[8] = 0;
}

int list_dir(void) {
    char attrs[9];
    char name[13];
    for (int i = 0; i < (int)bs->root_entry_count; i++) {
        struct fat_dir_entry entry = root_entries[i];
        if (entry.filename[0] == 0) break;
        if ((u8)entry.filename[0] == 0xE5) continue;
        if ((u8)entry.attr == 0x0F) continue; /* Long-file-name helper entry. */
        format_entry(entry, name);
        format_attrs(entry.attr, attrs);
        printf("%s %d-%d-%d %d:%d:%d %s\n",
            attrs,
            entry.c_date.Year + 1980, entry.c_date.Month, entry.c_date.Day,
            entry.c_time.Hour, entry.c_time.Minute, entry.c_time.Second * 2,
            name);
    }
    return 0;
}

FILE *fopen(char *path, char *mode) {
    char fname[13];
    int idx = -1;
    for (int i = 0; i < (int)bs->root_entry_count; i++) {
        struct fat_dir_entry entry = root_entries[i];
        format_entry(entry, fname);
        if (filename_match(fname, path) && !entry.attr.Directory) {
            idx = i;
            break;
        }
    }
    
    if (idx < 0) {
        if (mode && mode[0] == 'w') {
            idx = create_file(path);
            if (idx < 0) return 0;
        } else {
            return 0;
        }
    }

    FILE *file = malloc(sizeof(FILE));
    if (!file) return 0;
    file->cluster   = root_entries[idx].cluster;
    file->filesize  = root_entries[idx].filesize;
    file->offset    = 0;
    file->dir_index = idx;
    if (mode && mode[0] == 'w') {
        file->filesize = 0;
        root_entries[idx].filesize = 0;
        if (sync_dir_entry(idx) < 0) {
            free(file);
            return 0;
        }
    }
    return file;
}

static u16 get_nth_cluster(u16 start, u16 count) {
    u16 cur = start;
    while (count--) {
        if (cur == 0xFFFF) return 0xFFFF;
        cur = fat[cur];
    }
    return cur;
}

u32 fread(void *buf, u32 size, u32 n, FILE *stream) {
    if (!stream || !buf || !size || !n) return 0;
    u8 *buffer  = malloc(bytes_per_cluster);
    if (!buffer) return 0;
    u32 to_read = n * size;
    u16 cur_cluster = get_nth_cluster(stream->cluster,
                                      stream->offset / bytes_per_cluster);
    u32 offset = stream->offset % bytes_per_cluster;

    while (to_read && valid_cluster(cur_cluster)) {
        if (stream->filesize == stream->offset) break;
        u32 r = MIN(to_read, bytes_per_cluster - offset);
        r = MIN(r, stream->filesize - stream->offset);
        if (ata_lba_read(data_addr + (cur_cluster - 2) * bs->sectors_per_cluster,
                         bs->sectors_per_cluster, buffer) < 0) break;
        memcpy(buf, buffer + offset, r);
        buf = (u8 *)buf + r;
        to_read        -= r;
        stream->offset += r;
        offset = (offset + r) % bytes_per_cluster;
        cur_cluster = fat[cur_cluster];
        if (is_eoc(cur_cluster)) break;
    }
    free(buffer);
    return n * size - to_read;
}

int fseek(FILE *stream, long offset, int whence) {
    long new_offset;
    if (!stream) return -1;
    switch (whence) {
        case SEEK_SET: new_offset = offset; break;
        case SEEK_CUR: new_offset = (long)stream->offset + offset; break;
        case SEEK_END: new_offset = (long)stream->filesize + offset; break;
        default: return -1;
    }
    if (new_offset < 0 || (u32)new_offset > stream->filesize) return -1;
    stream->offset = (u32)new_offset;
    return 0;
}

int fclose(FILE *stream) {
    free(stream);
    return 0;
}

static u16 allocate_cluster(u16 current) {
    for (unsigned int i = 2; i < bs->sectors_per_fat * bs->bytes_per_sector / sizeof(*fat); i++) {
        if (fat[i] == 0x0000) {
            fat[current] = i;
            fat[i] = 0xFFFF;
            return i;
        }
    }
    return 0;
}

u32 fwrite(void *ptr, u32 size, u32 n, FILE *stream) {
    if (!stream || !ptr || !size || !n) return 0;
    if (!valid_cluster(stream->cluster)) {
        if (stream->filesize || stream->offset) return 0;
        u16 cluster = find_free_cluster();
        if (cluster == 0xFFFF) return 0;
        fat[cluster] = 0xFFFF;
        stream->cluster = cluster;
        root_entries[stream->dir_index].cluster = cluster;
        if (sync_fat_sector(cluster) < 0 || sync_dir_entry(stream->dir_index) < 0) return 0;
    }
    u8 *buffer  = malloc(bytes_per_cluster);
    if (!buffer) return 0;
    u32 to_write = n * size;
    u16 cur_cluster = get_nth_cluster(stream->cluster,
                                      stream->offset / bytes_per_cluster);
    u32 offset = stream->offset % bytes_per_cluster;

    while (to_write && valid_cluster(cur_cluster)) {
        u32 r = MIN(to_write, bytes_per_cluster - offset);
        if (ata_lba_read(data_addr + (cur_cluster - 2) * bs->sectors_per_cluster,
                         bs->sectors_per_cluster, buffer) < 0) break;
        memcpy(buffer + offset, ptr, r);
        if (ata_lba_write(data_addr + (cur_cluster - 2) * bs->sectors_per_cluster,
                          bs->sectors_per_cluster, buffer) < 0) break;
        ptr = (u8 *)ptr + r;
        to_write       -= r;
        stream->offset += r;
        offset = (offset + r) % bytes_per_cluster;
        if (is_eoc(fat[cur_cluster]) && to_write) {
            u16 prev = cur_cluster;
            cur_cluster = allocate_cluster(cur_cluster);
            if (!cur_cluster) break;
            if (sync_fat_sector(prev) < 0 || sync_fat_sector(cur_cluster) < 0) break;
        } else {
            cur_cluster = fat[cur_cluster];
        }
    }
    free(buffer);
    if (stream->offset > stream->filesize) {
        stream->filesize = stream->offset;
        if (stream->dir_index >= 0) {
            root_entries[stream->dir_index].filesize = stream->filesize;
            if (sync_dir_entry(stream->dir_index) < 0) return 0;
        }
    }
    return n * size - to_write;
}

static int find_empty_dir_entry(void) {
    for (int i = 0; i < (int)bs->root_entry_count; i++) {
        u8 c = (u8)root_entries[i].filename[0];
        if (c == 0x00 || c == 0xE5) {
            return i;
        }
    }
    return -1;
}

static u16 find_free_cluster(void) {
    u32 total_clusters = bs->sectors_per_fat * 512 / 2; // FAT16 uses 2 bytes per cluster entry
    for (u16 i = 2; i < total_clusters; i++) {
        if (fat[i] == 0x0000) {
            return i;
        }
    }
    return 0xFFFF;
}

static void convert_to_fat_name(const char *src, char *dest_name, char *dest_ext) {
    memset(dest_name, ' ', 8);
    memset(dest_ext, ' ', 3);
    
    int i = 0;

    while (src[i] && src[i] != '.' && i < 8) {
        char c = src[i];
        if (c >= 'a' && c <= 'z') c -= 32;
        dest_name[i] = c;
        i++;
    }

    while (src[i] && src[i] != '.') i++;
    

    if (src[i] == '.') {
        i++;
        int j = 0;
        while (src[i] && j < 3) {
            char c = src[i];
            if (c >= 'a' && c <= 'z') c -= 32; 
            dest_ext[j] = c;
            i++;
            j++;
        }
    }
}

int create_file(char *filename) {
    int dir_idx = find_empty_dir_entry();
    if (dir_idx < 0) return -1;

    u16 cluster = find_free_cluster();
    if (cluster == 0xFFFF) return -2;
    
    struct fat_dir_entry *entry = &root_entries[dir_idx];
    memset(entry, 0, sizeof(struct fat_dir_entry));

    convert_to_fat_name(filename, entry->filename, entry->extension);
    entry->cluster = cluster;
    entry->filesize = 0;
    entry->attr.ReadOnly = 0;
    entry->attr.Hidden = 0;
    entry->attr.System = 0;
    entry->attr.Directory = 0;
    entry->attr.AchieveFlag = 1;
    
    fat[cluster] = 0xFFFF;
    
    if (sync_fat_sector(cluster) < 0 || sync_dir_entry(dir_idx) < 0) return -3;
    
    return dir_idx;
}

int set_file_date(char *filename, int year, int month, int day) {
    char fname[13];
    for (int i = 0; i < (int)bs->root_entry_count; i++) {
        format_entry(root_entries[i], fname);
        if (filename_match(fname, filename)) {
            root_entries[i].c_date.Year  = year - 1980;
            root_entries[i].c_date.Month = month;
            root_entries[i].c_date.Day   = day;
            sync_dir_entry(i);
            return 0;
        }
    }
    return -1;
}
