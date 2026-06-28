#include "fat.h"
#include "sys/lba.h"
#include "io.h"
#include "malloc.h"
#include "string.h"
#define MIN(x,y) ((x)<(y)?(x):(y))
#define FAT_BS_ADDR    0x9000
#define FAT_TABLE_ADDR 0x9200
#define FAT_ROOT_ADDR  0x11200

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

static void clean_entry(struct fat_dir_entry *entry) {
    for (int i = 0; i < 8; i++)
        if (entry->filename[i] == ' ') entry->filename[i] = 0;
    for (int i = 0; i < 3; i++)
        if (entry->extension[i] == ' ') entry->extension[i] = 0;
}

int init_fs(u32 start_sector) {
    bs          = (struct fat_bootsector *)FAT_BS_ADDR;
    fat         = (u16 *)FAT_TABLE_ADDR;
    root_entries = (struct fat_dir_entry *)FAT_ROOT_ADDR;

    if (!bs->code_jump[0]) return -1;
    if (bs->bytes_per_sector != 512) return -2;

    fat_addr            = start_sector + bs->resv_sectors;
    root_addr           = fat_addr + bs->sectors_per_fat * bs->fat_count;
    data_addr           = root_addr + (bs->root_entry_count * sizeof(struct fat_dir_entry)) / 512;
    bytes_per_cluster   = bs->sectors_per_cluster * bs->bytes_per_sector;

    for (int i = 0; i < bs->root_entry_count; i++)
        clean_entry(&root_entries[i]);

    return 0;
}

static void sync_fat_sector(u16 cluster) {
    u32 byte_off   = (u32)cluster * sizeof(*fat);
    u32 sector_off = byte_off / bs->bytes_per_sector;
    u8 *sector_ptr = (u8 *)fat + sector_off * bs->bytes_per_sector;
    ata_lba_write(fat_addr + sector_off, 1, sector_ptr);
}

static void sync_dir_entry(int idx) {
    u32 byte_off   = (u32)idx * sizeof(struct fat_dir_entry);
    u32 sector_off = byte_off / bs->bytes_per_sector;
    u8 *sector_ptr = (u8 *)root_entries + sector_off * bs->bytes_per_sector;
    ata_lba_write(root_addr + sector_off, 1, sector_ptr);
}

static void format_entry(struct fat_dir_entry entry, char buf[13]) {
    memset(buf, 0, 13);
    strncpy(buf, entry.filename, 8);
    if (!entry.extension[0]) return;
    strcat(buf, ".");
    strncat(buf, entry.extension, 3);
}

int read_file(char *filename) {
    char fname[13];
    int idx = -1;
    for (int i = 0; i < (int)bs->root_entry_count; i++) {
        struct fat_dir_entry entry = root_entries[i];
        format_entry(entry, fname);
        if (strcmp(fname, filename) == 0 && !entry.attr.Directory) {
            idx = i;
            break;
        }
    }
    if (idx < 0) return -1;

    u16 cur_cluster = root_entries[idx].cluster;
    u32 filesize    = root_entries[idx].filesize;
    u8 *buf = malloc(bytes_per_cluster);
    if (!buf) return -1;

    while (cur_cluster != 0xFFFF) {
        ata_lba_read(data_addr + (cur_cluster - 2) * bs->sectors_per_cluster,
                     bs->sectors_per_cluster, buf);
        u32 to_read = filesize > bytes_per_cluster ? bytes_per_cluster : filesize;
        for (u32 i = 0; i < to_read; i++) printf("%c", buf[i]);
        cur_cluster = fat[cur_cluster];
        filesize   -= to_read;
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
    for (int i = 0; i < (int)bs->root_entry_count; i++) {
        struct fat_dir_entry entry = root_entries[i];
        if (entry.filename[0] == 0) break;
        format_attrs(entry.attr, attrs);
        printf(
            entry.attr.Directory || *entry.extension == 0
                ? "%s %d-%d-%d %d:%d:%d %s\n"
                : "%s %d-%d-%d %d:%d:%d %s.%s\n",
            attrs,
            entry.c_date.Year + 1980, entry.c_date.Month, entry.c_date.Day,
            entry.c_time.Hour, entry.c_time.Minute, entry.c_time.Second * 2,
            entry.filename, entry.extension);
    }
    return 0;
}

FILE *fopen(char *path, char *mode) {
    (void)mode;
    char fname[13];
    int idx = -1;
    for (int i = 0; i < (int)bs->root_entry_count; i++) {
        struct fat_dir_entry entry = root_entries[i];
        format_entry(entry, fname);
        if (strcmp(fname, path) == 0 && !entry.attr.Directory) {
            idx = i;
            break;
        }
    }
    if (idx < 0) return 0;

    FILE *file = malloc(sizeof(FILE));
    file->cluster   = root_entries[idx].cluster;
    file->filesize  = root_entries[idx].filesize;
    file->offset    = 0;
    file->dir_index = idx;
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
    u8 *buffer  = malloc(bytes_per_cluster);
    u32 to_read = n * size;
    u16 cur_cluster = get_nth_cluster(stream->cluster,
                                      stream->offset / bytes_per_cluster);
    u32 offset = stream->offset % bytes_per_cluster;

    while (to_read) {
        if (stream->filesize == stream->offset) break;
        u32 r = MIN(to_read, bytes_per_cluster - offset);
        r = MIN(r, stream->filesize - stream->offset);
        ata_lba_read(data_addr + (cur_cluster - 2) * bs->sectors_per_cluster,
                     bs->sectors_per_cluster, buffer);
        memcpy(buf, buffer + offset, r);
        buf = (u8 *)buf + r;
        to_read        -= r;
        stream->offset += r;
        offset = (offset + r) % bytes_per_cluster;
        cur_cluster = fat[cur_cluster];
    }
    free(buffer);
    return n * size - to_read;
}

int fseek(FILE *stream, long offset, int whence) {
    switch (whence) {
        case SEEK_SET: stream->offset = offset; break;
        case SEEK_CUR: stream->offset += offset; break;
        case SEEK_END: stream->offset = stream->filesize - offset; break;
        default: return -1;
    }
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
    u8 *buffer  = malloc(bytes_per_cluster);
    u32 to_write = n * size;
    u16 cur_cluster = get_nth_cluster(stream->cluster,
                                      stream->offset / bytes_per_cluster);
    u32 offset = stream->offset % bytes_per_cluster;

    while (to_write) {
        u32 r = MIN(to_write, bytes_per_cluster - offset);
        ata_lba_read(data_addr + (cur_cluster - 2) * bs->sectors_per_cluster,
                     bs->sectors_per_cluster, buffer);
        memcpy(buffer + offset, ptr, r);
        ata_lba_write(data_addr + (cur_cluster - 2) * bs->sectors_per_cluster,
                      bs->sectors_per_cluster, buffer);
        ptr = (u8 *)ptr + r;
        to_write       -= r;
        stream->offset += r;
        offset = (offset + r) % bytes_per_cluster;
        if (fat[cur_cluster] == 0xFFFF && to_write) {
            u16 prev = cur_cluster;
            cur_cluster = allocate_cluster(cur_cluster);
            if (!cur_cluster) break;
            /* Persiste les deux entrées FAT modifiées (prev -> cur_cluster, et le EOF de cur_cluster) */
            sync_fat_sector(prev);
            sync_fat_sector(cur_cluster);
        } else {
            cur_cluster = fat[cur_cluster];
        }
    }
    free(buffer);
    if (stream->offset > stream->filesize) {
        stream->filesize = stream->offset;
        if (stream->dir_index >= 0) {
            root_entries[stream->dir_index].filesize = stream->filesize;
            sync_dir_entry(stream->dir_index);
        }
    }
    return n * size - to_write;
}
