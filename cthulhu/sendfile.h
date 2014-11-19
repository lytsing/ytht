#ifndef SENDFILE_H
#define SENDFILE_H

extern ssize_t arch_sendfile __P ((client_t *));
extern ssize_t xwrite __P ((client_t *));
extern ssize_t xsendfile __P ((client_t *, off_t, size_t, struct iovec *, size_t));

#endif /* SENDFILE_H */
