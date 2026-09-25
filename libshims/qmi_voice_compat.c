#include <android/log.h>
#include <dlfcn.h>
#include <errno.h>
#include <fcntl.h>
#include <link.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>

/* QCRIL first connects to pm-service over /dev/vndbinder. Android 13's
 * libhardware_legacy then searches for SystemSuspend on the same Binder
 * driver, although the service is registered on /dev/binder. Use the legacy
 * kernel wakelock interface here, which this device still exposes to radio. */
static int write_wakelock(const char *path, const char *id)
{
    int fd;
    size_t length;
    ssize_t written;
    int saved_errno;

    if (id == NULL || *id == '\0') {
        errno = EINVAL;
        return -1;
    }

    fd = open(path, O_WRONLY | O_CLOEXEC);
    if (fd < 0)
        return -1;

    length = strlen(id);
    written = write(fd, id, length);
    saved_errno = errno;
    close(fd);
    if (written != (ssize_t)length) {
        errno = written < 0 ? saved_errno : EIO;
        return -1;
    }
    return 0;
}

int acquire_wake_lock(int lock, const char *id)
{
    (void)lock;
    return write_wakelock("/sys/power/wake_lock", id);
}

int release_wake_lock(const char *id)
{
    return write_wakelock("/sys/power/wake_unlock", id);
}

typedef int (*qmi_send_async_fn)(void *, unsigned int, void *, unsigned int,
                                 void *, unsigned int, void *, void *, void *);

static uintptr_t loaded_address(const struct dl_phdr_info *info, uintptr_t value)
{
    return value < info->dlpi_addr ? info->dlpi_addr + value : value;
}

static size_t gnu_symbol_count(const uint32_t *hash)
{
    uint32_t buckets_count = hash[0];
    uint32_t first_symbol = hash[1];
    uint32_t bloom_count = hash[2];
    const ElfW(Addr) *bloom = (const ElfW(Addr) *)(hash + 4);
    const uint32_t *buckets = (const uint32_t *)(bloom + bloom_count);
    const uint32_t *chains = buckets + buckets_count;
    size_t count = first_symbol;

    if (buckets_count > 4096 || bloom_count > 4096 || first_symbol > 4096)
        return 0;

    for (uint32_t i = 0; i < buckets_count; ++i) {
        uint32_t symbol = buckets[i];
        if (symbol < first_symbol)
            continue;
        while (symbol < 4096) {
            uint32_t chain = chains[symbol - first_symbol];
            if ((size_t)symbol + 1 > count)
                count = (size_t)symbol + 1;
            if (chain & 1)
                break;
            ++symbol;
        }
        if (symbol == 4096)
            return 0;
    }
    return count;
}

static int find_qmi_cci(struct dl_phdr_info *info, size_t size, void *data)
{
    const ElfW(Dyn) *dynamic = NULL;
    const ElfW(Sym) *symbols = NULL;
    const char *strings = NULL;
    const uint32_t *gnu_hash = NULL;
    const uint32_t *sysv_hash = NULL;
    size_t symbol_count = 0;
    size_t strings_size = 0;
    const char *name;

    (void)size;
    name = strrchr(info->dlpi_name, '/');
    name = name ? name + 1 : info->dlpi_name;
    if (strcmp(name, "libqmi_cci.so") != 0)
        return 0;

    for (ElfW(Half) i = 0; i < info->dlpi_phnum; ++i) {
        if (info->dlpi_phdr[i].p_type == PT_DYNAMIC) {
            dynamic = (const ElfW(Dyn) *)(info->dlpi_addr +
                                           info->dlpi_phdr[i].p_vaddr);
            break;
        }
    }
    if (dynamic == NULL)
        return 1;

    for (const ElfW(Dyn) *entry = dynamic; entry->d_tag != DT_NULL; ++entry) {
        switch (entry->d_tag) {
        case DT_SYMTAB:
            symbols = (const ElfW(Sym) *)loaded_address(info, entry->d_un.d_ptr);
            break;
        case DT_STRTAB:
            strings = (const char *)loaded_address(info, entry->d_un.d_ptr);
            break;
        case DT_STRSZ:
            strings_size = entry->d_un.d_val;
            break;
        case DT_GNU_HASH:
            gnu_hash = (const uint32_t *)loaded_address(info, entry->d_un.d_ptr);
            break;
        case DT_HASH:
            sysv_hash = (const uint32_t *)loaded_address(info, entry->d_un.d_ptr);
            break;
        }
    }
    if (symbols == NULL || strings == NULL || strings_size == 0)
        return 1;

    if (sysv_hash != NULL)
        symbol_count = sysv_hash[1];
    else if (gnu_hash != NULL)
        symbol_count = gnu_symbol_count(gnu_hash);
    if (symbol_count > 4096)
        symbol_count = 0;

    for (size_t i = 0; i < symbol_count; ++i) {
        if (symbols[i].st_shndx == SHN_UNDEF ||
            symbols[i].st_name >= strings_size)
            continue;
        if (strcmp(strings + symbols[i].st_name,
                   "qmi_client_send_msg_async") == 0) {
            *(qmi_send_async_fn *)data =
                (qmi_send_async_fn)(info->dlpi_addr + symbols[i].st_value);
            break;
        }
    }
    return 1;
}

/* The Kenzo RIL adds Voice DIAL extension fields that the Libra modem rejects
 * as QMI_ERR_MALFORMED_MSG. Mask only those fields for ordinary voice calls. */
int qmi_client_send_msg_async(void *client, unsigned int msg_id,
                              void *req, unsigned int req_len,
                              void *resp, unsigned int resp_len,
                              void *callback, void *cookie, void *txn)
{
    static qmi_send_async_fn real_send;
    uint8_t *bytes = req;
    int32_t call_type;

    if (real_send == NULL) {
        real_send = (qmi_send_async_fn)dlsym(RTLD_NEXT,
                                            "qmi_client_send_msg_async");
        if (real_send == NULL)
            dl_iterate_phdr(find_qmi_cci, &real_send);
        if (real_send == NULL) {
            __android_log_print(ANDROID_LOG_ERROR, "LibraVoiceQMI",
                                "cannot resolve qmi_client_send_msg_async");
            return -1;
        }
    }

    if (bytes != NULL && msg_id == 0x20 && req_len == 2296 &&
        ((bytes[0] >= '0' && bytes[0] <= '9') ||
         bytes[0] == '+' || bytes[0] == '*' || bytes[0] == '#') &&
        bytes[82] == 1) {
        memcpy(&call_type, bytes + 84, sizeof(call_type));
        if (call_type == 0)
            memset(bytes + 2284, 0, req_len - 2284);
    }

    return real_send(client, msg_id, req, req_len, resp, resp_len,
                     callback, cookie, txn);
}
