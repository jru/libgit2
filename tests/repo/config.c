#include "clar_libgit2.h"
#include "fileops.h"
#include <ctype.h>

static git_buf path = GIT_BUF_INIT;

void test_repo_config__initialize(void)
{
	cl_fixture_sandbox("empty_standard_repo");
	cl_git_pass(cl_rename("empty_standard_repo/.gitted", "empty_standard_repo/.git"));

	git_buf_clear(&path);

	cl_must_pass(p_mkdir("alternate", 0777));
	cl_git_pass(git_path_prettify(&path, "alternate", NULL));
}

void test_repo_config__cleanup(void)
{
	cl_git_pass(git_path_prettify(&path, "alternate", NULL));
	cl_git_pass(git_futils_rmdir_r(path.ptr, NULL, GIT_RMDIR_REMOVE_FILES));
	git_buf_free(&path);
	cl_assert(!git_path_isdir("alternate"));

	cl_fixture_cleanup("empty_standard_repo");
}

void test_repo_config__open_missing_global(void)
{
	git_repository *repo;
	git_config *config, *global;

	cl_git_pass(git_libgit2_opts(
		GIT_OPT_SET_SEARCH_PATH, GIT_CONFIG_LEVEL_GLOBAL, path.ptr));
	cl_git_pass(git_libgit2_opts(
		GIT_OPT_SET_SEARCH_PATH, GIT_CONFIG_LEVEL_SYSTEM, path.ptr));
	cl_git_pass(git_libgit2_opts(
		GIT_OPT_SET_SEARCH_PATH, GIT_CONFIG_LEVEL_XDG, path.ptr));

	cl_git_pass(git_repository_open(&repo, "empty_standard_repo"));
	cl_git_pass(git_repository_config(&config, repo));
	cl_git_pass(git_config_open_level(&global, config, GIT_CONFIG_LEVEL_GLOBAL));

	cl_git_pass(git_config_set_string(global, "test.set", "42"));

	git_config_free(global);
	git_config_free(config);
	git_repository_free(repo);

	git_futils_dirs_global_shutdown();
}

void test_repo_config__open_missing_global_with_separators(void)
{
	git_repository *repo;
	git_config *config, *global;

	cl_git_pass(git_buf_printf(&path, "%c%s", GIT_PATH_LIST_SEPARATOR, "dummy"));

	cl_git_pass(git_libgit2_opts(
		GIT_OPT_SET_SEARCH_PATH, GIT_CONFIG_LEVEL_GLOBAL, path.ptr));
	cl_git_pass(git_libgit2_opts(
		GIT_OPT_SET_SEARCH_PATH, GIT_CONFIG_LEVEL_SYSTEM, path.ptr));
	cl_git_pass(git_libgit2_opts(
		GIT_OPT_SET_SEARCH_PATH, GIT_CONFIG_LEVEL_XDG, path.ptr));

	git_buf_free(&path);

	cl_git_pass(git_repository_open(&repo, "empty_standard_repo"));
	cl_git_pass(git_repository_config(&config, repo));
	cl_git_pass(git_config_open_level(&global, config, GIT_CONFIG_LEVEL_GLOBAL));

	cl_git_pass(git_config_set_string(global, "test.set", "42"));

	git_config_free(global);
	git_config_free(config);
	git_repository_free(repo);

	git_futils_dirs_global_shutdown();
}

#include "repository.h"

void test_repo_config__read_no_configs(void)
{
	git_repository *repo;
	int val;

	cl_git_pass(git_libgit2_opts(
		GIT_OPT_SET_SEARCH_PATH, GIT_CONFIG_LEVEL_GLOBAL, path.ptr));
	cl_git_pass(git_libgit2_opts(
		GIT_OPT_SET_SEARCH_PATH, GIT_CONFIG_LEVEL_SYSTEM, path.ptr));
	cl_git_pass(git_libgit2_opts(
		GIT_OPT_SET_SEARCH_PATH, GIT_CONFIG_LEVEL_XDG, path.ptr));

	/* with none */

	cl_must_pass(p_unlink("empty_standard_repo/.git/config"));
	cl_assert(!git_path_isfile("empty_standard_repo/.git/config"));

	cl_git_pass(git_repository_open(&repo, "empty_standard_repo"));
	git_repository__cvar_cache_clear(repo);
	val = -1;
	cl_git_pass(git_repository__cvar(&val, repo, GIT_CVAR_ABBREV));
	cl_assert_equal_i(GIT_ABBREV_DEFAULT, val);
	git_repository_free(repo);

	git_futils_dirs_global_shutdown();

	/* with just system */

	cl_must_pass(p_mkdir("alternate/1", 0777));
	cl_git_pass(git_buf_joinpath(&path, path.ptr, "1"));
	cl_git_rewritefile("alternate/1/gitconfig", "[core]\n\tabbrev = 10\n");
	cl_git_pass(git_libgit2_opts(
		GIT_OPT_SET_SEARCH_PATH, GIT_CONFIG_LEVEL_SYSTEM, path.ptr));

	cl_git_pass(git_repository_open(&repo, "empty_standard_repo"));
	git_repository__cvar_cache_clear(repo);
	val = -1;
	cl_git_pass(git_repository__cvar(&val, repo, GIT_CVAR_ABBREV));
	cl_assert_equal_i(10, val);
	git_repository_free(repo);

	/* with xdg + system */

	cl_must_pass(p_mkdir("alternate/2", 0777));
	path.ptr[path.size - 1] = '2';
	cl_git_rewritefile("alternate/2/config", "[core]\n\tabbrev = 20\n");
	cl_git_pass(git_libgit2_opts(
		GIT_OPT_SET_SEARCH_PATH, GIT_CONFIG_LEVEL_XDG, path.ptr));

	cl_git_pass(git_repository_open(&repo, "empty_standard_repo"));
	git_repository__cvar_cache_clear(repo);
	val = -1;
	cl_git_pass(git_repository__cvar(&val, repo, GIT_CVAR_ABBREV));
	cl_assert_equal_i(20, val);
	git_repository_free(repo);

	/* with global + xdg + system */

	cl_must_pass(p_mkdir("alternate/3", 0777));
	path.ptr[path.size - 1] = '3';
	cl_git_rewritefile("alternate/3/.gitconfig", "[core]\n\tabbrev = 30\n");
	cl_git_pass(git_libgit2_opts(
		GIT_OPT_SET_SEARCH_PATH, GIT_CONFIG_LEVEL_GLOBAL, path.ptr));

	cl_git_pass(git_repository_open(&repo, "empty_standard_repo"));
	git_repository__cvar_cache_clear(repo);
	val = -1;
	cl_git_pass(git_repository__cvar(&val, repo, GIT_CVAR_ABBREV));
	cl_assert_equal_i(30, val);
	git_repository_free(repo);

	/* with all configs */

	cl_git_rewritefile("empty_standard_repo/.git/config", "[core]\n\tabbrev = 40\n");

	cl_git_pass(git_repository_open(&repo, "empty_standard_repo"));
	git_repository__cvar_cache_clear(repo);
	val = -1;
	cl_git_pass(git_repository__cvar(&val, repo, GIT_CVAR_ABBREV));
	cl_assert_equal_i(40, val);
	git_repository_free(repo);

	/* with all configs but delete the files ? */

	cl_git_pass(git_repository_open(&repo, "empty_standard_repo"));
	git_repository__cvar_cache_clear(repo);
	val = -1;
	cl_git_pass(git_repository__cvar(&val, repo, GIT_CVAR_ABBREV));
	cl_assert_equal_i(40, val);

	cl_must_pass(p_unlink("empty_standard_repo/.git/config"));
	cl_assert(!git_path_isfile("empty_standard_repo/.git/config"));

	cl_must_pass(p_unlink("alternate/1/gitconfig"));
	cl_assert(!git_path_isfile("alternate/1/gitconfig"));

	cl_must_pass(p_unlink("alternate/2/config"));
	cl_assert(!git_path_isfile("alternate/2/config"));

	cl_must_pass(p_unlink("alternate/3/.gitconfig"));
	cl_assert(!git_path_isfile("alternate/3/.gitconfig"));

	git_repository__cvar_cache_clear(repo);
	val = -1;
	cl_git_pass(git_repository__cvar(&val, repo, GIT_CVAR_ABBREV));
	cl_assert_equal_i(40, val);
	git_repository_free(repo);

	/* reopen */

	cl_assert(!git_path_isfile("empty_standard_repo/.git/config"));
	cl_assert(!git_path_isfile("alternate/3/.gitconfig"));

	cl_git_pass(git_repository_open(&repo, "empty_standard_repo"));
	git_repository__cvar_cache_clear(repo);
	val = -1;
	cl_git_pass(git_repository__cvar(&val, repo, GIT_CVAR_ABBREV));
	cl_assert_equal_i(7, val);
	git_repository_free(repo);

	cl_assert(!git_path_exists("empty_standard_repo/.git/config"));
	cl_assert(!git_path_exists("alternate/3/.gitconfig"));

	git_futils_dirs_global_shutdown();
}
