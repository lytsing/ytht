/*

buffer.c

Author: Tatu Ylonen <ylo@cs.hut.fi>

Copyright (c) 1995 Tatu Ylonen <ylo@cs.hut.fi>, Espoo, Finland
                   All rights reserved

Created: Sat Mar 18 04:15:33 1995 ylo

Functions for manipulating fifo buffers (that can grow if needed).

*/

/*
 * $Id: buffer.c 4882 2003-12-09 03:06:47Z yuhuan $
 * $Log$
 * Revision 1.2  2003/12/09 03:06:47  yuhuan
 * sync with smth
 *
 * Revision 1.1.1.1  2002/10/01 09:42:06  ylsdd
 * ˮľ��sshbbsd����
 * Ȼ�������İ�
 *
 * Revision 1.3  2002/08/04 11:39:40  kcn
 * format c
 *
 * Revision 1.2  2002/08/04 11:08:45  kcn
 * format C
 *
 * Revision 1.1.1.1  2002/04/27 05:47:26  kxn
 * no message
 *
 * Revision 1.1  2001/07/04 06:07:08  bbsdev
 * bbs sshd
 *
 * Revision 1.1.1.1  1996/02/18 21:38:11  ylo
 * 	Imported ssh-1.2.13.
 *
 * Revision 1.2  1995/07/13  01:18:46  ylo
 * 	Removed "Last modified" header.
 * 	Added cvs log.
 *
 * $Endlog$
 */

#include "includes.h"
#include "xmalloc.h"
#include "buffer.h"
#include "ssh.h"

/* Initializes the buffer structure. */

void buffer_init(Buffer * buffer)
{
    buffer->alloc = 4096;
    buffer->buf = xmalloc(buffer->alloc);
    buffer->offset = 0;
    buffer->end = 0;
}

/* Frees any memory used for the buffer. */

void buffer_free(Buffer * buffer)
{
    memset(buffer->buf, 0, buffer->alloc);
    xfree(buffer->buf);
}

/* Clears any data from the buffer, making it empty.  This does not actually
   zero the memory. */

void buffer_clear(Buffer * buffer)
{
    buffer->offset = 0;
    buffer->end = 0;
}

/* Appends data to the buffer, expanding it if necessary. */

void buffer_append(Buffer * buffer, const char *data, unsigned int len)
{
    char *cp;

    buffer_append_space(buffer, &cp, len);
    memcpy(cp, data, len);
}

/* Appends space to the buffer, expanding the buffer if necessary.
   This does not actually copy the data into the buffer, but instead
   returns a pointer to the allocated region. */

void buffer_append_space(Buffer * buffer, char **datap, unsigned int len)
{
    /* If the buffer is empty, start using it from the beginning. */
    if (buffer->offset == buffer->end) {
        buffer->offset = 0;
        buffer->end = 0;
    }

  restart:
    /* If there is enough space to store all data, store it now. */
    if (buffer->end + len < buffer->alloc) {
        *datap = buffer->buf + buffer->end;
        buffer->end += len;
        return;
    }

    /* If the buffer is quite empty, but all data is at the end, move the
       data to the beginning and retry. */
    if (buffer->offset > buffer->alloc / 2) {
        memmove(buffer->buf, buffer->buf + buffer->offset, buffer->end - buffer->offset);
        buffer->end -= buffer->offset;
        buffer->offset = 0;
        goto restart;
    }

    /* Increase the size of the buffer and retry. */
    buffer->alloc += len + 4096;
    buffer->buf = xrealloc(buffer->buf, buffer->alloc);
    if (buffer->alloc > 512 * 1024)
	log_msg("buffer size exceed twice of packet size");
    goto restart;
}

/* Returns the number of bytes of data in the buffer. */

unsigned int buffer_len(Buffer * buffer)
{
    return buffer->end - buffer->offset;
}

/* Gets data from the beginning of the buffer. */

void buffer_get(Buffer * buffer, char *buf, unsigned int len)
{
    if (len > buffer->end - buffer->offset)
        fatal("buffer_get trying to get more bytes than in buffer");
    memcpy(buf, buffer->buf + buffer->offset, len);
    buffer->offset += len;
}

/* Consumes the given number of bytes from the beginning of the buffer. */

void buffer_consume(Buffer * buffer, unsigned int bytes)
{
    if (bytes > buffer->end - buffer->offset)
        fatal("buffer_get trying to get more bytes than in buffer");
    buffer->offset += bytes;
}

/* Consumes the given number of bytes from the end of the buffer. */

void buffer_consume_end(Buffer * buffer, unsigned int bytes)
{
    if (bytes > buffer->end - buffer->offset)
        fatal("buffer_get trying to get more bytes than in buffer");
    buffer->end -= bytes;
}

/* Returns a pointer to the first used byte in the buffer. */

char *buffer_ptr(Buffer * buffer)
{
    return buffer->buf + buffer->offset;
}

/* Dumps the contents of the buffer to stderr. */

void buffer_dump(Buffer * buffer)
{
    int i;
    unsigned char *ucp = (unsigned char *) buffer->buf;

    for (i = buffer->offset; i < buffer->end; i++)
        fprintf(stderr, " %02x", ucp[i]);
    fprintf(stderr, "\n");
}
