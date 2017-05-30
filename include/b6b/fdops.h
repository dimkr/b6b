/*
 * This file is part of b6b.
 *
 * Copyright 2017 Dima Krasner
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

ssize_t b6b_fd_peeksz(struct b6b_interp *interp, void *priv);
ssize_t b6b_fd_on_read(struct b6b_interp *interp,
                       const ssize_t out,
                       int *eof);
ssize_t b6b_fd_read(struct b6b_interp *interp,
                    void *priv,
                    unsigned char *buf,
                    const size_t len,
                    int *eof,
                    int *again);
ssize_t b6b_fd_recv(struct b6b_interp *interp,
                    void *priv,
                    unsigned char *buf,
                    const size_t len,
                    int *eof,
                    int *again);

ssize_t b6b_fd_write(struct b6b_interp *interp,
                    void *priv,
                    const unsigned char *buf,
                    const size_t len);
ssize_t b6b_fd_send(struct b6b_interp *interp,
                    void *priv,
                    const unsigned char *buf,
                    const size_t len);

int b6b_fd_fd(void *priv);
void b6b_fd_close(void *priv);

ssize_t b6b_fd_peeksz_u64(struct b6b_interp *interp, void *priv);
ssize_t b6b_fd_read_u64(struct b6b_interp *interp,
                        void *priv,
                        unsigned char *buf,
                        const size_t len,
                        int *eof,
                        int *again);
ssize_t b6b_fd_write_u64(struct b6b_interp *interp,
                         void *priv,
                         const unsigned char *buf,
                         const size_t len);
