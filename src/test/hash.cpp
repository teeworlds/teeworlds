#include <gtest/gtest.h>

#include <base/hash_ctxt.h>
#include <base/system.h>

static void Expect(SHA256_DIGEST Actual, const char *pWanted)
{
	char aActual[SHA256_MAXSTRSIZE];
	sha256_str(Actual, aActual, sizeof(aActual));
	EXPECT_STREQ(aActual, pWanted);
}

TEST(Hash, Sha256)
{
	// https://en.wikipedia.org/w/index.php?title=SHA-2&oldid=840187620#Test_vectors
	Expect(sha256("", 0), "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855");
	SHA256_CTX ctxt;

	sha256_init(&ctxt);
	Expect(sha256_finish(&ctxt), "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855");

	// printf 'The quick brown fox jumps over the lazy dog.' | sha256sum
	char QUICK_BROWN_FOX[] = "The quick brown fox jumps over the lazy dog.";
	Expect(sha256(QUICK_BROWN_FOX, str_length(QUICK_BROWN_FOX)), "ef537f25c895bfa782526529a9b63d97aa631564d5d789c2b765448c8635fb6c");

	sha256_init(&ctxt);
	sha256_update(&ctxt, "The ", 4);
	sha256_update(&ctxt, "quick ", 6);
	sha256_update(&ctxt, "brown ", 6);
	sha256_update(&ctxt, "fox ", 4);
	sha256_update(&ctxt, "jumps ", 6);
	sha256_update(&ctxt, "over ", 5);
	sha256_update(&ctxt, "the ", 4);
	sha256_update(&ctxt, "lazy ", 5);
	sha256_update(&ctxt, "dog.", 4);
	Expect(sha256_finish(&ctxt), "ef537f25c895bfa782526529a9b63d97aa631564d5d789c2b765448c8635fb6c");
}

TEST(Hash, Sha256Eq)
{
	EXPECT_EQ(sha256("", 0), sha256("", 0));
}
