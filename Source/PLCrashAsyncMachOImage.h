/*
 * Author: Landon Fuller <landonf@plausible.coop>
 *
 * Copyright (c) 2011-2013 Plausible Labs Cooperative, Inc.
 * All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef PLCrashAsyncMachOImage_h
#define PLCrashAsyncMachOImage_h

#include <stdint.h>
#include <mach/mach.h>
#include <mach-o/loader.h>
#include <mach-o/nlist.h>

#include "PLCrashAsyncMObject.h"

/**
 * @internal
 * @ingroup plcrash_async_image
 * @{
 */

/**
 * A Mach-O image instance.
 */
typedef struct plcrash_async_macho {    
    /** The Mach task in which the Mach-O image can be found */
    mach_port_t task;

    /** The binary image's header address. */
    pl_vm_address_t header_addr;
    
    /** The binary's dyld-reported reported vmaddr slide. */
    int64_t vmaddr_slide;

    /** The binary image's name/path. */
    char *name;

    /** The Mach-O header. For our purposes, the 32-bit and 64-bit headers are identical. Note that the header
     * values may require byte-swapping for the local process' use. */
    struct mach_header header;
    
    /** Total size, in bytes, of the in-memory Mach-O header. This may differ from the header field above,
     * as the above field does not include the full mach_header_64 extensions to the mach_header. */
    pl_vm_size_t header_size;

    /** Number of load commands */
    uint32_t ncmds;

    /** Mapped Mach-O load commands */
    plcrash_async_mobject_t load_cmds;

    /** Total size, in bytes, of the Mach-O image's __TEXT segment. */
    pl_vm_size_t text_size;

    /** If true, the image is 64-bit Mach-O. If false, it is a 32-bit Mach-O image. */
    bool m64;

    /** The byte order functions to use for this image */
    const plcrash_async_byteorder_t *byteorder;
} plcrash_async_macho_t;

/**
 * A mapped Mach-O segment.
 */
typedef struct plcrash_async_macho_mapped_segment_t {
    /** The segment's mapped memory object */
    plcrash_async_mobject_t mobj;

    /* File offset of this segment. */
    uint64_t	fileoff;
    
    /* File size of the segment. */
	uint64_t	filesize;
} pl_async_macho_mapped_segment_t;

/**
 * A 32-bit/64-bit neutral symbol table entry.
 */
typedef struct plcrash_async_macho_symtab_entry {
    /* Index into the string table */
    uint32_t n_strx;

    /** Symbol type. */
    uint8_t n_type;

    /** Section number. */
    uint8_t n_sect;

    /** Description (see <mach-o/stab.h>). */
    uint16_t n_desc;

    /** Symbol value */
    pl_vm_address_t n_value;
} plcrash_async_macho_symtab_entry_t;

/**
 * A Mach-O symtab reader. Provides support for iterating the contents of a Mach-O symbol table.
 */
typedef struct plcrash_async_macho_symtab_reader {
    /** The mapped LINKEDIT segment. */
    pl_async_macho_mapped_segment_t linkedit;

    /** Pointer to the symtab table within the mapped linkedit segment. The validity of this pointer (and the length of
     * data available) is gauranteed. */
    void *symtab;
    
    /** Total number of elements in the symtab. */
    uint32_t nsyms;
    
    /** Pointer to the global symbol table, if available. May be NULL. The validity of this pointer (and the length of
     * data available) is gauranteed. */
    void *symtab_global;

    /** Total number of elements in the global symtab. */
    uint32_t nsyms_global;

    /** Pointer to the local symbol table, if available. May be NULL. The validity of this pointer (and the length of
     * data available) is gauranteed. */
    void *symtab_local;

    /** Total number of elements in the local symtab. */
    uint32_t nsyms_local;
} plcrash_async_macho_symtab_reader_t;

/**
 * Prototype of a callback function used to execute user code with async-safely fetched symbol.
 *
 * @param address The symbol address.
 * @param name The symbol name. The callback is responsible for copying this value, as its backing storage is not gauranteed to exist
 * after the callback returns.
 * @param context The API client's supplied context value.
 */
typedef void (*pl_async_macho_found_symbol_cb)(pl_vm_address_t address, const char *name, void *ctx);

plcrash_error_t plcrash_nasync_macho_init (plcrash_async_macho_t *image, mach_port_t task, const char *name, pl_vm_address_t header);

void *plcrash_async_macho_next_command (plcrash_async_macho_t *image, void *previous);
void *plcrash_async_macho_next_command_type (plcrash_async_macho_t *image, void *previous, uint32_t expectedCommand);
void *plcrash_async_macho_find_command (plcrash_async_macho_t *image, uint32_t cmd);
void *plcrash_async_macho_find_segment_cmd (plcrash_async_macho_t *image, const char *segname);

plcrash_error_t plcrash_async_macho_map_segment (plcrash_async_macho_t *image, const char *segname, pl_async_macho_mapped_segment_t *seg);
plcrash_error_t plcrash_async_macho_map_section (plcrash_async_macho_t *image, const char *segname, const char *sectname, plcrash_async_mobject_t *mobj);
plcrash_error_t plcrash_async_macho_find_symbol (plcrash_async_macho_t *image, pl_vm_address_t pc, pl_async_macho_found_symbol_cb symbol_cb, void *context);
plcrash_error_t plcrash_async_macho_find_symbol_pc (plcrash_async_macho_t *image, const char *symbol, pl_vm_address_t *pc);

plcrash_error_t plcrash_async_macho_symtab_reader_init (plcrash_async_macho_symtab_reader_t *reader, plcrash_async_macho_t *image);
void plcrash_async_macho_symtab_reader_free (plcrash_async_macho_symtab_reader_t *reader);

void plcrash_async_macho_mapped_segment_free (pl_async_macho_mapped_segment_t *segment);

void plcrash_nasync_macho_free (plcrash_async_macho_t *image);

/**
 * @}
 */

#endif // PLCrashAsyncMachOImage_h
