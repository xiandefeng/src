/*	$Id: test-mft.c,v 1.7 2020/11/02 13:40:58 tb Exp $ */
/*
 * Copyright (c) 2019 Kristaps Dzonsons <kristaps@bsd.lv>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/nameser.h>
#include <assert.h>
#include <err.h>
#include <resolv.h>	/* b64_ntop */
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <openssl/err.h>
#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/x509v3.h>

#include "extern.h"

int verbose;

static void
pem_print(X509 **xp)
{
	BIO	*bio = NULL;
	char	*pem = NULL;

	bio = BIO_new(BIO_s_mem());
	assert(bio != NULL);

	if (0 == PEM_write_bio_X509(bio, *xp)) {
		BIO_free(bio);
		errx(1, "PEM_write_bio_X509: unable to write cert");
	}

	if ((pem = (char *) malloc(bio->num_write + 1)) == NULL)
		err(1, NULL);

	memset(pem, 0, bio->num_write + 1);
	BIO_read(bio, pem, bio->num_write);
	BIO_free(bio);
	printf("%s", pem);
	free(pem);
}

static void
mft_print(const struct mft *p)
{
	size_t	 i;
	char hash[256];

	assert(p != NULL);

	printf("Subject key identifier: %s\n", p->ski);
	printf("Authority key identifier: %s\n", p->aki);
	for (i = 0; i < p->filesz; i++) {
		b64_ntop(p->files[i].hash, sizeof(p->files[i].hash),
		    hash, sizeof(hash));
		printf("%5zu: %s\n", i + 1, p->files[i].file);
		printf("\thash %s\n", hash);
	}
}


int
main(int argc, char *argv[])
{
	int		 c, i, ppem = 0, verb = 0;
	struct mft	*p;
	X509		*xp = NULL;

	ERR_load_crypto_strings();
	OpenSSL_add_all_ciphers();
	OpenSSL_add_all_digests();

	while (-1 != (c = getopt(argc, argv, "pv")))
		switch (c) {
		case 'p':
			ppem++;
			break;
		case 'v':
			verb++;
			break;
		default:
			errx(1, "bad argument %c", c);
		}

	argv += optind;
	argc -= optind;

	if (argc == 0)
		errx(1, "argument missing");

	for (i = 0; i < argc; i++) {
		if ((p = mft_parse(&xp, argv[i])) == NULL)
			break;
		if (verb)
			mft_print(p);
		if (ppem)
			pem_print(&xp);
		mft_free(p);
		X509_free(xp);
	}

	EVP_cleanup();
	CRYPTO_cleanup_all_ex_data();
	ERR_remove_state(0);
	ERR_free_strings();

	if (i < argc)
		errx(1, "test failed for %s", argv[i]);

	printf("OK\n");
	return 0;
}
