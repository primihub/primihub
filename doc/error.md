
### 1、proxy error
#### error message
ERROR: no such package '@rules_foreign_cc//foreign_cc': java.io.IOException: Error downloading [https://github.com/bazelbuild/rules_foreign_cc/archive/0.5.1.tar.gz] to /private/var/tmp/_bazel_yankaili/0debc33b2ff838305d7336badf927d09/external/rules_foreign_cc/temp18356444105672555342/0.5.1.tar.gz: Proxy address 127.0.0.1:7890 is not a valid URL

#### solution
set proxy

    export http{,s}_proxy=http://127.0.0.1:7890

### 2、Mac M1 compile error
#### darwin_x86_64

  https://githubhot.com/repo/bazelbuild/tulsi/issues/272

  M1

  https://engineering.mercari.com/en/blog/entry/20211210-bazel-remote-execution-for-ios-builds-with-apple-silicon/

  https://github.com/bazelbuild/bazel/commit/e4e06c0293bda20ad8c2b8db131ce821316b8d12#diff-1f4964536f397e8679625954c7139bd6

  https://github.com/bazelbuild/bazel/commit/b235517a1f3b91561a513ea4381365dba30dd592#diff-1f4964536f397e8679625954c7139bd6

  openssl

  https://groups.google.com/g/bazel-discuss/c/NCvZ3HYyTl0?pli=1

  https://github.com/bazelbuild/bazel/issues/7032

  https://github.com/bazelbuild/rules_foreign_cc/issues/337

  https://github.com/bazelbuild/rules_foreign_cc/issues/337#issuecomment-582500380

  https://github.com/openssl/openssl/issues/3840

  https://github.com/3rdparty/bazel-rules-openssl/blob/main/BUILD.openssl.bazel

### 3、install bazel@5.0.0

  wget https://mirrors.huaweicloud.com/bazel/5.0.0/bazel-5.0.0-installer-darwin-x86_64.sh
  chmod +x ./bazel-5.0.0-installer-darwin-x86_64.sh
  ./bazel-5.0.0-installer-darwin-x86_64.sh


### 4、bazel 5.0.0

    "_scm_call_1", referenced from:
      _internal_guile_eval in guile.o
      _guile_expand_wrapper in guile.o
      _guile_eval_wrapper in guile.o
    "_scm_eval_string", referenced from:
        _internal_guile_eval in guile.o
    "_scm_from_locale_string", referenced from:
        _guile_expand_wrapper in guile.o
    "_scm_from_utf8_string", referenced from:
        _internal_guile_eval in guile.o
    "_scm_to_locale_string", referenced from:
        _internal_guile_eval in guile.o
        _guile_expand_wrapper in guile.o
        _guile_eval_wrapper in guile.o
    "_scm_variable_ref", referenced from:
        _guile_init in guile.o
    "_scm_with_guile", referenced from:
        _func_guile in guile.o
  ld: symbol(s) not found for architecture x86_64
  clang: error: linker command failed with exit code 1 (use -v to see invocation)
  _____ END BUILD LOGS _____
  rules_foreign_cc: Build wrapper script location: bazel-out/darwin-opt-exec-2B5CBBC6/bin/external/rules_foreign_cc/toolchains/make_tool_foreign_cc/wrapper_build_script.sh
  rules_foreign_cc: Build script location: bazel-out/darwin-opt-exec-2B5CBBC6/bin/external/rules_foreign_cc/toolchains/make_tool_foreign_cc/build_script.sh
  rules_foreign_cc: Build log location: bazel-out/darwin-opt-exec-2B5CBBC6/bin/external/rules_foreign_cc/toolchains/make_tool_foreign_cc/BootstrapGNUMake.log

  Target //:node failed to build
  Use --verbose_failures to see the command lines of failed build steps.

### 5. ar: only one of -a and -[bi] options allowed

    ar -static -s apps/libapps.a apps/app_rand.o apps/apps.o apps/bf_prefix.o apps/opt.o apps/s_cb.o apps/s_socket.o
    ar: only one of -a and -[bi] options allowed
    usage:  ar -d [-TLsv] archive file ...
        ar -m [-TLsv] archive file ...
        ar -m [-abiTLsv] position archive file ...
    6.0.0-pre.20220411.2
        ar -p [-TLsv] archive [file ...]
        ar -q [-cTLsv] archive file ...
        ar -r [-cuTLsv] archive file ...
        ar -r [-abciuTLsv] position archive file ...
        ar -t [-TLsv] archive [file ...]
        ar -x [-ouTLsv] archive [file ...]
    make[1]: *** [Makefile:676: apps/libapps.a] Error 1
    make[1]: Leaving directory '/private/var/tmp/_bazel_liyankai/cb6f6703cfdeb7b1b37d6df6c4eeb127/sandbox/darwin-sandbox/399/execroot/__main__/bazel-out/darwin_arm64-fastbuild/bin/external/openssl/openssl.build_tmpdir'
    make: *** [Makefile:177: build_libs] Error 2
    make: Leaving directory '/private/var/tmp/_bazel_liyankai/cb6f6703cfdeb7b1b37d6df6c4eeb127/sandbox/darwin-sandbox/399/execroot/__main__/bazel-out/darwin_arm64-fastbuild/bin/external/openssl/openssl.build_tmpdir'

