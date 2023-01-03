
linux_x86_64:
	#bazel build --config=linux --define cpu=amd64  --define microsoft-apsi=true //:node //:cli
	bazel build --config=linux --define cpu=amd64  --define microsoft-apsi=true //:node //:cli //:linkcontext //:opt_paillier_c2py

macos_arm64:
	bazel build --config=macos --define cpu=arm64 --define microsoft-apsi=true //:node //:cli //:opt_paillier_c2py //:linkcontext
