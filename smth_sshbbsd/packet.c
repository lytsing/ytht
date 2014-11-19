/*

packet.c

Author: Tatu Ylonen <ylo@cs.hut.fi>

Copyright (c) 1995 Tatu Ylonen <ylo@cs.hut.fi>, Espoo, Finland
                   All rights reserved

Created: Sat Mar 18 02:40:40 1995 ylo

This file contains code implementing the packet protocol and communication
with the other side.  This same code is used both on client and server side.

*/

/*
 * $Id: packet.c 7781 2007-02-14 06:00:50Z ylsdd $
 * $Log$
 * Revision 1.2  2003/11/22 07:14:12  yuhuan
 * ssh no warning
 *
 * Revision 1.1.1.1  2002/10/01 09:42:05  ylsdd
 * ˮľ��sshbbsd����
 * Ȼ�������İ�
 *
 * Revision 1.4  2002/08/22 15:42:52  kcn
 * fix bug
 *
 * Revision 1.3  2002/08/04 11:39:42  kcn
 * format c
 *
 * Revision 1.2  2002/08/04 11:08:47  kcn
 * format C
 *
 * Revision 1.1.1.1  2002/04/27 05:47:25  kxn
 * no message
 *
 * Revision 1.1  2001/07/04 06:07:11  bbsdev
 * bbs sshd
 *
 * Revision 1.12  1999/02/21 19:52:30  ylo
 * 	Intermediate commit of ssh1.2.27 stuff.
 * 	Main change is sprintf -> snprintf; however, there are also
 * 	many other changes.
 *
 * Revision 1.11  1998/06/11 00:08:44  kivinen
 *      Crc fixing detection code.
 *
 * Revision 1.10  1998/05/23  20:22:50  kivinen
 *      Changed () -> (void).
 *
 * Revision 1.9  1998/04/30  01:54:30  kivinen
 *      Fixed SSH_MSG_IGNORE handling. Now it will skip the string
 *      argument also.
 *
 * Revision 1.8  1998/03/27 16:59:02  kivinen
 *      Added ENABLE_TCP_NODELAY support.
 *
 * Revision 1.7  1997/04/05 17:29:21  ylo
 *      Added packet_get_len (returns the remaining length of incoming
 *      packet).
 *
 * Revision 1.6  1997/03/19 19:26:58  kivinen
 *      Added packet_get_all function. Added checks to
 *      packet_read_poll that previous packet buffer must be empty
 *      before starting to put new packet in.
 *
 * Revision 1.5  1996/11/24 08:23:46  kivinen
 *      Changed all code that checked EAGAIN to check EWOULDBLOCK too.
 *      Added support for debug messages that are printed always (if
 *      the first character of debug message is * it is printed with
 *      error function instead of debug).
 *
 * Revision 1.4  1996/10/07 11:53:24  ttsalo
 *      From "Charles M. Hannum" <mycroft@gnu.ai.mit.edu>:
 *      Made the use of TCP_NODELAY conditional.
 *
 * Revision 1.3  1996/09/22 21:58:38  ylo
 *      Added code to clear keepalives properly when requested.
 *
 * Revision 1.2  1996/05/28 16:42:13  ylo
 *      Workaround for Solaris select() bug while reading version id.
 *
 * Revision 1.1.1.1  1996/02/18 21:38:12  ylo
 *      Imported ssh-1.2.13.
 *
 * Revision 1.6  1995/09/24  23:59:12  ylo
 *      Added packet_get_protocol_flags.
 *
 * Revision 1.5  1995/09/09  21:26:43  ylo
 * /m/shadows/u2/users/ylo/ssh/README
 *
 * Revision 1.4  1995/07/27  03:59:37  ylo
 *      Fixed a bug in new arcfour keying.
 *
 * Revision 1.3  1995/07/27  02:17:11  ylo
 *      Changed keying for arcfour to avoid using the same key for both
 *      directions.
 *
 * Revision 1.2  1995/07/13  01:27:33  ylo
 *      Removed "Last modified" header.
 *      Added cvs log.
 *
 * $Endlog$
 */

#include "includes.h"
#include "xmalloc.h"
#include "randoms.h"
#include "buffer.h"
#include "packet.h"
#include "bufaux.h"
#include "ssh.h"
#include "crc32.h"
#include "cipher.h"
#include "getput.h"
#include "compress.h"
#include "deattack.h"

/* This variable contains the file descriptors used for communicating with
   the other side.  connection_in is used for reading; connection_out
   for writing.  These can be the same descriptor, in which case it is
   assumed to be a socket. */
static int connection_in = -1;
static int connection_out = -1;

/* Cipher type.  This value is only used to determine whether to pad the
   packets with zeroes or random data. */
static int cipher_type = SSH_CIPHER_NONE;

/* Protocol flags for the remote side. */
static unsigned int remote_protocol_flags = 0;

/* Encryption context for receiving data.  This is only used for decryption. */
static CipherContext receive_context;

/* Encryption coontext for sending data.  This is only used for encryption. */
static CipherContext send_context;

/* Buffer for raw input data from the socket. */
static Buffer input;

/* Buffer for raw output data going to the socket. */
static Buffer output;

/* Buffer for the partial outgoing packet being constructed. */
static Buffer outgoing_packet;

/* Buffer for the incoming packet currently being processed. */
static Buffer incoming_packet;

/* Scratch buffer for packet compression/decompression. */
static Buffer compression_buffer;

/* Flag indicating whether packet compression/decompression is enabled. */
static int packet_compression = 0;

/* Pointer to the random number generator state. */
static RandomState *random_state;

/* Flag indicating whether this module has been initialized. */
int initialized = 0;

/* Set to true if the connection is interactive. */
static int interactive_mode = 0;

/* Maximum size of a packet. */
static unsigned int max_packet_size = (sizeof(int) < 4) ? 32000 : 256000;

/* Sets the descriptors used for communication.  Disables encryption until
   packet_set_encryption_key is called. */

void packet_set_connection(int fd_in, int fd_out, RandomState * state)
{
    connection_in = fd_in;
    connection_out = fd_out;
    random_state = state;
    cipher_type = SSH_CIPHER_NONE;
    cipher_set_key(&send_context, SSH_CIPHER_NONE, (unsigned char *) "", 0, 1);
    cipher_set_key(&receive_context, SSH_CIPHER_NONE, (unsigned char *) "", 0, 0);
    if (!initialized) {
        initialized = 1;
        buffer_init(&input);
        buffer_init(&output);
        buffer_init(&outgoing_packet);
        buffer_init(&incoming_packet);
    }

    /* Kludge: arrange the close function to be called from fatal(). */
    fatal_add_cleanup((void (*)(void *)) packet_close, NULL);

    /* Initially set the connection interactive and enable keepalives.  The
       values will be reset to their final values later (after 
       authentication). */
    packet_set_interactive(1, 1);
}

/* Sets the connection into non-blocking mode. */

void packet_set_nonblocking(void)
{
    /* Set the socket into non-blocking mode. */
#if defined(O_NONBLOCK) && !defined(O_NONBLOCK_BROKEN)
    if (fcntl(connection_in, F_SETFL, O_NONBLOCK) < 0)
        error("fcntl O_NONBLOCK: %.100s", strerror(errno));
#else                           /* O_NONBLOCK && !O_NONBLOCK_BROKEN */
    if (fcntl(connection_in, F_SETFL, O_NDELAY) < 0)
        error("fcntl O_NDELAY: %.100s", strerror(errno));
#endif                          /* O_NONBLOCK && !O_NONBLOCK_BROKEN */

    if (connection_out != connection_in) {
#if defined(O_NONBLOCK) && !defined(O_NONBLOCK_BROKEN)
        if (fcntl(connection_out, F_SETFL, O_NONBLOCK) < 0)
            error("fcntl O_NONBLOCK: %.100s", strerror(errno));
#else                           /* O_NONBLOCK && !O_NONBLOCK_BROKEN */
        if (fcntl(connection_out, F_SETFL, O_NDELAY) < 0)
            error("fcntl O_NDELAY: %.100s", strerror(errno));
#endif                          /* O_NONBLOCK && !O_NONBLOCK_BROKEN */
    }
}

/* Returns the socket used for reading. */

int packet_get_connection_in(void)
{
    return connection_in;
}

/* Returns the descriptor used for writing. */

int packet_get_connection_out(void)
{
    return connection_out;
}

/* Closes the connection and clears and frees internal data structures. */

void packet_close(void)
{
    if (!initialized)
        return;
    initialized = 0;
    if (connection_in == connection_out) {
        shutdown(connection_out, 2);
        close(connection_out);
    } else {
        close(connection_in);
        close(connection_out);
    }
    buffer_free(&input);
    buffer_free(&output);
    buffer_free(&outgoing_packet);
    buffer_free(&incoming_packet);
    if (packet_compression) {
        buffer_free(&compression_buffer);
        buffer_compress_uninit();
    }
}

/* Sets remote side protocol flags. */

void packet_set_protocol_flags(unsigned int protocol_flags)
{
    remote_protocol_flags = protocol_flags;
//  channel_set_options((protocol_flags & SSH_PROTOFLAG_HOST_IN_FWD_OPEN) != 0);
}

/* Returns the remote protocol flags set earlier by the above function. */

unsigned int packet_get_protocol_flags(void)
{
    return remote_protocol_flags;
}

/* Starts packet compression from the next packet on in both directions. 
   Level is compression level 1 (fastest) - 9 (slow, best) as in gzip. */

void packet_start_compression(int level)
{
    if (packet_compression)
        fatal("Compression already enabled.");
    packet_compression = 1;
    buffer_init(&compression_buffer);
    buffer_compress_init(level);
}

/* Encrypts the given number of bytes, copying from src to dest.
   bytes is known to be a multiple of 8. */

void packet_encrypt(CipherContext * cc, void *dest, void *src, unsigned int bytes)
{
    assert((bytes % 8) == 0);
    cipher_encrypt(cc, dest, src, bytes);
}

/* Decrypts the given number of bytes, copying from src to dest.
   bytes is known to be a multiple of 8. */

void packet_decrypt(CipherContext * cc, void *dest, void *src, unsigned int bytes)
{
    int i;

    assert((bytes % 8) == 0);

    /* $Id: packet.c 7781 2007-02-14 06:00:50Z ylsdd $
     * Cryptographic attack detector for ssh - Modifications for packet.c 
     * (C)1998 CORE-SDI, Buenos Aires Argentina
     * Ariel Futoransky(futo@core-sdi.com)
     */

    switch (cc->type) {
#ifndef WITHOUT_IDEA
    case SSH_CIPHER_IDEA:
        i = detect_attack(src, bytes, cc->u.idea.iv);
        break;
#endif
    case SSH_CIPHER_NONE:
        i = DEATTACK_OK;
        break;
    default:
        i = detect_attack(src, bytes, NULL);
        break;
    }

    if (i == DEATTACK_DETECTED)
        packet_disconnect("crc32 compensation attack: network attack detected");

    cipher_decrypt(cc, dest, src, bytes);
}

/* Causes any further packets to be encrypted using the given key.  The same
   key is used for both sending and reception.  However, both directions
   are encrypted independently of each other. */

void packet_set_encryption_key(const unsigned char *key, unsigned int keylen, int cipher, int is_client)
{
    cipher_type = cipher;
    if (cipher == SSH_CIPHER_ARCFOUR) {
        if (is_client) {        /* In client: use first half for receiving, second for sending. */
            cipher_set_key(&receive_context, cipher, key, keylen / 2, 0);
            cipher_set_key(&send_context, cipher, key + keylen / 2, keylen / 2, 1);
        } else {                /* In server: use first half for sending, second for receiving. */
            cipher_set_key(&receive_context, cipher, key + keylen / 2, keylen / 2, 0);
            cipher_set_key(&send_context, cipher, key, keylen / 2, 1);
        }
    } else {
        /* All other ciphers use the same key in both directions for now. */
        cipher_set_key(&receive_context, cipher, key, keylen, 0);
        cipher_set_key(&send_context, cipher, key, keylen, 1);
    }
}

/* Starts constructing a packet to send. */

void packet_start(int type)
{
    char buf[9];

    buffer_clear(&outgoing_packet);
    memset(buf, 0, 8);
    buf[8] = type;
    buffer_append(&outgoing_packet, buf, 9);
}

/* Appends a character to the packet data. */

void packet_put_char(int value)
{
    char ch = value;

    buffer_append(&outgoing_packet, &ch, 1);
}

/* Appends an integer to the packet data. */

void packet_put_int(unsigned int value)
{
    buffer_put_int(&outgoing_packet, value);
}

/* Appends a string to packet data. */

void packet_put_string(const char *buf, unsigned int len)
{
    buffer_put_string(&outgoing_packet, buf, len);
}

/* Appends an arbitrary precision integer to packet data. */

void packet_put_mp_int(MP_INT * value)
{
    buffer_put_mp_int(&outgoing_packet, value);
}

/* Finalizes and sends the packet.  If the encryption key has been set,
   encrypts the packet before sending. */

void packet_send(void)
{
    char buf[8], *cp;
    int i, padding, len;
    unsigned long checksum;

    if (buffer_len(&outgoing_packet) >= max_packet_size - 30)
        fatal("packet_send: sending too big a packet: size %u, limit %u.", buffer_len(&outgoing_packet), max_packet_size);

    /* If using packet compression, compress the payload of the outgoing
       packet. */
    if (packet_compression) {
        buffer_clear(&compression_buffer);
        buffer_consume(&outgoing_packet, 8);    /* Skip padding. */
        buffer_append(&compression_buffer, "\0\0\0\0\0\0\0\0", 8);      /* padding */
        buffer_compress(&outgoing_packet, &compression_buffer);
        buffer_clear(&outgoing_packet);
        buffer_append(&outgoing_packet, buffer_ptr(&compression_buffer), buffer_len(&compression_buffer));
    }

    /* Compute packet length without padding (add checksum, remove padding). */
    len = buffer_len(&outgoing_packet) + 4 - 8;

    /* Insert padding. */
    padding = 8 - len % 8;
    if (cipher_type != SSH_CIPHER_NONE) {
        cp = buffer_ptr(&outgoing_packet);
        for (i = 0; i < padding; i++)
            cp[7 - i] = random_get_byte(random_state);
    }
    buffer_consume(&outgoing_packet, 8 - padding);

    /* Add check bytes. */
    checksum = ssh_crc32((unsigned char *) buffer_ptr(&outgoing_packet), buffer_len(&outgoing_packet));
    PUT_32BIT(buf, checksum);
    buffer_append(&outgoing_packet, buf, 4);

#ifdef PACKET_DEBUG
    fprintf(stderr, "packet_send plain: ");
    buffer_dump(&outgoing_packet);
#endif

    /* Append to output. */
    PUT_32BIT(buf, len);
    buffer_append(&output, buf, 4);
    buffer_append_space(&output, &cp, buffer_len(&outgoing_packet));
    packet_encrypt(&send_context, cp, buffer_ptr(&outgoing_packet), buffer_len(&outgoing_packet));

#ifdef PACKET_DEBUG
    fprintf(stderr, "encrypted: ");
    buffer_dump(&output);
#endif

    buffer_clear(&outgoing_packet);

    /* Note that the packet is now only buffered in output.  It won\'t be
       actually sent until packet_write_wait or packet_write_poll is called. */
}

/* Waits until a packet has been received, and returns its type.  Note that
   no other data is processed until this returns, so this function should
   not be used during the interactive session. */

int packet_read(void)
{
    int type, len;
    fd_set set;
    char buf[8192];

    /* Since we are blocking, ensure that all written packets have been sent. */
    packet_write_wait();

    /* Stay in the loop until we have received a complete packet. */
    for (;;) {
        /* Try to read a packet from the buffer. */
        type = packet_read_poll();
        /* If we got a packet, return it. */
        if (type != SSH_MSG_NONE)
            return type;
        /* Otherwise, wait for some data to arrive, add it to the buffer,
           and try again. */
        FD_ZERO(&set);
        FD_SET(connection_in, &set);
        /* Wait for some data to arrive. */
        select(connection_in + 1, &set, NULL, NULL, NULL);
        /* Read data from the socket. */
        len = read(connection_in, buf, sizeof(buf));
        if (len == 0)
            fatal_severity(SYSLOG_SEVERITY_INFO, "Connection closed by remote host.");
        if (len < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
                continue;
            fatal_severity(SYSLOG_SEVERITY_INFO, "Read from socket failed: %.100s", strerror(errno));
        }
        /* Append it to the buffer. */
        packet_process_incoming(buf, len);
    }
 /*NOTREACHED*/}

/* Waits until a packet has been received, verifies that its type matches
   that given, and gives a fatal error and exits if there is a mismatch. */

void packet_read_expect(int expected_type)
{
    int type;

    type = packet_read();
    if (type != expected_type)
        packet_disconnect("Protocol error: expected packet type %d, got %d", expected_type, type);
}

/* Checks if a full packet is available in the data received so far via
   packet_process_incoming.  If so, reads the packet; otherwise returns
   SSH_MSG_NONE.  This does not wait for data from the connection. 
   
   SSH_MSG_DISCONNECT is handled specially here.  Also,
   SSH_MSG_IGNORE messages are skipped by this function and are never returned
   to higher levels. */

int packet_read_poll(void)
{
    unsigned int len, padded_len;
    unsigned char *ucp;
    char buf[8], *cp;
    unsigned long checksum, stored_checksum;

  restart:

    /* Check if input size is less than minimum packet size. */
    if (buffer_len(&input) < 4 + 8)
        return SSH_MSG_NONE;
    /* Get length of incoming packet. */
    ucp = (unsigned char *) buffer_ptr(&input);
    len = GET_32BIT(ucp);
    if (len < 1 + 2 + 2 || len > 256 * 1024)
        packet_disconnect("Bad packet length %u.", len);
    padded_len = (len + 8) & ~7;

    /* Check if the packet has been entirely received. */
    if (buffer_len(&input) < 4 + padded_len)
        return SSH_MSG_NONE;

    /* The entire packet is in buffer. */

    /* Consume packet length. */
    buffer_consume(&input, 4);

/* Confirm that packet is empty after all data is processed from it. Calls
   fatal buffer is not empty. */
    if (buffer_len(&incoming_packet) != 0)
        packet_disconnect("Junk data left to incoming packet buffer after all data processed");

    /* Copy data to incoming_packet. */
    buffer_clear(&incoming_packet);
    buffer_append_space(&incoming_packet, &cp, padded_len);
    packet_decrypt(&receive_context, cp, buffer_ptr(&input), padded_len);
    buffer_consume(&input, padded_len);

#ifdef PACKET_DEBUG
    fprintf(stderr, "read_poll plain: ");
    buffer_dump(&incoming_packet);
#endif

    /* Compute packet checksum. */
    checksum = ssh_crc32((unsigned char *) buffer_ptr(&incoming_packet), buffer_len(&incoming_packet) - 4);

    /* Skip padding. */
    buffer_consume(&incoming_packet, 8 - len % 8);

    /* Test check bytes. */
    assert(len == buffer_len(&incoming_packet));
    ucp = (unsigned char *) buffer_ptr(&incoming_packet) + len - 4;
    stored_checksum = GET_32BIT(ucp);
    if (checksum != stored_checksum)
        packet_disconnect("Corrupted check bytes on input.");
    buffer_consume_end(&incoming_packet, 4);

    /* If using packet compression, decompress the packet. */
    if (packet_compression) {
        buffer_clear(&compression_buffer);
        buffer_uncompress(&incoming_packet, &compression_buffer);
        buffer_clear(&incoming_packet);
        buffer_append(&incoming_packet, buffer_ptr(&compression_buffer), buffer_len(&compression_buffer));
    }

    /* Get packet type. */
    buffer_get(&incoming_packet, &buf[0], 1);

    /* Handle disconnect message. */
    if ((unsigned char) buf[0] == SSH_MSG_DISCONNECT)
        fatal_severity(SYSLOG_SEVERITY_INFO, "%.900s", packet_get_string(NULL));

    /* Ignore ignore messages. */
    if ((unsigned char) buf[0] == SSH_MSG_IGNORE) {
        char *str;

        str = packet_get_string(NULL);
        xfree(str);
        goto restart;
    }

    /* Send debug messages as debugging output. */
    if ((unsigned char) buf[0] == SSH_MSG_DEBUG) {
        char *str;

        str = packet_get_string(NULL);
        if (*str == '*') {      /* Magical kludge to force displaying
                                   debug messages with '*' anyway, even
                                   if not in verbose mode. */
            error("Remote: %.900s", str);
        } else {
            debug("Remote: %.900s", str);
        }
        xfree(str);
        goto restart;
    }

    /* Return type. */
    return (unsigned char) buf[0];
}

/* Buffers the given amount of input characters.  This is intended to be
   used together with packet_read_poll. */

void packet_process_incoming(const char *buf, unsigned int len)
{
    buffer_append(&input, buf, len);
}

/* Returns the number of bytes left in the incoming packet. */

unsigned int packet_get_len(void)
{
    return buffer_len(&incoming_packet);
}

/* Returns a character from the packet. */

unsigned int packet_get_char(void)
{
    char ch;

    buffer_get(&incoming_packet, &ch, 1);
    return (unsigned char) ch;
}

/* Returns an integer from the packet data. */

unsigned int packet_get_int(void)
{
    return buffer_get_int(&incoming_packet);
}

/* Returns an arbitrary precision integer from the packet data.  The integer
   must have been initialized before this call. */

void packet_get_mp_int(MP_INT * value)
{
    buffer_get_mp_int(&incoming_packet, value);
}

/* Returns a string from the packet data.  The string is allocated using
   xmalloc; it is the responsibility of the calling program to free it when
   no longer needed.  The length_ptr argument may be NULL, or point to an
   integer into which the length of the string is stored. */

char *packet_get_string(unsigned int *length_ptr)
{
    return buffer_get_string(&incoming_packet, length_ptr);
}

/* Clears incoming data buffer */

void packet_get_all(void)
{
    buffer_clear(&incoming_packet);
}

/* Sends a diagnostic message from the server to the client.  This message
   can be sent at any time (but not while constructing another message).
   The message is printed immediately, but only if the client is being
   executed in verbose mode.  These messages are primarily intended to
   ease debugging authentication problems.   The length of the formatted
   message must not exceed 1024 bytes.  This will automatically call
   packet_write_wait. */

void packet_send_debug(const char *fmt, ...)
{
    char buf[1024];
    va_list args;

    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);

    packet_start(SSH_MSG_DEBUG);
    packet_put_string(buf, strlen(buf));
    packet_send();
    packet_write_wait();
}

/* Logs the error plus constructs and sends a disconnect
   packet, closes the connection, and exits.  This function never returns.
   The error message should not contain a newline.  The length of the
   formatted message must not exceed 1024 bytes. */

void packet_disconnect(const char *fmt, ...)
{
    char buf[1024];
    va_list args;
    static int disconnecting = 0;

    if (disconnecting)          /* Guard against recursive invocations. */
        fatal("packet_disconnect called recursively.");
    disconnecting = 1;

    /* Format the message.  Note that the caller must make sure the message
       is of limited size. */
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);

    /* Send the disconnect message to the other side, and wait for it to get 
       sent. */
    packet_start(SSH_MSG_DISCONNECT);
    packet_put_string(buf, strlen(buf));
    packet_send();
    packet_write_wait();


    /* Close the connection. */
    packet_close();

    /* Display the error locally and exit. */
    fatal("Local: %.100s", buf);
}

/* Checks if there is any buffered output, and tries to write some of the
   output. */

void packet_write_poll(void)
{
    int len = buffer_len(&output);

    if (len > 0) {
        len = write(connection_out, buffer_ptr(&output), len);
        if (len <= 0) {
            if (len != 0 && (errno == EAGAIN || errno == EWOULDBLOCK))
                return;
            else
                fatal_severity(SYSLOG_SEVERITY_INFO, "Write failed: %.100s", strerror(errno));
	}
        buffer_consume(&output, len);
    }
}

/* Calls packet_write_poll repeatedly until all pending output data has
   been written. */

void packet_write_wait(void)
{
    packet_write_poll();
    while (packet_have_data_to_write()) {
        fd_set set;

        FD_ZERO(&set);
        FD_SET(connection_out, &set);
        select(connection_out + 1, NULL, &set, NULL, NULL);
        packet_write_poll();
    }
}

/* Returns true if there is buffered data to write to the connection. */

int packet_have_data_to_write(void)
{
    return buffer_len(&output) != 0;
}

/* Returns true if there is not too much data to write to the connection. */

int packet_not_very_much_data_to_write(void)
{
    if (interactive_mode || sizeof(int) < 4)
        return buffer_len(&output) < 16384;
    else
        return buffer_len(&output) < 128 * 1024;
}

/* Informs that the current session is interactive.  Sets IP flags for that. */

void packet_set_interactive(int interactive, int keepalives)
{
    int on = 1, off = 0;

    /* Record that we are in interactive mode. */
    interactive_mode = interactive;

    /* Only set socket options if using a socket (as indicated by the descriptors
       being the same). */
    if (connection_in != connection_out)
        return;

    if (keepalives) {
        /* Set keepalives if requested. */
        if (setsockopt(connection_in, SOL_SOCKET, SO_KEEPALIVE, (void *) &on, sizeof(on)) < 0)
            error("setsockopt SO_KEEPALIVE: %.100s", strerror(errno));
    } else {
        /* Clear keepalives if we don't want them. */
        if (setsockopt(connection_in, SOL_SOCKET, SO_KEEPALIVE, (void *) &off, sizeof(off)) < 0)
            error("setsockopt SO_KEEPALIVE off: %.100s", strerror(errno));
    }

    if (interactive) {
        /* Set IP options for an interactive connection.  Use IPTOS_LOWDELAY
           and TCP_NODELAY. */
#ifdef IPTOS_LOWDELAY
        int lowdelay = IPTOS_LOWDELAY;

        if (setsockopt(connection_in, IPPROTO_IP, IP_TOS, (void *) &lowdelay, sizeof(lowdelay)) < 0)
            error("setsockopt IPTOS_LOWDELAY: %.100s", strerror(errno));
#endif                          /* IPTOS_LOWDELAY */
#if defined(TCP_NODELAY) && defined(ENABLE_TCP_NODELAY)
        if (setsockopt(connection_in, IPPROTO_TCP, TCP_NODELAY, (void *) &on, sizeof(on)) < 0)
            error("setsockopt TCP_NODELAY: %.100s", strerror(errno));
#endif                          /* TCP_NODELAY */
    } else {
        /* Set IP options for a non-interactive connection.  Use 
           IPTOS_THROUGHPUT. */
#ifdef IPTOS_THROUGHPUT
        int throughput = IPTOS_THROUGHPUT;

        if (setsockopt(connection_in, IPPROTO_IP, IP_TOS, (void *) &throughput, sizeof(throughput)) < 0)
            error("setsockopt IPTOS_THROUGHPUT: %.100s", strerror(errno));
#endif                          /* IPTOS_THROUGHPUT */
#if defined(TCP_NODELAY) && defined(ENABLE_TCP_NODELAY)
        if (setsockopt(connection_in, IPPROTO_TCP, TCP_NODELAY, (void *) &off, sizeof(off)) < 0)
            error("setsockopt TCP_NODELAY: %.100s", strerror(errno));
#endif                          /* TCP_NODELAY */
    }
}

/* Returns true if the current connection is interactive. */

int packet_is_interactive(void)
{
    return interactive_mode;
}

/* Sets the maximum packet size that can be sent to the other side. */

void packet_set_max_size(unsigned int max_size)
{
    max_packet_size = max_size;
}

/* Returns the maximum packet size that can be sent to the other side. */

unsigned int packet_max_size(void)
{
    return max_packet_size;
}
