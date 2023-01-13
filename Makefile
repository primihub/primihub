
linux_x86_64:
	bazel build --config=linux_x86_64 //:node //:cli //:linkcontext //:opt_paillier_c2py //:linkcontext

linux_aarch64:
	bazel build --config=linux_aarch64 //:node //:cli //:linkcontext //:opt_paillier_c2py //:linkcontext 

macos_arm64:
	bazel build --config=macos --define cpu=arm64 --define microsoft-apsi=true //:node //:cli //:opt_paillier_c2py //:linkcontext
